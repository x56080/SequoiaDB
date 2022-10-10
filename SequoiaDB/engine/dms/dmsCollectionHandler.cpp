/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY{} without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
#include "msgDef.h"
#include "rtn.hpp"
#include "utilUniqueID.hpp"

namespace engine
{
dmsCollectionHandler::dmsCollectionHandler( dmsStorageUnit *su,
                                            dmsStorageUnitID suID,
                                            dmsMBContext *mbContext )
: _su( su ), _suID( suID ), _mbContext( mbContext )
{
}

dmsCollectionHandler::~dmsCollectionHandler()
{
   if ( _suID != DMS_INVALID_CS )
   {
      SDB_ASSERT( _su && _mbContext, "can not be nullptr" );
      _su->data()->releaseMBContext( _mbContext );
      pmdGetKRCB()->getDMSCB()->suUnlock( _suID );
   }
   _reset();
}

BOOLEAN dmsCollectionHandler::isClosed() const
{
   return _mbContext == nullptr && _su == nullptr && _suID == DMS_INVALID_CS;
}

void dmsCollectionHandler::close()
{
   if ( _suID != DMS_INVALID_CS )
   {
      SDB_ASSERT( _su && _mbContext, "can not be nullptr" );
      _su->data()->releaseMBContext( _mbContext );
      pmdGetKRCB()->getDMSCB()->suUnlock( _suID );
   }
   _reset();
}

INT32 dmsCollectionHandler::createIndex( IExecutor *executor,
                                         const dmsBuildIndexOptions &o,
                                         const bson::BSONObj &indexDef )
{
   SDB_ASSERT( FALSE, "todo" );
   return SDB_OK;
}

INT32 dmsCollectionHandler::getMetaData( IExecutor *executor,
                                         bson::BSONObj &data )
{
   INT32 rc = SDB_OK;
   BSONObjBuilder builder;
   if ( isClosed() )
   {
      rc = SDB_DMS_CONTEXT_IS_CLOSE;
      PD_LOG(PDERROR, "dms collection handler is closed");
      goto error;
   }
   builder.append( FIELD_NAME_NAME, _mbContext->mb()->_collectionName );
   builder.append( FIELD_NAME_CL_UNIQUEID,
                   (INT64)_mbContext->mb()->_clUniqueID );

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
      rc = SDB_DMS_CONTEXT_IS_CLOSE;
      PD_LOG(PDERROR, "dms collection handler is closed");
      goto error;
   }
   rc = _su->getIndexes( _mbContext, idxList, FALSE );
   PD_RC_CHECK( rc, PDERROR, "dump indexes failed, rc: %d", rc );

   for ( MON_IDX_LIST::const_iterator it = idxList.cbegin();
         it != idxList.cend();
         ++it )
   {
      indexes.push_back( it->_indexDef );
   }

done:
   return rc;
error:
   goto done;
}

INT32 dmsCollectionHandler::removeIndex( IExecutor *executor,
                                         const CHAR *indexName )
{
   SDB_ASSERT( FALSE, "todo" );
   return SDB_OK;
}

INT32 dmsCollectionHandler::truncate( IExecutor *executor,
                                      const dmsTruncateCLOptions &o )
{
   SDB_ASSERT( FALSE, "todo" );
   return SDB_OK;
}

INT32 dmsCollectionHandler::insertRecord( IExecutor *executor,
                                          const bson::BSONObj &record,
                                          const dmsInsertRecordOptions &o,
                                          utilInsertResult *result )
{
   SDB_ASSERT( FALSE, "todo" );
   return SDB_OK;
}

INT32 dmsCollectionHandler::insertBatch(
   IExecutor *executor,
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