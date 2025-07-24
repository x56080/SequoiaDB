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

   Source File Name = dmsStorageUnit.cpp

   Descriptive Name = Data Management Service Storage Unit

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data insert/update/delete. This file does NOT include index logic.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsStorageUnit.hpp"
#include "dmsScanner.hpp"
#include "mthModifier.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsStorageLob.hpp"
#include "pmdStartup.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU, "_dmsStorageUnit::_dmsStorageUnit" )
   _dmsStorageUnit::_dmsStorageUnit ( const CHAR *pSUName,
                                      UINT32 sequence,
                                      utilCacheMgr *pMgr,
                                      INT32 pageSize,
                                      INT32 lobPageSize )
   :_apm(this),
    _pDataSu( NULL ),
    _pIndexSu( NULL ),
    _pLobSu( NULL ),
    _pMgr( pMgr ),
    _pCacheUnit( NULL )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU ) ;
      SDB_ASSERT ( pSUName, "name can't be null" ) ;

      pmdOptionsCB *options = pmdGetOptionCB() ;

      if ( 0 == pageSize )
      {
         pageSize = DMS_PAGE_SIZE_DFT ;
      }

      if ( 0 == lobPageSize )
      {
         lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
      }

      CHAR dataFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;
      CHAR idxFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;

      _storageInfo._pageSize = pageSize ;
      _storageInfo._lobdPageSize = lobPageSize ;
      ossStrncpy( _storageInfo._suName, pSUName, DMS_SU_NAME_SZ ) ;
      _storageInfo._suName[DMS_SU_NAME_SZ] = 0 ;
      _storageInfo._sequence = sequence ;
      _storageInfo._overflowRatio = options->getOverFlowRatio() ;
      _storageInfo._extentThreshold = options->getExtendThreshold() << 20 ;
      _storageInfo._enableSparse = options->sparseFile() ;
      _storageInfo._directIO = options->useDirectIOInLob() ;
      _storageInfo._cacheMergeSize = options->getCacheMergeSize() ;
      _storageInfo._pageAllocTimeout = options->getPageAllocTimeout() ;
      _storageInfo._dataIsOK = pmdGetStartup().isOK() ;
      _storageInfo._curLSNOnStart = pmdGetSyncMgr()->getCompleteLSN() ;
      // make secret value
      _storageInfo._secretValue = ossPack32To64( (UINT32)time(NULL),
                                                 (UINT32)(ossRand()*239641) ) ;

      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, sequence, DMS_DATA_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   _storageInfo._suName, sequence, DMS_INDEX_SU_EXT_NAME ) ;

      _pDataSu = SDB_OSS_NEW dmsStorageData( dataFileName,
                                             &_storageInfo ) ;
      if ( _pDataSu )
      {
         _pIndexSu = SDB_OSS_NEW dmsStorageIndex( idxFileName, &_storageInfo,
                                                  _pDataSu ) ;
      }

      /// alloc cache unit
      _pCacheUnit = SDB_OSS_NEW utilCacheUnit( _pMgr ) ;

      if ( NULL != _pDataSu && NULL != _pIndexSu && NULL != _pCacheUnit )
      {
         /// reuse buf for lob
         ossMemset( dataFileName, 0, sizeof( dataFileName ) ) ;
         ossMemset( idxFileName, 0 , sizeof( idxFileName ) ) ;
         ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                      _storageInfo._suName, _storageInfo._sequence,
                      DMS_LOB_META_SU_EXT_NAME ) ;
         ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                      _storageInfo._suName, _storageInfo._sequence,
                      DMS_LOB_DATA_SU_EXT_NAME ) ;

         _pLobSu = SDB_OSS_NEW dmsStorageLob( dataFileName, idxFileName,
                                              &_storageInfo, _pDataSu,
                                              _pCacheUnit ) ;
      }

      PD_TRACE_EXIT ( SDB__DMSSU ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DESC, "_dmsStorageUnit::~_dmsStorageUnit" )
   _dmsStorageUnit::~_dmsStorageUnit()
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DESC ) ;
      close() ;

      if ( _pIndexSu )
      {
         SDB_OSS_DEL _pIndexSu ;
         _pIndexSu = NULL ;
      }
      if ( _pLobSu )
      {
         SDB_OSS_DEL _pLobSu ;
         _pLobSu = NULL ;
      }
      if ( _pCacheUnit )
      {
         SDB_OSS_DEL _pCacheUnit ;
         _pCacheUnit = NULL ;
      }
      // _pDataSu must be delete at the last
      if ( _pDataSu )
      {
         SDB_OSS_DEL _pDataSu ;
         _pDataSu = NULL ;
      }
      PD_TRACE_EXIT ( SDB__DMSSU_DESC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_OPEN, "_dmsStorageUnit::open" )
   INT32 _dmsStorageUnit::open( const CHAR *pDataPath,
                                const CHAR *pIndexPath,
                                const CHAR *pLobPath,
                                const CHAR *pLobMetaPath,
                                IDataSyncManager *pSyncMgr,
                                BOOLEAN createNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_OPEN ) ;
      if ( !_pDataSu || !_pIndexSu || !_pLobSu || !_pCacheUnit )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory failed" ) ;
         goto error ;
      }

      // open data
      rc = _pDataSu->openStorage( pDataPath, pSyncMgr, createNew ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open storage data su failed, rc: %d", rc ) ;
         if ( createNew && SDB_FE != rc )
         {
            goto rmdata ;
         }
         goto error ;
      }

      // open index
      rc = _pIndexSu->openStorage( pIndexPath, pSyncMgr, createNew ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Open storage index su failed, rc: %d", rc ) ;
         if ( createNew )
         {
            if ( SDB_FE != rc )
            {
               goto rmboth ;
            }
            goto rmdata ;
         }
         else if ( SDB_FNE == rc &&
                   _pDataSu->isCrashed() &&
                   0 == _pDataSu->getCollectionNum() )
         {
            /// create data file then crashed, so clean the data file
            goto rmdata ;
         }
         goto error ;
      }

      // open lob
      rc = _pLobSu->open( pLobPath, pLobMetaPath, pSyncMgr, createNew ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open storage lob, rc:%d", rc ) ;
         if ( createNew )
         {
            goto rmboth ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_OPEN, rc ) ;
      return rc ;
   error:
      close() ;
      goto done ;
   rmdata:
      {
         INT32 rcTmp = _pDataSu->removeStorage() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Failed to remove cs data file[%s] in "
                    "rollback, rc: %d", _pDataSu->getSuFileName(), rc ) ;
         }
      }
      goto done ;
   rmboth:
      {
         INT32 rcTmp = _pIndexSu->removeStorage() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Failed to remove cs idnex file[%s] in "
                    "rollback, rc: %d", _pIndexSu->getSuFileName(), rc ) ;
         }
      }
      goto rmdata ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CLOSE, "_dmsStorageUnit::close" )
   void _dmsStorageUnit::close()
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_CLOSE ) ;
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      /// The order is:
      /// cacheUnit -> lob -> index -> data( must be in last )
      if ( _pCacheUnit )
      {
         _pCacheUnit->fini( cb ) ;
      }
      if ( _pLobSu )
      {
         _pLobSu->closeStorage() ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->closeStorage() ;
      }
      if ( _pDataSu )
      {
         _pDataSu->closeStorage() ;
      }
      PD_TRACE_EXIT ( SDB__DMSSU_CLOSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_REMOVE, "_dmsStorageUnit::remove" )
   INT32 _dmsStorageUnit::remove ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_REMOVE ) ;

      /// The order is:
      /// cacheUnit -> lob -> index -> data( must be in last )

      if ( _pCacheUnit )
      {
         _pCacheUnit->dropDirty() ;
      }

      if ( _pLobSu )
      {
         _pLobSu->removeStorageFiles() ;
      }

      if ( _pIndexSu )
      {
         /// first clear all page map
         _pIndexSu->getPageMapUnit()->clear() ;

         rc = _pIndexSu->removeStorage() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove collection space[%s] "
                      "index file, rc: %d", CSName(), rc ) ;
      }

      if ( _pDataSu )
      {
         rc = _pDataSu->removeStorage() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove collection space[%s] "
                      "data file, rc: %d", CSName(), rc ) ;
      }

      PD_LOG( PDEVENT, "Remove collection space[%s] files succeed", CSName() ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_REMOVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_RENAMECS, "_dmsStorageUnit::renameCS" )
   INT32 _dmsStorageUnit::renameCS( const CHAR *pNewName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_RENAMECS ) ;

      CHAR dataFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;
      CHAR idxFileName[DMS_SU_FILENAME_SZ + 1] = {0} ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu || !_pCacheUnit )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc memory failed" ) ;
         goto error ;
      }

      /// data and index
      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_DATA_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_INDEX_SU_EXT_NAME ) ;

      rc = _pDataSu->renameStorage( pNewName, dataFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename storage data failed, rc: %d", rc ) ;
         goto error ;
      }
      rc = _pIndexSu->renameStorage( pNewName, idxFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename storage index failed, rc: %d", rc ) ;
         goto error ;
      }

      /// lobm and lobd
      ossMemset( dataFileName, 0, sizeof( dataFileName ) ) ;
      ossMemset( idxFileName, 0 , sizeof( idxFileName ) ) ;
      ossSnprintf( dataFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_LOB_META_SU_EXT_NAME ) ;
      ossSnprintf( idxFileName, DMS_SU_FILENAME_SZ, "%s.%d.%s",
                   pNewName, _storageInfo._sequence,
                   DMS_LOB_DATA_SU_EXT_NAME ) ;

      rc = _pLobSu->rename( pNewName, dataFileName, idxFileName ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Rename storage lob failed, rc: %d", rc ) ;
         goto error ;
      }

      /// update storage info
      ossStrncpy( _storageInfo._suName, pNewName, DMS_SU_NAME_SZ ) ;
      _storageInfo._suName[DMS_SU_NAME_SZ] = 0 ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_RENAMECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU__RESETCOLLECTION, "_dmsStorageUnit::_resetCollection" )
   INT32 _dmsStorageUnit::_resetCollection( dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU__RESETCOLLECTION ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;

      // drop all indexes
      rc = _pIndexSu->dropAllIndexes( context, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Drop all indexes failed, rc: %d", rc ) ;
         // don't go to error, continue
      }

      rc = _pDataSu->_truncateCollection( context ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Truncate collection data failed, rc: %d", rc ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU__RESETCOLLECTION, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_LDEXTA, "_dmsStorageUnit::loadExtentA" )
   INT32 _dmsStorageUnit::loadExtentA ( dmsMBContext *mbContext,
                                        const CHAR *pBuffer,
                                        UINT16 numPages,
                                        const BOOLEAN toLoad,
                                        SINT32 *tAllocatedExtent )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_LDEXTA ) ;

      dmsExtRW extRW ;
      dmsExtent *sourceExt  = (dmsExtent*)pBuffer ;
      dmsExtent *extAddr = NULL ;
      SINT32 allocatedExtent = DMS_INVALID_EXTENT ;

      // allocate a new extent
      rc = _pDataSu->_allocateExtent( mbContext, numPages, FALSE, toLoad,
                                      &allocatedExtent ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Can't allocate extent for %d pages, rc = %d",
                  numPages, rc ) ;
         goto error ;
      }

      extRW = _pDataSu->extent2RW( allocatedExtent, mbContext->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      // get the address
      extAddr = extRW.writePtr<dmsExtent>( 0, getPageSize() * numPages ) ;
      if ( !extAddr )
      {
         PD_LOG( PDERROR, "Get extent[%d] write address failed",
                 allocatedExtent ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      // copy data part
      ossMemcpy ( &((CHAR*)extAddr)[DMS_EXTENT_METADATA_SZ],
                  &pBuffer[DMS_EXTENT_METADATA_SZ],
                  _pDataSu->pageSize() * numPages  - DMS_EXTENT_METADATA_SZ ) ;

      // reset header part
      extAddr->_recCount          = sourceExt->_recCount ;
      extAddr->_firstRecordOffset = sourceExt->_firstRecordOffset ;
      extAddr->_lastRecordOffset  = sourceExt->_lastRecordOffset ;
      extAddr->_freeSpace         = sourceExt->_freeSpace ;

      if ( tAllocatedExtent )
      {
         *tAllocatedExtent = allocatedExtent ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSU_LDEXTA, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_LDEXT, "_dmsStorageUnit::loadExtent" )
   INT32 _dmsStorageUnit::loadExtent ( dmsMBContext *mbContext,
                                       const CHAR *pBuffer,
                                       UINT16 numPages )
   {
      INT32 rc                 = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSU_LDEXT ) ;

      dmsExtRW extRW ;
      SINT32 allocatedExtent   = DMS_INVALID_EXTENT ;
      dmsExtent *extAddr       = NULL ;
      SDB_ASSERT ( pBuffer, "buffer can't be NULL" ) ;

      rc = loadExtentA ( mbContext, pBuffer, numPages, FALSE,
                         &allocatedExtent ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to loadExtentA, rc = %d",
                  numPages, rc ) ;
         goto error ;
      }

      extRW = _pDataSu->extent2RW( allocatedExtent, mbContext->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.writePtr<dmsExtent>() ;
      if ( !extAddr )
      {
         PD_LOG( PDERROR, "Get extent[%d] write address failed",
                 allocatedExtent ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      // reset delete list
      _pDataSu->_mapExtent2DelList( mbContext->mb(), extAddr,
                                    allocatedExtent ) ;
      // add count
      addExtentRecordCount( mbContext->mb(), extAddr->_recCount ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSU_LDEXT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_INSERTRECORD, "_dmsStorageUnit::insertRecord" )
   INT32 _dmsStorageUnit::insertRecord ( const CHAR *pName,
                                         BSONObj &record,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpscb,
                                         BOOLEAN mustOID,
                                         BOOLEAN canUnLock,
                                         dmsMBContext *context,
                                         utilInsertResult *insertResult )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_INSERTRECORD ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pDataSu->insertRecord( context, record, cb, dpscb, mustOID,
                                   canUnLock, insertResult ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_INSERTRECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_UPDATERECORDS, "_dmsStorageUnit::updateRecords" )
   INT32 _dmsStorageUnit::updateRecords ( const CHAR *pName,
                                          pmdEDUCB *cb,
                                          SDB_DPSCB *dpscb,
                                          _mthMatchTree *matcher,
                                          mthModifier &modifier,
                                          SINT64 &numRecords,
                                          SINT64 maxUpdate,
                                          dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_UPDATERECORDS ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      {
         _mthRecordGenerator generator ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr = 0 ;
         numRecords = 0 ;
         dmsTBScanner tbScanner( _pDataSu, context, matcher,
                                 DMS_ACCESS_TYPE_UPDATE, maxUpdate ) ;
         while ( SDB_OK == ( rc = tbScanner.advance( recordID, generator,
                                                     cb ) ) )
         {
            generator.getDataPtr( recordDataPtr ) ;
            rc = _pDataSu->updateRecord( context, recordID, recordDataPtr, cb,
                                         dpscb, modifier ) ;
            PD_RC_CHECK( rc, PDERROR, "Update record failed, rc: %d", rc ) ;

            ++numRecords ;
         }

         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get next record, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_UPDATERECORDS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DELETERECORDS, "_dmsStorageUnit::deleteRecords" )
   INT32 _dmsStorageUnit::deleteRecords ( const CHAR *pName,
                                          pmdEDUCB * cb,
                                          SDB_DPSCB *dpscb,
                                          _mthMatchTree *matcher,
                                          SINT64 &numRecords,
                                          SINT64 maxDelete,
                                          dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_DELETERECORDS ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      {
         _mthRecordGenerator generator ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr = 0 ;
         numRecords = 0 ;
         dmsTBScanner tbScanner( _pDataSu, context, matcher,
                                 DMS_ACCESS_TYPE_DELETE, maxDelete ) ;
         while ( SDB_OK == ( rc = tbScanner.advance( recordID, generator,
                                                     cb ) ) )
         {
            generator.getDataPtr( recordDataPtr ) ;
            rc = _pDataSu->deleteRecord( context, recordID, recordDataPtr,
                                         cb, dpscb ) ;
            PD_RC_CHECK( rc, PDERROR, "Delete record failed, rc: %d", rc ) ;

            ++numRecords ;
         }

         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get next record, rc: %d", rc ) ;
            goto error ;
         }
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DELETERECORDS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_REBUILDINDEXES, "_dmsStorageUnit::rebuildIndexes" )
   INT32 _dmsStorageUnit::rebuildIndexes( const CHAR *pName,
                                          pmdEDUCB * cb,
                                          dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_REBUILDINDEXES ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->rebuildIndexes( context, cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_REBUILDINDEXES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CREATEINDEX, "_dmsStorageUnit::createIndex" )
   INT32 _dmsStorageUnit::createIndex( const CHAR *pName, const BSONObj &index,
                                       pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                       BOOLEAN isSys, dmsMBContext * context,
                                       INT32 sortBufferSize )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSSU_CREATEINDEX ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->createIndex( context, index, cb, dpscb, isSys, sortBufferSize ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_CREATEINDEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DROPINDEX, "_dmsStorageUnit::dropIndex" )
   INT32 _dmsStorageUnit::dropIndex( const CHAR *pName, const CHAR *indexName,
                                     pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                     BOOLEAN isSys, dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DROPINDEX ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->dropIndex( context, indexName, cb, dpscb, isSys ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DROPINDEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DROPINDEX1, "_dmsStorageUnit::dropIndex" )
   INT32 _dmsStorageUnit::dropIndex( const CHAR *pName, OID &indexOID,
                                     pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                     BOOLEAN isSys, dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DROPINDEX1 ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _pIndexSu->dropIndex( context, indexOID, cb, dpscb, isSys ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_DROPINDEX1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_COUNTCOLLECTION, "_dmsStorageUnit::countCollection" )
   INT32 _dmsStorageUnit::countCollection ( const CHAR *pName,
                                            INT64 &recordNum,
                                            pmdEDUCB *cb,
                                            dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      //dmsExtent *pExtent           = NULL ;
      recordNum                    = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSU_COUNTCOLLECTION ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }

      /*{
         dmsExtentItr itr( _pDataSu, context ) ;
         while ( SDB_OK == ( rc = itr.next( &pExtent, cb ) ) )
         {
            recordNum += pExtent->_recCount ;
         }
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
      }*/
      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to lock dms mb context[%s], rc: %d",
                      context->toString().c_str(), rc ) ;
      }
      recordNum = context->mbStat()->_totalRecords ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_COUNTCOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETCOLLECTIONFLAG, "_dmsStorageUnit::getCollectionFlag" )
   INT32 _dmsStorageUnit::getCollectionFlag( const CHAR *pName, UINT16 &flag,
                                             dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETCOLLECTIONFLAG ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      flag = context->mb()->_flag ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETCOLLECTIONFLAG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_CHANGECOLLECTIONFLAG, "_dmsStorageUnit::changeCollectionFlag" )
   INT32 _dmsStorageUnit::changeCollectionFlag( const CHAR *pName, UINT16 flag,
                                                dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_CHANGECOLLECTIONFLAG ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      context->mb()->_flag = flag ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_CHANGECOLLECTIONFLAG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETCOLLECTIONATTRIBUTES, "_dmsStorageUnit::getCollectionAttributes" )
   INT32 _dmsStorageUnit::getCollectionAttributes( const CHAR *pName,
                                                   UINT32 &attributes,
                                                   dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETCOLLECTIONATTRIBUTES ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      attributes = context->mb()->_attributes ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETCOLLECTIONATTRIBUTES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_UPDATECOLLECTIONATTRIBUTES, "_dmsStorageUnit::updateCollectionAttributes" )
   INT32 _dmsStorageUnit::updateCollectionAttributes( const CHAR *pName,
                                                      UINT32 newAttributes,
                                                      dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSU_UPDATECOLLECTIONATTRIBUTES ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      context->mb()->_attributes = newAttributes ;

   done :
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_UPDATECOLLECTIONATTRIBUTES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageUnit::getCollectionCompType( const CHAR *pName,
                                                 UTIL_COMPRESSOR_TYPE &compType,
                                                 dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN getContext = FALSE ;

      if ( !context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;
         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Lock collection failed, rc: %d", rc ) ;
      }

      compType = (UTIL_COMPRESSOR_TYPE)context->mb()->_compressorType ;
   done:
      if ( getContext && context )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETSEGEXTENTS, "_dmsStorageUnit::getSegExtents" )
   INT32 _dmsStorageUnit::getSegExtents( const CHAR *pName,
                                         vector < dmsExtentID > &segExtents,
                                         dmsMBContext *context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      const dmsMBEx *mbEx          = NULL ;
      dmsExtentID firstID          = DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETSEGEXTENTS ) ;
      segExtents.clear() ;

      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      if ( DMS_INVALID_EXTENT == context->mb()->_mbExExtentID )
      {
         PD_LOG( PDERROR, "Invalid meta extent id: %d, collection name: %s",
                 context->mb()->_mbExExtentID,
                 context->mb()->_collectionName ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      extRW = _pDataSu->extent2RW( context->mb()->_mbExExtentID,
                                   context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      mbEx = extRW.readPtr<dmsMBEx>() ;
      if ( mbEx )
      {
         mbEx = extRW.readPtr<dmsMBEx>( 0, (UINT32)mbEx->_header._blockSize <<
                                           _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( !mbEx )
      {
         PD_LOG( PDERROR, "Get extent[%d] read address failed",
                 context->mb()->_mbExExtentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < mbEx->_header._segNum ; ++i )
      {
         mbEx->getFirstExtentID( i, firstID ) ;
         if ( DMS_INVALID_EXTENT != firstID )
         {
            segExtents.push_back( firstID ) ;
         }
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETSEGEXTENTS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETINDEXES, "_dmsStorageUnit::getIndexes" )
   INT32 _dmsStorageUnit::getIndexes( const CHAR *pName,
                                      vector< _monIndex > &resultIndexes,
                                      dmsMBContext * context )
   {
      INT32 rc                     = SDB_OK ;
      BOOLEAN getContext           = FALSE ;
      UINT32 indexID               = 0 ;
      monIndex indexItem ;
      PD_TRACE_ENTRY ( SDB__DMSSU_GETINDEXES ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID],
                              _pIndexSu, NULL ) ;
         indexItem._indexFlag = indexCB.getFlag () ;
         indexItem._scanExtLID = indexCB.scanExtLID () ;
         indexItem._version = indexCB.version () ;
         // copy the index def to it's owned buffer
         indexItem._indexDef = indexCB.getDef().copy () ;
         // add
         resultIndexes.push_back ( indexItem ) ;
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETINDEXES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETINDEX, "_dmsStorageUnit::getIndex" )
   INT32 _dmsStorageUnit::getIndex( const CHAR *pName,
                                    const CHAR *pIndexName,
                                    _monIndex &resultIndex,
                                    dmsMBContext *context )
   {
      INT32 rc                     = SDB_IXM_NOTEXIST ;
      BOOLEAN getContext           = FALSE ;
      UINT32 indexID               = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSU_GETINDEX ) ;
      SDB_ASSERT( pIndexName, "Index name can't be NULL" ) ;

      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name can't be NULL" ) ;

         rc = _pDataSu->getMBContext( &context, pName, SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "Get collection[%s] mb context failed, "
                      "rc: %d", pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID],
                              _pIndexSu, NULL ) ;
         if ( 0 == ossStrcmp( indexCB.getName(), pIndexName ) )
         {
            resultIndex._indexFlag = indexCB.getFlag () ;
            resultIndex._scanExtLID = indexCB.scanExtLID () ;
            resultIndex._version = indexCB.version () ;
            // copy the index def to it's owned buffer
            resultIndex._indexDef = indexCB.getDef().copy () ;

            rc = SDB_OK ;
            break ;
         }
      }

   done :
      if ( context && getContext )
      {
         _pDataSu->releaseMBContext( context ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSU_GETINDEX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo ( vector<monCLSimple> &collectionList,
                                    BOOLEAN sys )
   {
      PD_TRACE_ENTRY( SDB__DMSSU_DUMPINFO ) ;
      // lock meta data
      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         if ( !sys && dmsIsSysCLName( it->first ) )
         {
            ++it ;
            continue ;
         }
         monCLSimple info ;
         ossStrncpy ( info._name, it->first, DMS_COLLECTION_NAME_SZ ) ;
         info._name[ DMS_COLLECTION_NAME_SZ ] = 0 ;
         // add
         collectionList.push_back ( info ) ;

         ++it ;
      }

      // release meta lock
      _pDataSu->_metadataLatch.release_shared() ;
      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPCLSIMPLE, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo( set<monCLSimple> &collectionList,
                                   BOOLEAN sys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPCLSIMPLE ) ;
      // lock meta
      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         monCLSimple collection ;
         if ( !sys && dmsIsSysCLName( it->first ) )
         {
            ++it ;
            continue ;
         }

         ossMemset ( collection._name, 0, sizeof(collection._name) ) ;
         ossStrncpy ( collection._name, CSName(), DMS_SU_NAME_SZ ) ;
         ossStrncat ( collection._name, ".", 1 ) ;
         ossStrncat ( collection._name, it->first,
                      DMS_COLLECTION_NAME_SZ ) ;
         //add
         collectionList.insert ( collection ) ;

         ++it ;
      }

      // release meta
      _pDataSu->_metadataLatch.release_shared() ;
      PD_TRACE_EXIT ( SDB__DMSSU_DUMPCLSIMPLE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO1, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo ( set<_monCollection> &collectionList,
                                    BOOLEAN sys )
   {
      dmsMB *mb = NULL ;
      dmsMBStatInfo *mbStat = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO1 ) ;
      // lock meta
      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         monCollection collection ;
         if ( !sys && dmsIsSysCLName( it->first ) )
         {
            ++it ;
            continue ;
         }

         mb = &_pDataSu->_dmsMME->_mbList[it->second] ;
         mbStat = &_pDataSu->_mbStatInfo[it->second] ;

         ossMemset ( collection._name, 0, sizeof(collection._name) ) ;
         ossStrncpy ( collection._name, CSName(), DMS_SU_NAME_SZ ) ;
         ossStrncat ( collection._name, ".", 1 ) ;
         ossStrncat ( collection._name, mb->_collectionName,
                      DMS_COLLECTION_NAME_SZ ) ;
         detailedInfo &info = collection.addDetails ( CSSequence(),
                                                      mb->_numIndexes,
                                                      mb->_blockID,
                                                      mb->_flag,
                                                      mb->_logicalID,
                                                      mbStat->_totalRecords,
                                                      mbStat->_totalDataPages,
                                                      mbStat->_totalIndexPages,
                                                      mbStat->_totalLobPages,
                                                      mbStat->_totalDataFreeSpace,
                                                      mbStat->_totalIndexFreeSpace ) ;

         info._attribute = mb->_attributes ;
         info._dictCreated = mb->_dictExtentID != DMS_INVALID_EXTENT ? 1 : 0 ;
         info._compressType = mb->_compressorType ;
         info._dictVersion = mb->_dictVersion ;

         info._totalLobs = mbStat->_totalLobs ;

         info._pageSize = getPageSize() ;
         info._lobPageSize = getLobPageSize() ;
         info._currCompressRatio = mbStat->_lastCompressRatio ;

         /// sync info
         info._dataCommitLSN = mb->_commitLSN ;
         info._idxCommitLSN = mb->_idxCommitLSN ;
         info._lobCommitLSN = mb->_lobCommitLSN ;
         info._dataIsValid = mbStat->_commitFlag.peek() ? TRUE : FALSE ;
         info._idxIsValid = mbStat->_idxCommitFlag.peek() ? TRUE : FALSE ;
         info._lobIsValid = mbStat->_lobCommitFlag.peek() ? TRUE : FALSE ;

         if ( !_pLobSu->isOpened() )
         {
            info._lobCommitLSN = 0 ;
            info._lobIsValid = TRUE ;
         }

         //add
         collectionList.insert ( collection ) ;

         ++it ;
      }

      // release meta
      _pDataSu->_metadataLatch.release_shared() ;
      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO1 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_DUMPINFO2, "_dmsStorageUnit::dumpInfo" )
   void _dmsStorageUnit::dumpInfo ( set<_monStorageUnit> &storageUnitList,
                                    BOOLEAN sys )
   {
      monStorageUnit su ;
      const dmsStorageUnitHeader *dataHeader = _pDataSu->getHeader() ;

      PD_TRACE_ENTRY ( SDB__DMSSU_DUMPINFO2 ) ;
      if ( !sys && dmsIsSysCSName( CSName() ) )
      {
         goto done ;
      }

      ossMemset ( su._name, 0, sizeof ( su._name ) ) ;
      ossStrncpy ( su._name, CSName(), DMS_SU_NAME_SZ ) ;
      su._pageSize = getPageSize() ;
      su._lobPageSize = getLobPageSize() ;
      su._sequence = CSSequence() ;
      su._numCollections = dataHeader->_numMB ;
      su._collectionHWM = dataHeader->_MBHWM ;
      su._size = totalSize() ;
      su._CSID = CSID() ;
      su._logicalCSID = LogicalCSID() ;

      //add
      storageUnitList.insert ( su ) ;
   done :
      PD_TRACE_EXIT ( SDB__DMSSU_DUMPINFO2 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALSIZE, "_dmsStorageUnit::totalSize" )
   INT64 _dmsStorageUnit::totalSize( UINT32 type ) const
   {
      INT64 totalSize = 0 ;
      const dmsStorageUnitHeader *dataHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALSIZE ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         dataHeader = _pDataSu->getHeader() ;
         totalSize += ( (INT64)( dataHeader->_storageUnitSize ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( type & DMS_SU_INDEX )
      {
         dataHeader = _pIndexSu->getHeader() ;
         totalSize += ( (INT64)( dataHeader->_storageUnitSize ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalSize += ( (INT64)( _pLobSu->getHeader()->_storageUnitSize ) <<
                        _pLobSu->pageSizeSquareRoot() ) ;
         totalSize += _pLobSu->getLobData()->getFileSz() ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALSIZE,
                  PD_PACK_LONG ( totalSize ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALSIZE ) ;
      return totalSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALDATAPAGES, "_dmsStorageUnit::totalDataPages" )
   INT64 _dmsStorageUnit::totalDataPages( UINT32 type ) const
   {
      INT64 totalDataPages = 0 ;
      const dmsStorageUnitHeader *dataHeader = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALDATAPAGES ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         dataHeader = _pDataSu->getHeader() ;
         totalDataPages += dataHeader->_pageNum ;
      }
      if ( type & DMS_SU_INDEX )
      {
         dataHeader = _pIndexSu->getHeader() ;
         totalDataPages += dataHeader->_pageNum ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalDataPages += _pLobSu->getHeader()->_pageNum ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALDATAPAGES,
                  PD_PACK_LONG ( totalDataPages ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALDATAPAGES ) ;
      return totalDataPages ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALDATASIZE, "_dmsStorageUnit::totalDataSize" )
   INT64 _dmsStorageUnit::totalDataSize( UINT32 type ) const
   {
      INT64 totalSize = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALDATASIZE ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         totalSize += ( totalDataPages( DMS_SU_DATA ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( type & DMS_SU_INDEX )
      {
         totalSize += ( totalDataPages( DMS_SU_INDEX ) <<
                        _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalSize += _pLobSu->getLobData()->getDataSz() ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALDATASIZE,
                  PD_PACK_LONG ( totalSize ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALDATASIZE ) ;
      return totalSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_TOTALFREEPAGES, "_dmsStorageUnit::totalFreePages" )
   INT64 _dmsStorageUnit::totalFreePages ( UINT32 type ) const
   {
      INT64 freePages = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSSU_TOTALFREEPAGES ) ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         freePages += (INT64)_pDataSu->freePageNum() ;
      }
      if ( type & DMS_SU_INDEX )
      {
         freePages += (INT64)_pIndexSu->freePageNum() ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         freePages += (INT64)_pLobSu->freePageNum() ;
      }

   done:
      PD_TRACE1 ( SDB__DMSSU_TOTALFREEPAGES,
                  PD_PACK_INT ( freePages ) ) ;
      PD_TRACE_EXIT ( SDB__DMSSU_TOTALFREEPAGES ) ;
      return freePages ;
   }

   INT64 _dmsStorageUnit::totalFreeSize( UINT32 type ) const
   {
      INT64 totalFreeSize = 0 ;

      if ( !_pDataSu || !_pIndexSu || !_pLobSu )
      {
         goto done ;
      }

      if ( type & DMS_SU_DATA )
      {
         totalFreeSize += ( totalFreePages( DMS_SU_DATA ) <<
                            _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( type & DMS_SU_INDEX )
      {
         totalFreeSize += ( totalFreePages( DMS_SU_INDEX ) <<
                            _pDataSu->pageSizeSquareRoot() ) ;
      }
      if ( ( type & DMS_SU_LOB ) && _pLobSu->isOpened() )
      {
         totalFreeSize += ( totalFreePages( DMS_SU_LOB ) *
                            _pDataSu->getLobdPageSize() ) ;
      }

   done:
      return totalFreeSize ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSU_GETSTATINFO, "_dmsStorageUnit::getStatInfo" )
   void _dmsStorageUnit::getStatInfo( dmsStorageUnitStat & statInfo )
   {
      PD_TRACE_ENTRY ( SDB__DMSSU_GETSTATINFO ) ;
      ossMemset( &statInfo, 0, sizeof( dmsStorageUnitStat ) ) ;

      dmsMBStatInfo *mbStat = NULL ;

      // lock meta
      _pDataSu->_metadataLatch.get_shared() ;

      dmsStorageData::COLNAME_MAP_IT it = _pDataSu->_collectionNameMap.begin() ;
      while ( it != _pDataSu->_collectionNameMap.end() )
      {
         mbStat = &_pDataSu->_mbStatInfo[it->second] ;

         ++statInfo._clNum ;
         statInfo._totalCount += mbStat->_totalRecords ;
         statInfo._totalDataPages += mbStat->_totalDataPages ;
         statInfo._totalIndexPages += mbStat->_totalIndexPages ;
         statInfo._totalLobPages += mbStat->_totalLobPages ;
         statInfo._totalDataFreeSpace += mbStat->_totalDataFreeSpace ;
         statInfo._totalIndexFreeSpace += mbStat->_totalIndexFreeSpace ;

         ++it ;
      }

      // release meta
      _pDataSu->_metadataLatch.release_shared() ;
      PD_TRACE_EXIT ( SDB__DMSSU_GETSTATINFO ) ;
   }

   void _dmsStorageUnit::setSyncConfig( UINT32 syncInterval,
                                        UINT32 syncRecordNum,
                                        UINT32 syncDirtyRatio )
   {
      if ( _pLobSu )
      {
         _pLobSu->setSyncConfig( syncInterval,
                                 syncRecordNum,
                                 syncDirtyRatio ) ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->setSyncConfig( syncInterval,
                                   syncRecordNum,
                                   syncDirtyRatio ) ;
      }
      if ( _pDataSu )
      {
         _pDataSu->setSyncConfig( syncInterval,
                                  syncRecordNum,
                                  syncDirtyRatio ) ;
      }
   }

   void _dmsStorageUnit::setSyncDeep( BOOLEAN syncDeep )
   {
      if ( _pLobSu )
      {
         _pLobSu->setSyncDeep( syncDeep ) ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->setSyncDeep( syncDeep ) ;
      }
      if ( _pDataSu )
      {
         _pDataSu->setSyncDeep( syncDeep ) ;
      }
   }

   void _dmsStorageUnit::enableSync( BOOLEAN enable )
   {
      if ( _pLobSu )
      {
         _pLobSu->enableSync( enable ) ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->enableSync( enable ) ;
      }
      if ( _pDataSu )
      {
         _pDataSu->enableSync( enable ) ;
      }
   }

   void _dmsStorageUnit::restoreForCrash()
   {
      if ( _pLobSu )
      {
         _pLobSu->restoreForCrash() ;
      }
      if ( _pIndexSu )
      {
         _pIndexSu->restoreForCrash() ;
      }
      if ( _pDataSu )
      {
         _pDataSu->restoreForCrash() ;
      }
   }

   INT32 _dmsStorageUnit::sync( BOOLEAN sync,
                                IExecutor *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;

      if ( NULL != _pLobSu && _pLobSu->isOpened() )
      {
         _pLobSu->lock() ;
         rcTmp = _pLobSu->sync( TRUE, sync, cb ) ;
         _pLobSu->unlock() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Sync file[%s] failed, rc: %d",
                    _pLobSu->getSuFileName(), rc ) ;
            /// not go to error
            rc = rc ? rc : rcTmp ;
         }
      }

      if ( NULL != _pIndexSu )
      {
         _pIndexSu->lock() ;
         rcTmp = _pIndexSu->sync( TRUE, sync, cb ) ;
         _pIndexSu->unlock() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Sync file[%s] failed, rc: %d",
                    _pLobSu->getSuFileName(), rc ) ;
            /// not go to error
            rc = rc ? rc : rcTmp ;
         }
      }

      /// data file must be the last
      if ( NULL != _pDataSu )
      {
         _pDataSu->lock() ;
         rc = _pDataSu->sync( TRUE, sync, cb ) ;
         _pDataSu->unlock() ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Sync file[%s] failed, rc: %d",
                    _pLobSu->getSuFileName(), rc ) ;
            /// not go to error
            rc = rc ? rc : rcTmp ;
         }
      }

      return rc ;
   }

   UINT64 _dmsStorageUnit::getCurrentDataLSN() const
   {
      return NULL == _pDataSu ?
             -1 : _pDataSu->getCommitLSN() ;
   }

   UINT64 _dmsStorageUnit::getCurrentIdxLSN() const
   {
      return NULL == _pIndexSu ?
             -1 : _pIndexSu->getCommitLSN() ;
   }

   UINT64 _dmsStorageUnit::getCurrentLobLSN() const
   {
      return NULL == _pLobSu ?
             -1 : _pLobSu->getCommitLSN() ;
   }

   void _dmsStorageUnit::getValidFlag( BOOLEAN &dataFlag,
                                       BOOLEAN &idxFlag,
                                       BOOLEAN &lobFlag ) const
   {
      dataFlag =  NULL == _pDataSu ?
                  TRUE : ( _pDataSu->getCommitFlag() ? TRUE : FALSE ) ;
      idxFlag = NULL == _pIndexSu ?
                TRUE : ( _pIndexSu->getCommitFlag() ? TRUE : FALSE ) ;

      /// _pLobSu may be NULL, set it as 1
      lobFlag = ( NULL == _pLobSu || !_pLobSu->isOpened() ) ?
                TRUE : ( _pLobSu->getCommitFlag() ? TRUE : FALSE ) ;
   }

}  // namespace engine

