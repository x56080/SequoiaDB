/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = dmsWTCollection.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTCollection.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "wiredtiger/dmsWTDataCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"
#include <memory>

using namespace std ;
using namespace bson ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCollection implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_CREATEIDX, "_dmsWTCollection::createIndex" )
   INT32 _dmsWTCollection::createIndex( const dmsIdxMetadata &metadata,
                                        const dmsCreateIdxOptions &options,
                                        IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_CREATEIDX ) ;

      dmsWTStore store ;
      ossPoolString idxURI, idxConfig ;
      shared_ptr<IIndex> idxPtr ;

      rc = dmsWTIndex::buildIdxURI( metadata.getCSUID(),
                                    metadata.getCLOrigInnerID(),
                                    metadata.getCLOrigLID(),
                                    metadata.getIdxInnerID(),
                                    idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index URI, rc: %d", rc ) ;

      rc = dmsWTIndex::buildIdxConfigString( _engine.getOptions(), options, idxConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index config string, rc: %d", rc ) ;

      rc = _engine.createStore( idxURI.c_str(), idxConfig.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create index store, rc: %d", rc ) ;

      rc = _addIndex( metadata, store, idxPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add index, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_CREATEIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_DROPIDX, "_dmsWTCollection::dropIndex" )
   INT32 _dmsWTCollection::dropIndex( const dmsIdxMetadata &metadata,
                                      const dmsDropIdxOptions &options,
                                      IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_DROPIDX ) ;

      ossPoolString idxURI ;

      rc = dmsWTIndex::buildIdxURI( metadata.getCSUID(),
                                    metadata.getCLOrigInnerID(),
                                    metadata.getCLOrigLID(),
                                    metadata.getIdxInnerID(),
                                    idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index URI, rc: %d", rc ) ;

      rc = _engine.dropStore( idxURI.c_str(), "force,checkpoint_wait=false" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop index store, rc: %d", rc ) ;

      _removeIndex( metadata.getIdxKey() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_DROPIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_TRUNC, "_dmsWTCollection::truncate" )
   INT32 _dmsWTCollection::truncate( const dmsTruncCLOptions &options,
                                     IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_TRUNC ) ;

      dmsWTSession &session = dmsWTSession::getPersistSession( executor ) ;
      if ( session.isOpened() )
      {
         rc = _engine.truncateStore( session, _store.getURI().c_str(), nullptr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate data store, rc: %d", rc ) ;
      }
      else
      {
         rc = _engine.truncateStore( _store.getURI().c_str(), nullptr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate data store, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_TRUNC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETIDX, "_dmsWTCollection::getIndex" )
   INT32 _dmsWTCollection::getIndex( const dmsIdxMetadataKey &metadataKey,
                                     IExecutor *executor,
                                     shared_ptr<IIndex> &idxPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETIDX ) ;

      idxPtr = _getIndex( metadataKey ) ;
      PD_CHECK( idxPtr, SDB_IXM_NOTEXIST, error, PDDEBUG,
                "Failed to get index, collection [UID: %llx, LID: %x] "
                "index [UID: %x] not exist", metadataKey.getCLOrigUID(),
                metadataKey.getCLOrigLID(), metadataKey.getIdxInnerID() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_LOADIDX, "_dmsWTCollection::loadIndex" )
   INT32 _dmsWTCollection::loadIndex( const dmsIdxMetadata &metadata,
                                      IExecutor *executor,
                                      shared_ptr<IIndex> &idxPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_LOADIDX ) ;

      dmsWTStore store ;
      ossPoolString idxURI ;

      idxPtr.reset() ;

      rc = dmsWTIndex::buildIdxURI( metadata.getCSUID(),
                                    metadata.getCLOrigInnerID(),
                                    metadata.getCLOrigLID(),
                                    metadata.getIdxInnerID(),
                                    idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index URI, rc: %d", rc ) ;

      rc = _engine.loadStore( idxURI.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load index store [%s], rc: %d",
                   idxURI.c_str(), rc ) ;

      rc = _addIndex( metadata, store, idxPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add index, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_LOADIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_ALLOCRECID, "_dmsWTCollection::allocRecordID" )
   INT32 _dmsWTCollection::allocRecordID( UINT32 length, dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_ALLOCRECID ) ;

      UINT64 tmpRID = _metadata.getMBStat()->_ridGen.inc() ;
      rid._extent = (dmsExtentID)( tmpRID >> 32 ) ;
      rid._offset = (dmsOffset)( tmpRID & 0xFFFFFFFF ) ;

      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_ALLOCRECID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_INSERTREC, "_dmsWTCollection::insertRecord" )
   INT32 _dmsWTCollection::insertRecord( const dmsRecordID &rid,
                                         const dmsRecordData &recordData,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_INSERTREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value( recordData.data(), recordData.len() ) ;

      dmsWTSession &session = dmsWTSession::getPersistSession( executor ) ;
      if ( session.isOpened() )
      {
         dmsWTCursor cursor( session ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = _engine.insertToStore( cursor, key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record to engine, rc: %d", rc ) ;
      }
      else
      {
         rc = _engine.insertToStore( _store, key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record to engine, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_INSERTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_UPDATEREC, "_dmsWTCollection::updateRecord" )
   INT32 _dmsWTCollection::updateRecord( const dmsRecordID &rid,
                                         const dmsRecordData &recordData,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_UPDATEREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value( recordData.data(), recordData.len() ) ;

      dmsWTSession &session = dmsWTSession::getPersistSession( executor ) ;
      if ( session.isOpened() )
      {
         dmsWTCursor cursor( session ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = _engine.updateToStore( cursor, key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update record to engine, rc: %d", rc ) ;
      }
      else
      {
         rc = _engine.updateToStore( _store, key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update record to engine, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_UPDATEREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_RMREC, "_dmsWTCollection::removeRecord" )
   INT32 _dmsWTCollection::removeRecord( const dmsRecordID &rid,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_RMREC ) ;

      UINT64 key = rid.toUINT64() ;

      dmsWTSession &session = dmsWTSession::getPersistSession( executor ) ;
      if ( session.isOpened() )
      {
         dmsWTCursor cursor( session ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = _engine.removeFromStore( cursor, key ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove record to engine, rc: %d", rc ) ;
      }
      else
      {
         rc = _engine.removeFromStore( _store, key ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove record from engine, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_RMREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_EXTRACTREC, "_dmsWTCollection::extractRecord" )
   INT32 _dmsWTCollection::extractRecord( const dmsRecordID &rid,
                                          dmsRecordData &recordData,
                                          IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_EXTRACTREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value ;

      dmsWTSession &session = dmsWTSession::getPersistSession( executor ) ;
      if ( session.isOpened() )
      {
         dmsWTCursor cursor( session ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = _engine.extractFromStore( cursor, key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract record from engine, rc: %d", rc ) ;
      }
      else
      {
         rc = _engine.extractFromStore( _store, key, value ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract record from engine, rc: %d", rc ) ;
      }

      recordData.setData( (const CHAR *)( value.get()->data ),
                          value.get()->size,
                          UTIL_COMPRESSOR_INVALID,
                          TRUE ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_EXTRACTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_CREATEDATACURSOR, "_dmsWTCollection::createDataCursor" )
   INT32 _dmsWTCollection::createDataCursor( unique_ptr<IDataCursor> &cursor,
                                             const dmsRecordID &startRID,
                                             BOOLEAN afterStartRID,
                                             BOOLEAN isForward,
                                             IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_CREATEDATACURSOR ) ;

      UINT64 snapshotID = 0 ;
      IPersistUnit *persistUnit = nullptr ;

      rc = _engine.getService().getPersistUnit( executor, persistUnit ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc ) ;

      if ( persistUnit )
      {
         dmsWTPersistUnit *wtUnit = dynamic_cast<dmsWTPersistUnit *>( persistUnit ) ;
         PD_CHECK( wtUnit, SDB_SYS, error, PDERROR,
                  "Failed to get persist unit, it is not a WiredTiger persist unit" ) ;

         cursor = unique_ptr<dmsWTDataCursor>( new dmsWTDataCursor( wtUnit->getSession() ) ) ;
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;
      }
      else
      {
         cursor = unique_ptr<dmsWTDataCursor>( new dmsWTDataAsyncCursor() ) ;
         PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;
      }

      snapshotID = _metadata.getMBStat()->_snapshotID.fetch() ;
      rc = cursor->open( shared_from_this(),
                         startRID,
                         afterStartRID,
                         isForward,
                         snapshotID,
                         executor ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to open data cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_CREATEDATACURSOR, rc ) ;
      return rc ;

   error:
      cursor.release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_GETCOUNT, "_dmsWTCollection::getCount" )
   INT32 _dmsWTCollection::getCount( UINT64 &count,
                                     BOOLEAN isFast,
                                     IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_GETCOUNT ) ;

      if ( isFast )
      {
         count = _metadata.getMBStat()->_totalRecords.fetch() ;
      }
      else
      {
         dmsWTSession &session = dmsWTSession::getPersistSession( executor ) ;
         if ( session.isOpened() )
         {
            dmsWTCursor cursor( session ) ;

            rc = cursor.open( _store.getURI(), "" ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

            rc = cursor.getCount( count ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to count from store, rc: %d", rc ) ;
         }
         else
         {
            rc = _engine.countFromStore( _store, count ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get count from engine, rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_GETCOUNT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_VALIDATEDATA, "_dmsWTCollection::validateData" )
   INT32 _dmsWTCollection::validateData( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_VALIDATEDATA ) ;

      UINT64 recordCount = 0 ;
      dmsRecordID maxRID ;

      // recover record count
      rc = getCount( recordCount, FALSE, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record count, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Reset total record count [%llu]", recordCount ) ;
      _metadata.getMBStat()->_totalRecords.poke( recordCount ) ;
      _metadata.getMBStat()->_rcTotalRecords.poke( recordCount ) ;

      // recover record ID generator
      rc = _getMaxRecordID( maxRID, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get max record ID, rc: %d", rc ) ;

      if ( maxRID.isValid() )
      {
         PD_LOG( PDEVENT, "Move record ID to [extent: %u, offset: %u]",
                 maxRID._extent, maxRID._offset ) ;
         _metadata.getMBStat()->_ridGen.poke( maxRID.toUINT64() + 1 ) ;
      }
      else
      {
         PD_LOG( PDEVENT, "Move record ID to [extent: 0, offset: 0]" ) ;
         _metadata.getMBStat()->_ridGen.poke( 0 ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_VALIDATEDATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_BLDDATACONFSTR, "_dmsWTCollection::buildDataConfigString" )
   INT32 _dmsWTCollection::buildDataConfigString( const dmsWTEngineOptions &options,
                                                  const dmsCreateCLOptions &createCLOptions,
                                                  ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_BLDDATACONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "type=file," ;
         ss << "memory_page_max=10m," ;
         ss << "split_pct=90," ;
         ss << "leaf_value_max=64MB," ;
         ss << "checksum=on," ;

         switch ( createCLOptions._compressorType )
         {
         case UTIL_COMPRESSOR_SNAPPY:
            ss << "block_compressor=snappy," ;
            break ;
         case UTIL_COMPRESSOR_ZLIB:
            ss << "block_compressor=zlib," ;
            break ;
         case UTIL_COMPRESSOR_LZ4:
            ss << "block_compressor=lz4," ;
            break ;
         default:
            ss << "block_compressor=none," ;
            break ;
         }

         ss << "key_format=q," ;
         ss << "value_format=u," ;
         ss << "app_metadata=(formatVersion=" << DMS_WT_FORMART_VER_CUR << ")," ;
         ss << "log=(enabled=true)," ;

         configString = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build data config string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_BLDDATACONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_BLDDATAURI, "_dmsWTCollection::buildDataURI" )
   INT32 _dmsWTCollection::buildDataURI( utilCSUniqueID csUID,
                                         utilCLInnerID clInnerID,
                                         UINT32 clLID,
                                         ossPoolString &dataURI )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_BLDDATAURI ) ;

      try
      {
         ossPoolStringStream ss ;
         ss << "table:" ;
         dmsWTBuildDataIdent( csUID, clInnerID, clLID, ss ) ;
         dataURI = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build data URI, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_BLDDATAURI, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__ADDINDEX, "_dmsWTCollection::_addIndex" )
   INT32 _dmsWTCollection::_addIndex( const dmsIdxMetadata &metadata,
                                      const dmsWTStore &store,
                                      shared_ptr<IIndex> &idxPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__ADDINDEX ) ;

      try
      {
         dmsIdxMetadataKey key( metadata.getIdxKey() ) ;
         PD_CHECK( key.isValid(), SDB_SYS, error, PDERROR,
                   "Failed to save index, collection [UID: %llx, LID: %x], "
                   "index [UID: %x] is not valid",
                   key.getCLOrigUID(), key.getCLOrigLID(), key.getIdxInnerID() ) ;
         idxPtr = std::make_shared<dmsWTIndex>( metadata, _engine, store ) ;
         PD_CHECK( idxPtr, SDB_OOM, error, PDERROR,
                   "Failed to create index object" ) ;

         ossScopedRWLock lock( &_idxMapMutex, EXCLUSIVE ) ;
         auto res = _idxMap.insert( make_pair( key, idxPtr ) ) ;
         if ( !res.second )
         {
            PD_LOG( PDDEBUG, "Failed to add index, collection "
                    "[UID: %llx, LID: %x], index [UID: %x] already exist",
                    key.getCLOrigUID(), key.getCLOrigLID(), key.getIdxInnerID() ) ;
            idxPtr = res.first->second ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save index, occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION__ADDINDEX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__REMOVEINDEX, "_dmsWTCollection::_removeIndex" )
   void _dmsWTCollection::_removeIndex( const dmsIdxMetadataKey &metadataKey )
   {
      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__REMOVEINDEX ) ;

      ossScopedRWLock lock( &_idxMapMutex, EXCLUSIVE ) ;
      _idxMap.erase( metadataKey ) ;

      PD_TRACE_EXIT( SDB__DMSWTCOLLECTION__REMOVEINDEX ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__GETINDEX, "_dmsWTCollection::_getIndex" )
   shared_ptr<IIndex> _dmsWTCollection::_getIndex( const dmsIdxMetadataKey &metadataKey )
   {
      shared_ptr<IIndex> idxPtr ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__GETINDEX ) ;

      {
         ossScopedRWLock lock( &_idxMapMutex, SHARED ) ;
         _dmsWTIdxMapIter iter = _idxMap.find( metadataKey ) ;
         if ( iter != _idxMap.end() )
         {
            idxPtr = iter->second ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSWTCOLLECTION__GETINDEX ) ;

      return idxPtr ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__GETMAXRECORDID, "_dmsWTCollection::_getMaxRecordID" )
   INT32 _dmsWTCollection::_getMaxRecordID( dmsRecordID &rid, IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__GETMAXRECORDID ) ;

      dmsWTSession &session = dmsWTSession::getPersistSession( executor ) ;
      if ( session.isOpened() )
      {
         rc = _getMaxRecordID( session, rid, executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get max record ID, rc: %d", rc ) ;
      }
      else
      {
         dmsWTSession tmpSession ;
         dmsWTCursor cursor( tmpSession ) ;

         rc = _engine.openSession( tmpSession ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

         rc = _getMaxRecordID( tmpSession, rid, executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get max record ID, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION__GETMAXRECORDID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION__GETMAXRECORDID_SESS, "_dmsWTCollection::_getMaxRecordID" )
   INT32 _dmsWTCollection::_getMaxRecordID( dmsWTSession &session,
                                            dmsRecordID &rid,
                                            IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION__GETMAXRECORDID_SESS ) ;

      dmsWTCursor cursor( session ) ;
      UINT64 key = 0 ;

      rc = cursor.open( _store.getURI(), "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

      rc = cursor.moveToTail() ;
      if ( SDB_DMS_EOC == rc )
      {
         rid.reset() ;
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to count from store, rc: %d", rc ) ;

      rc = cursor.getKey( key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

      rid.fromUINT64( key ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION__GETMAXRECORDID_SESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
