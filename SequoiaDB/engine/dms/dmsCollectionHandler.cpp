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

   Source File Name = dmsCollectionHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/28/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsCollectionHandler.hpp"
#include "dms.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsObjectMetaInfo.hpp"
#include "msgDef.h"
#include "rtn.hpp"
#include "utilUniqueID.hpp"

namespace engine
{
   pmdEDUCB *castToEDUCB( IExecutor *executor )
   {
      pmdEDUCB *cb = dynamic_cast< pmdEDUCB * >( executor );
      if ( executor )
      {
         SDB_ASSERT( cb, "can not be nullptr" );
      }
      return cb;
   }

   SDB_DPSCB *castToDPSCB( IDataProtectionService *dps )
   {
      SDB_DPSCB *dpsCB = dynamic_cast< SDB_DPSCB * >( dps );
      if ( dps )
      {
         SDB_ASSERT( dpsCB, "can not be nullptr" );
      }
      return dpsCB;
   }

   class mbLockGuard : public utilPooledObject
   {
      public:
         mbLockGuard( dmsMBContext *mbContext, INT32 lockType )
         : _mbContext( mbContext ), _lockType( lockType )
         {
            if ( _lockType == SHARED || _lockType == EXCLUSIVE )
            {
               _mbContext->mbLock( _lockType );
            }
         }
         ~mbLockGuard()
         {
            if ( _lockType == SHARED || _lockType == EXCLUSIVE )
            {
               _mbContext->mbUnlock();
            }
         }

         mbLockGuard( mbLockGuard &&o)
         {
            _mbContext = o._mbContext;
            _lockType = o._lockType;
            o._mbContext = nullptr;
            o._lockType = -1;
         }

         mbLockGuard &operator=(mbLockGuard &&o)
         {
            _mbContext = o._mbContext;
            _lockType = o._lockType;
            o._mbContext = nullptr;
            o._lockType = -1;
            return *this;
         }

      private:
         dmsMBContext *_mbContext = nullptr;
         INT32 _lockType = -1;
   };

   mbLockGuard getSharedMBLockGuard( dmsMBContext *mbContext )
   {
      if ( mbContext->isMBLock() )
      {
         return mbLockGuard( mbContext, -1 );
      }
      else
      {
         return mbLockGuard( mbContext, SHARED );
      }
   }

   mbLockGuard getExclusiveMBLockGuard( dmsMBContext *mbContext )
   {
      if ( mbContext->mbLockType() == EXCLUSIVE )
      {
         return mbLockGuard( mbContext, -1 );
      }
      else if ( mbContext->mbLockType() == SHARED )
      {
         // Expect not yet locked or already locked exclusively.
         SDB_ASSERT( FALSE, "invalid lock type" );
         return mbLockGuard( mbContext, EXCLUSIVE );
      }
      else
      {
         return mbLockGuard( mbContext, EXCLUSIVE );
      }
   }

   dmsCollectionHandler::dmsCollectionHandler( dmsStorageUnit *su,
                                               dmsStorageUnitID suID,
                                               dmsMBContext *mbContext,
                                               dmsMmapEngine *mmapEngine )
   : _su( su ), _suID( suID ), _mbContext( mbContext ), _engine( mmapEngine )
   {
   }

   dmsCollectionHandler::~dmsCollectionHandler()
   {
      if ( !isClosed() )
      {
         _release();
      }
   }

   BOOLEAN dmsCollectionHandler::isClosed() const
   {
      return _mbContext == nullptr || _su == nullptr || _suID == DMS_INVALID_CS ||
             _engine == nullptr;
   }

   void dmsCollectionHandler::close()
   {
      if ( !isClosed() )
      {
         _release();
      }
   }

   void dmsCollectionHandler::_release()
   {
      if ( _suID != DMS_INVALID_CS )
      {
         SDB_ASSERT( _su && _mbContext && _engine, "can not be nullptr" );
         _su->data()->releaseMBContext( _mbContext );
         _engine->suUnlock( _suID );
      }
      _su = nullptr;
      _suID = DMS_INVALID_CS;
      _mbContext = nullptr;
      _engine = nullptr;
   }

   INT32 dmsCollectionHandler::getMetaData( IExecutor *executor, CONST_CL_META_INFO_PTR &meta )
   {
      INT32 rc = SDB_OK;
      MON_IDX_LIST idxList;
      DMS_CL_META_PTR tempCl = nullptr;
      if ( isClosed() )
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG( PDERROR, "dms collection handler is closed" );
         goto error;
      }

