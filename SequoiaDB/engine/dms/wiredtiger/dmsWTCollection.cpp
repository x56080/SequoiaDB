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
#include "ossErr.h"
#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "wiredtiger/dmsWTDataCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_TRUNCIDX, "_dmsWTCollection::truncateIndex" )
   INT32 _dmsWTCollection::truncateIndex( const dmsIdxMetadata &metadata,
                                          const dmsTruncateIdxOptions &options,
                                          IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_TRUNCIDX ) ;

      ossPoolString idxURI ;

      rc = dmsWTIndex::buildIdxURI( metadata.getCSUID(),
                                    metadata.getCLOrigInnerID(),
                                    metadata.getCLOrigLID(),
                                    metadata.getIdxInnerID(),
                                    idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build index URI, rc: %d", rc ) ;

      rc = _engine.truncateStore( idxURI.c_str(), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate idnex store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_TRUNCIDX, rc ) ;
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

      // TODO: no need to be aligned in WT
      UINT64 tmpRID = _metadata.getMBStat()->_ridGen.add( ossAlignX( length, 4 ) ) ;
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

      rc = _engine.insertToStore( _store, key, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record to engine, rc: %d", rc ) ;

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

      rc = _engine.updateToStore( _store, key, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update record to engine, rc: %d", rc ) ;

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

      rc = _engine.removeFromStore( _store, key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove record from engine, rc: %d", rc ) ;

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
      rc = _engine.extractFromStore( _store, key, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record from engine, rc: %d", rc ) ;

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

      cursor = unique_ptr<dmsWTDataCursor>( new dmsWTDataCursor() ) ;
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;

      rc = cursor->open( std::move( shared_from_this() ),
                         startRID,
                         afterStartRID,
                         isForward,
                         executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open data cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_CREATEDATACURSOR, rc ) ;
      return rc ;

   error:
      cursor.release() ;
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
         ss << "block_compressor=snappy," ;
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
         idxPtr = make_shared<dmsWTIndex>( metadata, _engine, store ) ;
         PD_CHECK( idxPtr, SDB_OOM, error, PDERROR,
                   "Failed to create index object" ) ;

         ossScopedRWLock lock( &_idxMapMutex, EXCLUSIVE ) ;
         _idxMap.insert( make_pair( metadata.getIdxKey(), idxPtr ) ) ;
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

}
}
