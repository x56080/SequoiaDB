/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dmsStorageIndex.cpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "dpsOp2Record.hpp"
#include "ixmExtent.hpp"
#include "bpsPrefetch.hpp"
#include "dmsCompress.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsIndexBuilder.hpp"

using namespace bson ;

namespace engine
{

   _dmsStorageIndex::_dmsStorageIndex( const CHAR * pSuFileName,
                                       dmsStorageInfo * pInfo,
                                       dmsStorageData * pDataSu )
   :_dmsStorageBase( pSuFileName, pInfo )
   {
      SDB_ASSERT( pDataSu, "Data Su can't be NULL" ) ;
      _pDataSu = pDataSu ;

      _pDataSu->_attach( this ) ;
   }

   _dmsStorageIndex::~_dmsStorageIndex()
   {
      _pDataSu->_detach() ;

      _pDataSu = NULL ;
   }

   dmsPageMapUnit* _dmsStorageIndex::getPageMapUnit()
   {
      return &_mbPageInfo ;
   }

   dmsPageMap* _dmsStorageIndex::getPageMap( UINT16 mbID )
   {
      return _mbPageInfo.getMap( mbID ) ;
   }

   UINT64 _dmsStorageIndex::_dataOffset()
   {
      return ( DMS_SME_OFFSET + DMS_SME_SZ ) ;
   }

   const CHAR* _dmsStorageIndex::_getEyeCatcher() const
   {
      return DMS_INDEXSU_EYECATCHER ;
   }

   UINT32 _dmsStorageIndex::_curVersion() const
   {
      return DMS_INDEXSU_CUR_VERSION ;
   }

   INT32 _dmsStorageIndex::_checkVersion( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;
      if ( pHeader->_version > _curVersion() )
      {
         PD_LOG( PDERROR, "Incompatible version: %u", pHeader->_version ) ;
         rc = SDB_DMS_INCOMPATIBLE_VERSION ;
      }
      else if ( pHeader->_secretValue != _pStorageInfo->_secretValue )
      {
         PD_LOG( PDERROR, "Secret value[%llu] not the same with data su[%llu]",
                 pHeader->_secretValue, _pStorageInfo->_secretValue ) ;
         rc = SDB_DMS_SECRETVALUE_NOT_SAME ;
      }
      return rc ;
   }

   INT32 _dmsStorageIndex::_onCreate( OSSFILE * file, UINT64 curOffSet )
   {
      return SDB_OK ;
   }

   INT32 _dmsStorageIndex::_onMapMeta( UINT64 curOffSet )
   {
      return SDB_OK ;
   }

   INT32 _dmsStorageIndex::_onOpened()
   {
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         if ( DMS_IS_MB_INUSE ( _pDataSu->_dmsMME->_mbList[i]._flag ) )
         {
            // analyze the unique index number
            for ( UINT32 j = 0 ; j < DMS_COLLECTION_MAX_INDEX ; ++j )
            {
               if ( DMS_INVALID_EXTENT ==
                    _pDataSu->_dmsMME->_mbList[i]._indexExtent[ j ] )
               {
                  break ;
               }
               ixmIndexCB indexCB( _pDataSu->_dmsMME->_mbList[i]._indexExtent[ j ],
                                   this, NULL ) ;
               if ( indexCB.isInitialized() && indexCB.unique() )
               {
                  _pDataSu->_mbStatInfo[i]._uniqueIdxNum++ ;
               }
            }
         }
      }