      try
      {
         mbLockGuard guard = getSharedMBLockGuard( _mbContext );
         std::shared_ptr< ossPoolString > clFullName =
            makeSharedPtrFromPool< ossPoolString >( _su->CSName() );
         PD_CHECK( clFullName, SDB_OOM, error, PDERROR, "out of memory, rc: %d", rc );
         clFullName->push_back( '.' );
         clFullName->append( _mbContext->mb()->_collectionName );
         tempCl = makeSharedPtrFromPool< dmsCollectionMetaInfo >(
            clFullName, _mbContext->mb()->_clUniqueID, _mbContext->mb()->_attributes,
            _su->getPageSizeLog2(), _mbContext->mbStat()->_totalDataPages,
            _mbContext->mbStat()->_globTransAvailTime.peek() );
         PD_CHECK( tempCl, SDB_OOM, error, PDERROR, "out of memory, rc: %d", rc );

         rc = _su->getIndexes( _mbContext, idxList, FALSE );
         PD_RC_CHECK( rc, PDERROR, "failed to dump indexes, rc: %d", rc );

         for ( MON_IDX_LIST::const_iterator it = idxList.cbegin(); it != idxList.cend(); ++it )
         {
            DMS_INDEX_META_PTR tempIndex = nullptr;
            BSONObjBuilder builder;
            UINT16 indexType = 0;
            rc = it->getIndexType( indexType );
            PD_RC_CHECK( rc, PDERROR, "failed to get index type, name: %s.%s.%s, rc: %d",
                         _su->CSName(), _mbContext->mb()->_collectionName, it->getIndexName(), rc );
            builder.appendElements( it->_indexDef );
            builder.append( IXM_FIELD_NAME_CB_EXTENT_ID, it->_indexCBExtentID );
            builder.append( FIELD_NAME_LOGICAL_ID, (INT64)it->_indexLID );
            builder.append( IXM_FIELD_NAME_TYPE, (INT32)indexType );
            builder.append( IXM_FIELD_NAME_INDEX_FLAG, (INT32)it->_indexFlag );
            rc = _dmsIndexMetaInfo::buildFromBson( builder.obj(), tempIndex, clFullName );
            PD_RC_CHECK( rc, PDERROR, "failed to build index[name: %s] meta info from bson, rc: %d",
                         it->getIndexName(), rc );
            rc = tempCl->pushIndexMetaInfo( tempIndex );
            PD_RC_CHECK( rc, PDERROR, "failed to push index[name: %s] meta info, rc: %d",
                         it->getIndexName(), rc );
         }
         meta = tempCl;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "occur exception: ", e.what() );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   DMS_STORAGE_TYPE dmsCollectionHandler::getCSStorageType()
   {
      SDB_ASSERT( _su, "can not be nullptr" );
      return _su->type();
   }

   INT32 dmsCollectionHandler::createIndex( IExecutor *executor,
                                            const dmsBuildIndexOptions &o,
                                            const bson::BSONObj &indexDef )
   {
      INT32 rc = SDB_OK;
      if ( isClosed() )
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG( PDERROR, "dms collection handler is closed" );
         goto error;
      }

      if ( DMS_STORAGE_CAPPED == _su->type() )
      {
         PD_LOG( PDERROR, "Index is not support on capped collection" );
         rc = SDB_OPTION_NOT_SUPPORT;
         goto error;
      }