      return SDB_OK ;
   }

   void _dmsStorageIndex::_onClosed()
   {
      /// Flush all pageMap to disk
      UINT16 pos = 0 ;
      dmsPageMap *pPageMap = NULL ;
      dmsPageMap::MAP_PAGES_IT it ;

      pPageMap = _mbPageInfo.beginNonEmpty( pos ) ;
      while( pPageMap )
      {
         it = pPageMap->begin() ;
         while( it != pPageMap->end() )
         {
            ixmExtent extent( it->first, this ) ;
            extent.setParent( it->second, FALSE ) ;
            ++it ;
         }
         pPageMap->clear() ;

         pPageMap = _mbPageInfo.nextNonEmpty( pos ) ;
      }
   }

   INT32 _dmsStorageIndex::reserveExtent( UINT16 mbID, dmsExtentID &extentID,
                                          dmsContext * context )
   {
      SDB_ASSERT( mbID < DMS_MME_SLOTS, "Invalid metadata block ID" ) ;

      INT32 rc                = SDB_OK ;
      dmsExtent *extAddr      = NULL ;
      extentID                = DMS_INVALID_EXTENT ;

      rc = _findFreeSpace ( 1, extentID, NULL/*context*/ ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Error find free space for %d pages, rc = %d",
                  1, rc ) ;
         goto error ;
      }

      extAddr = (dmsExtent*)extentAddr( extentID ) ;
      extAddr->init( 1, mbID, pageSize() ) ;

      _pDataSu->_mbStatInfo[mbID]._totalIndexPages += 1 ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::releaseExtent( dmsExtentID extentID )
   {
      INT32 rc             = SDB_OK ;
      dmsExtent *extAddr   = (dmsExtent*)extentAddr( extentID ) ;
      if ( !extAddr )
      {
         PD_LOG ( PDERROR, "extent id %d is not valid", extentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      if ( DMS_EXTENT_FLAG_INUSE != extAddr->_flag )
      {
         PD_LOG ( PDERROR, "extent id %d is not in use", extentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      extAddr->_flag = DMS_EXTENT_FLAG_FREED ;
      _pDataSu->_mbStatInfo[extAddr->_mbID]._totalIndexPages -= 1 ;
      rc = _releaseSpace( extentID, 1 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page, rc = %d", rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::createIndex( dmsMBContext *context,
                                        const BSONObj &index,
                                        pmdEDUCB * cb,
                                        SDB_DPSCB *dpscb,
                                        BOOLEAN isSys,
                                        INT32 sortBufferSize )
   {
      INT32 rc                     = SDB_OK ;
      dmsExtentID extentID         = DMS_INVALID_EXTENT ;
      dmsExtentID rootExtentID     = DMS_INVALID_EXTENT ;
      INT32  indexID               = 0 ;
      dmsExtentID indexLID         = DMS_INVALID_EXTENT ;

      BSONObj indexDef ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      dpsTransCB *pTransCB         = pmdGetKRCB()->getTransCB();
      UINT32 logRecSize            = 0;
      SDB_DPSCB *dropDps           = NULL ;
      INT32 rc1                    = 0 ;
      const CHAR *indexName = NULL ;

      if ( !ixmIndexCB::validateKey ( index, isSys ) )
      {
         PD_LOG_MSG ( PDERROR, "Index pattern is not valid" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _registerNewWriting() ;

      // let's first reserve extent
      rc = reserveExtent ( context->mbID(), extentID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reserve extent for collection[%u], "
                   "rc: %d", context->mbID(), rc ) ;

      // then let's reserve another extent for root extent ID
      rc = reserveExtent ( context->mbID(), rootExtentID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reserve root extent for collection"
                   "[%u], rc: %d", context->mbID(), rc ) ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_CRT_INDEX ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      indexName = index.getStringField( IXM_FIELD_NAME_NAME ) ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; indexID++ )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB curIdxCB( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         BOOLEAN sameName = (0 == ossStrncmp ( indexName,
                                              curIdxCB.getName(),
                                              IXM_INDEX_NAME_SIZE)) ;
         if ( sameName &&
              curIdxCB.isSameDef( index, TRUE ) )
         {
            PD_LOG( PDERROR, "Same index defined already:[%s:%s]",
                    curIdxCB.getName(),
                    index.getStringField( IXM_FIELD_NAME_NAME ) ) ;
            rc = SDB_IXM_REDEF ;
            goto error ;
         }
         else if ( sameName )
         {
            PD_LOG ( PDINFO, "Duplicate index name: %s",
                     index.getStringField( IXM_FIELD_NAME_NAME ) );
            rc = SDB_IXM_EXIST;
            goto error ;
         }
         else if ( curIdxCB.isSameDef( index ) )
         {
            PD_LOG ( PDERROR, "Duplicate index define: %s",
                     index.getStringField( IXM_FIELD_NAME_NAME ) );
            rc = SDB_IXM_EXIST_COVERD_ONE ;
            goto error ;
         }
         else
         {
            continue ;
         }
      }
      if ( DMS_COLLECTION_MAX_INDEX == indexID )
      {
         rc = SDB_DMS_MAX_INDEX ;
         goto error ;
      }

      {
         // initialize index control block, set flag to invalid
         ixmIndexCB indexCB ( extentID, index, context->mbID(), this,
                              context ) ;
         // verify the index control block is initialized
         if ( !indexCB.isInitialized() )
         {
            // if we can't initialize control block, we shouldn't call dropIndex
            // at the moment. So let's just reset _indexExtent to invalid and
            // free the extents
            PD_LOG ( PDERROR, "Failed to initialize index" ) ;
            rc = SDB_DMS_INIT_INDEX ;
            goto error ;
         }
         //set logical id
         indexLID = context->mb()->_indexHWCount ;
         indexCB.setLogicalID( indexLID ) ;

         // calc the reserve size
         if ( dpscb )
         {
            indexDef = indexCB.getDef().getOwned() ;
            _pDataSu->_clFullName( context->mb()->_collectionName, fullName,
                                   sizeof(fullName) ) ;
            rc = dpsIXCrt2Record( fullName, indexDef, record ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build record:%d", rc ) ;

            rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                         rc ) ;

            logRecSize = record.alignedLen() ;
            rc = pTransCB->reservedLogSpace( logRecSize, cb );
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                       logRecSize ) ;
               logRecSize = 0 ;
               goto error ;
            }
         }

         // initialize the root extent
         {
            // once the control block is allocated, let's do root extent
            ixmExtent idx( rootExtentID, context->mbID(), this ) ;
         }
         indexCB.setRoot ( rootExtentID ) ;

         if ( indexCB.unique() )
         {
            context->mbStat()->_uniqueIdxNum++ ;
         }
      }

      // change mb metadata
      context->mb()->_indexExtent[indexID] = extentID ;
      context->mb()->_numIndexes ++ ;
      context->mb()->_indexHWCount++ ;

      // log it
      if ( dpscb )
      {
         rc = _pDataSu->_logDPS( dpscb, info, cb, context,
                                 DMS_INVALID_EXTENT, TRUE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to insert ixcrt into log, rc = %d", rc ) ;
            goto error_after_create ;
         }
      }
      dropDps = dpscb ;

      // now we finished allocation part, let's get into build part
      rc = _rebuildIndex( context, extentID, cb, sortBufferSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build index[%s], rc = %d",
                 indexDef.toString().c_str(), rc ) ;
         goto error_after_create ;
      }

      rc = context->mbLock( EXCLUSIVE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to lock mb:%d", rc ) ;
         goto error_after_create ;
      }
      // if it is $oid, set DMS_MB_ATTR_NOIDINDEX with false
      if ( isSys && 0 == ossStrcmp( indexName, IXM_ID_KEY_NAME ) )
      {
         OSS_BIT_CLEAR( context->mb()->_attributes,
                        DMS_MB_ATTR_NOIDINDEX ) ;
      }

      /// creating index may cost long time. we mark file dirty again here.
      _registerNewWriting() ;
   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      return rc ;
   error :
      if ( DMS_INVALID_EXTENT != extentID )
      {
         releaseExtent ( extentID ) ;
      }
      if ( DMS_INVALID_EXTENT != rootExtentID )
      {
         releaseExtent ( rootExtentID ) ;
      }
      goto done ;
   error_after_create :
      rc1 = dropIndex ( context, indexName, cb, dropDps, isSys ) ;
      if ( rc1 )
      {
         PD_LOG ( PDERROR, "Failed to clean up invalid index, rc = %d", rc1 ) ;
      }
      goto done ;
   }

   INT32 _dmsStorageIndex::dropAllIndexes( dmsMBContext *context, pmdEDUCB *cb,
                                           SDB_DPSCB * dpscb )
   {
      INT32 rc = SDB_OK ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      while ( DMS_INVALID_EXTENT != context->mb()->_indexExtent[0] )
      {
         ixmIndexCB indexCB( context->mb()->_indexExtent[0], this, context ) ;
         rc = dropIndex( context, 0, indexCB.getLogicalID(), cb, dpscb, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop index[%d] failed, rc: %d", 0,
                      rc ) ;
      }
      context->mbStat()->_totalIndexPages = 0 ;
      context->mbStat()->_totalIndexFreeSpace = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageIndex::dropIndex( dmsMBContext *context, OID &indexOID,
                                      pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                      BOOLEAN isSys )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;
      OID oid ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d" ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_DROP_INDEX ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID],
                             this, context ) ;
         rc = indexCB.getIndexID ( oid ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get oid for %d index", indexID ) ;
            goto error ;
         }
         if ( indexOID == oid )
         {
            found = TRUE ;
            rc = dropIndex ( context, indexID, indexCB.getLogicalID(),
                             cb, dpscb, isSys ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to drop index %d:%s", context->mbID(),
                        indexOID.toString().c_str() ) ;
               goto error ;
            }
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::dropIndex( dmsMBContext *context,
                                      const CHAR *indexName,
                                      pmdEDUCB *cb, SDB_DPSCB * dpscb,
                                      BOOLEAN isSys )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_DROP_INDEX ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID], this,
                             context ) ;
         if ( 0 == ossStrncmp ( indexName, indexCB.getName(),
                                IXM_INDEX_NAME_SIZE ) )
         {
            found = TRUE ;
            rc = dropIndex ( context, indexID, indexCB.getLogicalID(),
                             cb, dpscb, isSys ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to drop index %d:%s", context->mbID(),
                        indexName ) ;
               goto error ;
            }
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::dropIndex( dmsMBContext *context, INT32 indexID,
                                      dmsExtentID indexLID, pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb, BOOLEAN isSys )
   {
      INT32 rc                     = SDB_OK ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      dpsTransCB *pTransCB         = pmdGetKRCB()->getTransCB() ;
      dpsMergeInfo info ;
      dpsLogRecord &record  = info.getMergeBlock().record() ;
      UINT32 logRecSize            = 0 ;
      BSONObj indexDef ;

      _registerNewWriting() ;
      rc = context->mbLock( EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to lock mb context[%s], rc: %d",
                 context->toString().c_str(), rc ) ;
         goto error ;
      }

      if ( indexID >= DMS_COLLECTION_MAX_INDEX )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }

      {
         // get index control block
         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         // verify the index control block is initialized
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "Failed to initialize index" ) ;
            rc = SDB_DMS_INIT_INDEX ;
            goto error ;
         }
         if ( indexLID != indexCB.getLogicalID() )
         {
            PD_LOG( PDERROR, "Index logical id not same, cur id: %d, "
                    "expect id: %d", indexLID, indexCB.getLogicalID() ) ;
            rc = SDB_IXM_NOTEXIST ;
            goto error ;
         }
         if ( IXM_INDEX_FLAG_NORMAL != indexCB.getFlag() &&
              IXM_INDEX_FLAG_INVALID != indexCB.getFlag() )
         {
            PD_LOG ( PDWARNING, "Index is either creating or dropping: %d",
                     (INT32)indexCB.getFlag() ) ;
         }
         if ( !isSys && 0 == ossStrcmp ( indexCB.getName(), IXM_ID_KEY_NAME ) )
         {
            PD_LOG ( PDERROR, "Cannot drop $id index" ) ;
            rc = SDB_IXM_DROP_ID ;
            goto error ;
         }
         if ( !isSys && 0 == ossStrcmp ( indexCB.getName(),
                                         IXM_SHARD_KEY_NAME ) )
         {
            PD_LOG ( PDERROR, "Cannot drop $shard index" ) ;
            rc = SDB_IXM_DROP_SHARD ;
            goto error ;
         }

         if ( isSys && 0 == ossStrcmp( indexCB.getName(),
                                       IXM_ID_KEY_NAME ) )
         {
            OSS_BIT_SET( context->mb()->_attributes,
                         DMS_MB_ATTR_NOIDINDEX ) ;
         }


         // reserved log-size
         if ( dpscb )
         {
            indexDef = indexCB.getDef().getOwned() ;
            _pDataSu->_clFullName( context->mb()->_collectionName, fullName,
                                   sizeof(fullName) ) ;
            rc = dpsIXDel2Record( fullName, indexDef, record ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

            rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                         rc ) ;

            logRecSize = record.alignedLen() ;
            rc = pTransCB->reservedLogSpace( logRecSize, cb );
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                       logRecSize ) ;
               logRecSize = 0 ;
               goto error ;
            }
         }

         // truncate index, do remove root
         rc = indexCB.truncate ( TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index, rc: %d", rc ) ;
            goto error ;
         }
         // truncate will set status back to normal, so we'll have to reset to
         // dropping again
         indexCB.setFlag ( IXM_INDEX_FLAG_DROPPING ) ;
         indexCB.clearLogicID() ;

         if ( indexCB.unique() )
         {
            context->mbStat()->_uniqueIdxNum-- ;
         }

         // release index control block extent
         rc = releaseExtent ( context->mb()->_indexExtent[indexID] ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to release indexCB extent: %d",
                     context->mb()->_indexExtent[indexID] ) ;
            indexCB.setFlag ( IXM_INDEX_FLAG_INVALID ) ;
            goto error ;
         }

         // copy back
         ossMemmove (&context->mb()->_indexExtent[indexID],
                     &context->mb()->_indexExtent[indexID+1],
                     sizeof(dmsExtentID)*(DMS_COLLECTION_MAX_INDEX-indexID-1));
         context->mb()->_indexExtent[DMS_COLLECTION_MAX_INDEX-1] =
            DMS_INVALID_EXTENT ;
      }
      context->mb()->_numIndexes -- ;

      // log it
      if ( dpscb )
      {
         rc = _pDataSu->_logDPS( dpscb, info, cb, context,
                                 DMS_INVALID_EXTENT, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert ixdel into log, "
                      "rc: %d", rc ) ;
      }

      _registerNewWriting() ;
   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::_rebuildIndex( dmsMBContext *context,
                                          dmsExtentID indexExtentID,
                                          pmdEDUCB * cb,
                                          INT32 sortBufferSize )
   {
      INT32 rc = SDB_OK ;
      dmsIndexBuilder* builder = NULL ;

      if ( sortBufferSize < 0 )
      {
         PD_LOG ( PDERROR, "invalid index sort buffer size" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( sortBufferSize > 0 && sortBufferSize < DMS_INDEX_SORT_BUFFER_MIN_SIZE )
      {
         sortBufferSize = DMS_INDEX_SORT_BUFFER_MIN_SIZE ;
      }

      builder = dmsIndexBuilder::createInstance( this, _pDataSu,
                                                 context, cb,
                                                 indexExtentID,
                                                 sortBufferSize ) ;
      if ( NULL == builder )
      {
         PD_LOG ( PDERROR, "Failed to get index builder instance, sort buffer size: %d",
                  sortBufferSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = builder->build() ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to build index: %d", rc ) ;
         goto error ;
      }

   done :
      if ( NULL != builder )
      {
         dmsIndexBuilder::releaseInstance( builder ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::rebuildIndexes( dmsMBContext *context, pmdEDUCB *cb,
                                           INT32 sortBufferSize )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      INT32 totalIndexNum          = 0 ;

      rc = truncateIndexes( context ) ;
      PD_RC_CHECK( rc, PDERROR, "truncate indexes failed, rc: %d", rc ) ;

      // need to lock mb
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ++totalIndexNum ;
      }

      PD_LOG ( PDEVENT, "Totally %d indexes to rebuild for collection %d",
               totalIndexNum, context->mbID() ) ;

      for ( indexID = 0 ; indexID < totalIndexNum ; ++indexID )
      {
         PD_LOG ( PDEVENT, "Rebuilding index %d for collection %d",
                  indexID, context->mbID() ) ;
         rc = _rebuildIndex ( context, context->mb()->_indexExtent[indexID],
                              cb, sortBufferSize ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to rebuild index %d, rc: %d", indexID,
                     rc ) ;
            goto error ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::_indexInsert( _ixmIndexCB *indexCB,
                                         const ixmKey &key,
                                         const dmsRecordID &rid,
                                         const Ordering& order,
                                         _pmdEDUCB *cb,
                                         BOOLEAN dupAllowed,
                                         BOOLEAN dropDups )
   {
      INT32 rc = SDB_OK ;
      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      // get root in each loop, since root page may change after each
      // insert (root split)
      ixmExtent rootidx ( indexCB->getRoot(), this ) ;

      rc = rootidx.insert ( key, rid, order, dupAllowed, indexCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to insert index, key[%s], rid[%d:%d], rc: %d",
                  key.toString( FALSE, TRUE ).c_str(), rid._extent,
                  rid._offset, rc ) ;
         goto error ;
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageIndex::_indexInsert( dmsMBContext *context,
                                         ixmIndexCB *indexCB,
                                         BSONObj &inputObj,
                                         const dmsRecordID &rid,
                                         pmdEDUCB *cb,
                                         BOOLEAN dupAllowed,
                                         BOOLEAN dropDups )
   {
      INT32 rc = SDB_OK ;
      BSONObjSet keySet ;

      SDB_ASSERT ( indexCB, "indexCB can't be NULL" ) ;

      rc = indexCB->getKeysFromObject ( inputObj, keySet ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get keys from object %s",
                    inputObj.toString().c_str() ) ;
/*
#if defined (_DEBUG)
      PD_LOG ( PDDEBUG, "IndexInsert\nIndex: %s\nRecord: %s",
               indexCB->keyPattern().toString().c_str(),
               inputObj.toString().c_str() ) ;
#endif*/

      {
         BSONObjSet::iterator it ;
         Ordering order = Ordering::make( indexCB->keyPattern() ) ;

         for ( it = keySet.begin() ; it != keySet.end() ; ++it )
         {/*
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key %s", (*it).toString().c_str() ) ;
#endif*/
            ixmKeyOwned ko ((*it)) ;

            rc = _indexInsert ( indexCB, ko, rid, order, cb, dupAllowed, dropDups ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to insert index, rc: %d", rc ) ;
               goto error ;
            }
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // caller is responsible to rollback the change if indexesInsert fail
   INT32 _dmsStorageIndex::indexesInsert( dmsMBContext *context,
                                          dmsExtentID extLID,
                                          BSONObj & inputObj,
                                          const dmsRecordID &rid,
                                          pmdEDUCB * cb )
   {
      INT32 rc                     = SDB_OK ;
      INT32 indexID                = 0 ;
      BOOLEAN unique               = FALSE ;
      BOOLEAN dropDups             = FALSE ;

      _registerNewWriting() ;
      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         PD_CHECK ( indexCB.isInitialized(), SDB_DMS_INIT_INDEX, error,
                    PDERROR, "Failed to init index" ) ;

         // if index is 'IXM_INDEX_FLAG_CREATING', then judge extent LID
         if ( IXM_INDEX_FLAG_CREATING == indexCB.getFlag() &&
              extLID > indexCB.scanExtLID() )
         {
            continue ;
         }
         // only attempt to insert into normal and creating indexes
         else if ( indexCB.getFlag() != IXM_INDEX_FLAG_NORMAL &&
                   indexCB.getFlag() != IXM_INDEX_FLAG_CREATING )
         {
            continue ;
         }
         unique = indexCB.unique() ;
         dropDups = indexCB.dropDups() ;
         rc = _indexInsert ( context, &indexCB, inputObj, rid, cb, !unique,
                             dropDups ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to insert index, rc: %d", rc ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::_indexUpdate( dmsMBContext *context,
                                         ixmIndexCB *indexCB,
                                         BSONObj &originalObj,
                                         BSONObj &newObj,
                                         const dmsRecordID &rid,
                                         pmdEDUCB *cb,
                                         BOOLEAN isRollback )
   {
      INT32 rc             = SDB_OK ;
      BSONObjSet keySetOri ;
      BSONObjSet keySetNew ;
      BOOLEAN unique       = FALSE ;
      BOOLEAN found        = FALSE ;
      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      SDB_ASSERT ( indexCB, "indexCB can't be NULL" ) ;

      rc = indexCB->getKeysFromObject ( originalObj, keySetOri ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get keys from org object %s",
                  originalObj.toString().c_str() ) ;
         goto error ;
      }

      unique = indexCB->unique() ;

      rc = indexCB->getKeysFromObject ( newObj, keySetNew ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get keys from new object %s",
                  newObj.toString().c_str() ) ;
         goto error ;
      }

#if defined (_DEBUG)
      PD_LOG ( PDDEBUG, "IndexUpdate\nIndex: %s\nFrom Record: %s\nTo Record %s",
               indexCB->keyPattern().toString().c_str(),
               originalObj.toString().c_str(),
               newObj.toString().c_str() ) ;
#endif

      // do merge scan for two sets, unindex the keys if the one in keySetOri
      // doesn't appear in keySetNew, and insert the one in keySetNew doesn't
      // appear in keySetOri
      {
         BSONObjSet::iterator itori ;
         BSONObjSet::iterator itnew ;
         Ordering order = Ordering::make(indexCB->keyPattern()) ;
         itori = keySetOri.begin() ;
         itnew = keySetNew.begin() ;
         while ( keySetOri.end() != itori && keySetNew.end() != itnew )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key From %s\nKey To %s",
                     (*itori).toString().c_str(),
                     (*itnew).toString().c_str() ) ;
#endif
            INT32 result = (*itori).woCompare((*itnew), BSONObj(), FALSE ) ;
            if ( 0 == result )
            {
               // new and original are the same, we don't need to change
               // anything in the index
               itori++ ;
               itnew++ ;
               continue ;
            }
            else if ( result < 0 )
            {
               // original smaller than new, that means the original doesn't
               // appear in the new list anymore, let's delete it
               ixmExtent rootidx ( indexCB->getRoot(), this ) ;
               ixmKeyOwned ko ((*itori)) ;
               rc = rootidx.unindex ( ko, rid, order, indexCB, found ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Failed to unindex, rc: %d", rc ) ;
                  goto error ;
               }
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
               // during rollback, since the previous change may half-way
               // completed, there could be some keys that has not been
               // inserted. So if we found any rid+key that does not in the
               // index, that means we've finished rollback
               if ( !found && isRollback )
               {
                  goto done ;
               }
               itori++ ;
               continue ;
            }
            else
            {
               // new smaller than original, that means the new one doesn't
               // appear in the original list, let's add it
               ixmExtent rootidx ( indexCB->getRoot(), this ) ;
               ixmKeyOwned ko ((*itnew)) ;
               rc = rootidx.insert ( ko, rid, order, !unique, indexCB ) ;
               if ( rc )
               {
                  // during rollback, since the previous change may half-way
                  // completed, there could be some keys that has not been
                  // removed. So if we hit error indicating the key and rid are
                  // identical, that means we've finished rollback
                  if ( SDB_IXM_IDENTICAL_KEY == rc && isRollback )
                  {
                     rc = SDB_OK ;
                     goto done ;
                  }
                  PD_LOG ( PDERROR, "Failed to insert index, rc: %d", rc ) ;
                  goto error ;
               }
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
               itnew++ ;
               continue ;
            }
         }

         // delete reset of itori
         while ( keySetOri.end() != itori )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key From %s", (*itori).toString().c_str() ) ;
#endif
            ixmExtent rootidx ( indexCB->getRoot(), this ) ;
            ixmKeyOwned ko ((*itori)) ;
            rc = rootidx.unindex ( ko, rid, order, indexCB, found ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to unindex, rc: %d", rc ) ;
               goto error ;
            }
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
            // during rollback, since the previous change may half-way
            // completed, there could be some keys that has not been
            // inserted. So if we found any rid+key that does not in the
            // index, that means we've finished rollback
            if ( !found && isRollback )
            {
               goto done ;
            }
            itori++ ;
         }

         // insert rest of itnew
         while ( keySetNew.end() != itnew )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key To %s", (*itnew).toString().c_str() ) ;
#endif
            ixmExtent rootidx ( indexCB->getRoot(), this ) ;
            ixmKeyOwned ko ((*itnew)) ;
            rc = rootidx.insert ( ko, rid, order, !unique, indexCB ) ;
            if ( rc )
            {
               // during rollback, since the previous change may half-way
               // completed, there could be some keys that has not been
               // removed. So if we hit error indicating the key and rid are
               // identical, that means we've finished rollback
               if ( SDB_IXM_IDENTICAL_KEY == rc && isRollback )
               {
                  rc = SDB_OK ;
                  goto done ;
               }
               PD_LOG ( PDERROR, "Failed to insert index, rc: %d", rc ) ;
               goto error ;
            }
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
            itnew++ ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // caller is responsible to rollback the change
   INT32 _dmsStorageIndex::indexesUpdate( dmsMBContext *context,
                                          dmsExtentID extLID,
                                          BSONObj &originalObj,
                                          BSONObj &newObj,
                                          const dmsRecordID &rid,
                                          pmdEDUCB *cb,
                                          BOOLEAN isRollback )
   {
      INT32 rc                     = SDB_OK ;
      INT32 indexID                = 0 ;

      _registerNewWriting() ;
      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      for ( indexID=0; indexID<DMS_COLLECTION_MAX_INDEX; indexID++ )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         PD_CHECK ( indexCB.isInitialized(), SDB_DMS_INIT_INDEX,
                    error, PDERROR, "Failed to init index" ) ;

         if ( IXM_INDEX_FLAG_CREATING == indexCB.getFlag() &&
              extLID > indexCB.scanExtLID() )
         {
            continue ;
         }
         // only attempt to insert into normal and creating indexes
         else if ( indexCB.getFlag() != IXM_INDEX_FLAG_NORMAL &&
                   indexCB.getFlag() != IXM_INDEX_FLAG_CREATING )
         {
            continue ;
         }
         rc = _indexUpdate ( context, &indexCB, originalObj, newObj, rid, cb,
                             isRollback ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to update index, rc: %d", rc ) ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::_indexDelete( dmsMBContext *context,
                                         ixmIndexCB *indexCB,
                                         BSONObj &inputObj,
                                         const dmsRecordID &rid,
                                         pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BSONObjSet keySet ;
      BOOLEAN result = FALSE ;
      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      SDB_ASSERT ( indexCB, "indexCB can't be NULL" ) ;

      rc = indexCB->getKeysFromObject ( inputObj, keySet ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get keys from object %s",
                  inputObj.toString().c_str() ) ;
         goto error ;
      }

#if defined (_DEBUG)
      PD_LOG ( PDDEBUG, "IndexDelete\nIndex: %s\nRecord: %s",
               indexCB->keyPattern().toString().c_str(),
               inputObj.toString().c_str() ) ;
#endif

      {
         BSONObjSet::iterator it ;
         Ordering order = Ordering::make(indexCB->keyPattern()) ;
         for ( it = keySet.begin() ; it != keySet.end() ; ++it )
         {

#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key %s", (*it).toString().c_str() ) ;
#endif

            // get root in each loop, since root page may change after each
            // insert (root split)
            ixmExtent rootidx ( indexCB->getRoot(), this ) ;
            ixmKeyOwned ko ((*it)) ;
            rc = rootidx.unindex ( ko, rid, order, indexCB, result ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to unindex, rc: %d", rc ) ;
               goto error ;
            }
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::indexesDelete( dmsMBContext *context,
                                          dmsExtentID extLID,
                                          BSONObj &inputObj,
                                          const dmsRecordID &rid,
                                          pmdEDUCB * cb )
   {
      INT32 rc                     = SDB_OK ;
      INT32 indexID                = 0 ;

      _registerNewWriting() ;
      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "Failed to init index" ) ;
            rc = SDB_DMS_INIT_INDEX ;
            goto error ;
         }
         if ( IXM_INDEX_FLAG_CREATING == indexCB.getFlag() &&
              extLID > indexCB.scanExtLID() )
         {
            continue ;
         }
         // only attempt to insert into normal and creating indexes
         else if ( indexCB.getFlag() != IXM_INDEX_FLAG_NORMAL &&
                   indexCB.getFlag() != IXM_INDEX_FLAG_CREATING )
         {
            continue ;
         }
         rc = _indexDelete ( context, &indexCB, inputObj, rid, cb ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to delete index, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::truncateIndexes( dmsMBContext * context )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;

      _registerNewWriting() ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "Failed to initialize index" ) ;
            rc = SDB_DMS_INIT_INDEX;
            goto error ;
         }

         // we don't check index flag since we are doing full index rebuild now
         // truncate index, do remove root
         rc = indexCB.truncate ( FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index, rc: %d", rc ) ;
            goto error ;
         }
      }

      context->mbStat()->_totalIndexPages = indexID << 1 ;
      context->mbStat()->_totalIndexFreeSpace =
      indexID * ( pageSize()-1-sizeof(ixmExtentHead) ) ;

      _registerNewWriting() ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::getIndexCBExtent( dmsMBContext *context,
                                             const CHAR *indexName,
                                             dmsExtentID &indexExtent )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;
      BOOLEAN hasLocked            = FALSE ;

      SDB_ASSERT ( indexName, "index name can't be NULL" ) ;

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         hasLocked = TRUE ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID], this,
                             context ) ;
         if ( 0 == ossStrncmp ( indexName, indexCB.getName(),
                                IXM_INDEX_NAME_SIZE ) )
         {
            indexExtent = context->mb()->_indexExtent[indexID] ;
            found = TRUE ;
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
      }

   done :
      if ( hasLocked )
      {
         context->mbUnlock() ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::getIndexCBExtent( dmsMBContext *context,
                                             const OID &indexOID,
                                             dmsExtentID &indexExtent )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;
      BOOLEAN hasLocked            = FALSE ;

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         hasLocked = TRUE ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID], this,
                             context ) ;
         OID id ;
         indexCB.getIndexID( id ) ;
         if ( indexOID == id )
         {
            indexExtent = context->mb()->_indexExtent[indexID] ;
            found = TRUE ;
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
      }

   done :
      if ( hasLocked )
      {
         context->mbUnlock() ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::getIndexCBExtent( dmsMBContext *context,
                                             INT32 indexID,
                                             dmsExtentID &indexExtent )
   {
      INT32 rc                      = SDB_OK ;
      BOOLEAN hasLocked             = FALSE ;

      if ( indexID >= DMS_COLLECTION_MAX_INDEX )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         hasLocked = FALSE ;
      }

      if ( context->mb()->_indexExtent[indexID] == DMS_INVALID_EXTENT )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }
      indexExtent = context->mb()->_indexExtent[indexID] ;

   done:
      if ( hasLocked )
      {
         context->mbUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _dmsStorageIndex::addStatFreeSpace( UINT16 mbID, UINT16 size )
   {
      if ( mbID < DMS_MME_SLOTS && _pDataSu )
      {
         _pDataSu->_mbStatInfo[mbID]._totalIndexFreeSpace += size ;
      }
   }

   void _dmsStorageIndex::decStatFreeSpace( UINT16 mbID, UINT16 size )
   {
      if ( mbID < DMS_MME_SLOTS && _pDataSu )
      {
         _pDataSu->_mbStatInfo[mbID]._totalIndexFreeSpace -= size ;
      }
   }

   INT32 _dmsStorageIndex::tryToFlush( BOOLEAN ignoreTick, BOOLEAN &failed )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      if ( !ignoreTick && !_noWriteForAWhile() )
      {
         failed = TRUE ;
         goto done ;
      }

      _validFlag = 1 ;

      rc = flushAll( TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to flush dirty segments:%d", rc ) ;
         goto error ;
      }

      if ( 0 ==_validFlag )
      {
         failed = TRUE ;
      }
      else
      {
         ossLatch ( &_pagecleanerLatch ) ;
         locked = TRUE ;
         if ( 0 ==_validFlag )
         {
            failed = TRUE ;
            goto done ;
         }

         _markHeaderValid() ;
         failed = FALSE ;
      }
   done:
      if ( locked )
      {
         ossUnlatch( &_pagecleanerLatch ) ;
      }
      return rc ;
   error:
      goto done ;
   }
}