      {
         pmdEDUCB *cb = castToEDUCB( executor );
         SDB_DPSCB *dpsCB = castToDPSCB( o.dpsCB );
         mbLockGuard guard = getExclusiveMBLockGuard( _mbContext );
         rc = _su->createIndex( _mbContext->mb()->_collectionName, indexDef, cb, dpsCB, o.sysCall,
                                _mbContext, o.sortBufferSize, o.result, o.idxStatus,
                                o.forceTransCallback, o.addUIDIfNotExist );

         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to create index %s.%s: %s, rc: %d", _su->CSName(),
                    _mbContext->mb()->_collectionName, indexDef.toString().c_str(), rc );
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dmsCollectionHandler::listIndex( IExecutor *executor,
                                          ossPoolVector< bson::BSONObj > &indexes )
   {
      INT32 rc = SDB_OK;
      MON_IDX_LIST idxList;
      if ( isClosed() )
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG( PDERROR, "dms collection handler is closed" );
         goto error;
      }
      {
         mbLockGuard guard = getSharedMBLockGuard( _mbContext );
         rc = _su->getIndexes( _mbContext, idxList, FALSE );
         PD_RC_CHECK( rc, PDERROR, "dump indexes failed, rc: %d", rc );

         for ( MON_IDX_LIST::const_iterator it = idxList.cbegin(); it != idxList.cend(); ++it )
         {
            BSONObjBuilder builder;
            builder.appendElements( it->_indexDef );
            builder.append( IXM_FIELD_NAME_CB_EXTENT_ID, it->_indexCBExtentID );
            builder.append( FIELD_NAME_LOGICAL_ID, (INT64)it->_indexLID );
            indexes.push_back( builder.obj() );
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dmsCollectionHandler::removeIndex( IExecutor *executor, const CHAR *indexName )
   {
      return SDB_NOT_SUPPORTED;
   }

   INT32 dmsCollectionHandler::removeIndex( IExecutor *executor,
                                            const CHAR *indexName,
                                            const dmsRemoveIndexOptions &o )
   {
      INT32 rc = SDB_OK;
      if ( isClosed() )
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG( PDERROR, "dms collection handler is closed" );
         goto error;
      }
      {
         pmdEDUCB *cb = castToEDUCB( executor );
         SDB_DPSCB *dpsCB = castToDPSCB( o.dpsCB );
         mbLockGuard guard = getExclusiveMBLockGuard( _mbContext );
         rc = _su->dropIndex( _mbContext->mb()->_collectionName, indexName, cb, dpsCB, o.sysCall,
                              _mbContext, o.idxStatus, o.onlyStandalone );
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dmsCollectionHandler::removeIndex( IExecutor *executor,
                                            const OID &indexOID,
                                            const dmsRemoveIndexOptions &o )
   {
      INT32 rc = SDB_OK;
      if ( isClosed() )
      {
         rc = SDB_INVALID_OPERATION;
         PD_LOG( PDERROR, "dms collection handler is closed" );
         goto error;
      }
      {
         pmdEDUCB *cb = castToEDUCB( executor );
         SDB_DPSCB *dpsCB = castToDPSCB( o.dpsCB );
         mbLockGuard guard = getExclusiveMBLockGuard( _mbContext );
         rc = _su->dropIndex( _mbContext->mb()->_collectionName, indexOID, cb, dpsCB, o.sysCall,
                              _mbContext, o.idxStatus, o.onlyStandalone );
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dmsCollectionHandler::truncate( IExecutor *executor, const dmsTruncateCLOptions &o )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::insertRecord( IExecutor *executor,
                                             const bson::BSONObj &record,
                                             const dmsInsertRecordOptions &o,
                                             utilInsertResult *result )
   {
      SDB_ASSERT( NULL != result, "insert result is invalid" );
      INT32 rc = SDB_OK;
      pmdEDUCB *cb = castToEDUCB( executor );
      SDB_DPSCB *dpsCB = castToDPSCB( o.dpsCB );
      mbLockGuard guard = getExclusiveMBLockGuard( _mbContext );
      rc = _su->insertRecord( _mbContext->mb()->_collectionName, record, cb, dpsCB, TRUE, FALSE,
                              _mbContext, o.position, result );
      if ( SDB_OK != rc )
      {
         if ( DMS_STORAGE_CAPPED == _su->type() )
         {
            PD_LOG( PDERROR,
                     "Failed to insert record into collection [%s.%s] "
                     "by position[%lld], rc: %d",
                     _su->CSName(), _mbContext->mb()->_collectionName, o.position, rc );
         }
         else
         {
            PD_LOG( PDERROR,
                     "Failed to insert record into collection [%s.%s], rc: %d",
                     _su->CSName(), _mbContext->mb()->_collectionName, rc );
         }
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dmsCollectionHandler::insertBatch( IExecutor *executor,
                                            const ossPoolVector< bson::BSONObj > &batch,
                                            const dmsInsertRecordOptions &o,
                                            utilInsertResult *result )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::updateRecord( IExecutor *executor,
                                             const dmsRecordID &rid,
                                             IRecordUpdater *updater,
                                             const dmsUpdateRecordOptions &o,
                                             utilUpdateResult *result )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::deleteRecord( IExecutor *executor,
                                             const dmsRecordID &rid,
                                             const dmsDeleteRecordOptions &o,
                                             utilDeleteResult *result )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::scan( IExecutor *executor,
                                     const dmsScanOptions &o,
                                     DATA_CURSOR_PTR &cursor )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::scanIndex( IExecutor *executor,
                                          const CHAR *indexName,
                                          const rtnPredicateList &predicate,
                                          const dmsIndexScanOptions &o,
                                          DATA_CURSOR_PTR &cursor )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::getRecordCount( IExecutor *executor, UINT64 &count )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::insertLobChunk( IExecutor *executor,
                                               const bson::OID &oid,
                                               UINT32 chunkId,
                                               UINT32 offset,
                                               UINT32 size,
                                               const CHAR *data )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::readLobChunk( IExecutor *executor,
                                             const bson::OID &oid,
                                             UINT32 chunkId,
                                             UINT32 offset,
                                             UINT32 size,
                                             CHAR *data,
                                             UINT32 &readSize )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::removeLobChunk( IExecutor *executor,
                                               const bson::OID &oid,
                                               UINT32 chunkId )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::updateLobChunk( IExecutor *executor,
                                               const bson::OID &oid,
                                               UINT32 chunkId,
                                               UINT32 offset,
                                               UINT32 size,
                                               const CHAR *data,
                                               BOOLEAN createIfNotExists )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::truncateLobChunk( IExecutor *executor,
                                                 const bson::OID &oid,
                                                 UINT32 chunkId,
                                                 UINT32 size,
                                                 UINT32 &tsize )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::listLobChunks( IExecutor *executor,
                                              const dmsListLobChunkOptions &o,
                                              DATA_CURSOR_PTR &cursor )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 dmsCollectionHandler::testLobChunk( IExecutor *executor,
                                             const bson::OID &oid,
                                             UINT32 chunkId,
                                             dmsLobChunkProfile *profile )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }
} // namespace engine