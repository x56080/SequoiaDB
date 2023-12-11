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

   Source File Name = dmsWTStorageService.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTStorageService.hpp"
#include "wiredtiger/dmsWTCollection.hpp"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "pmdOptionsMgr.hpp"

#include <boost/filesystem/operations.hpp>

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   const int DMS_WT_FORMART_V1 = 1 ;
   const int DMS_WT_FORMART_VER_CUR = DMS_WT_FORMART_V1 ;

   /*
      _dmsWTStorageService implement
    */
   _dmsWTStorageService::_dmsWTStorageService()
   {
   }

   _dmsWTStorageService::~_dmsWTStorageService()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_OPENENGINE, "_dmsWTStorageService::openEngine" )
   INT32 _dmsWTStorageService::openEngine( const dmsOpenEngineOptions &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_OPENENGINE ) ;

      dmsWTEngineOptions engineOptions ;
      ossPoolString config ;

      rc = _initEngineOptions( engineOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init WiredTiger engine options, rc: %d", rc ) ;

      rc = _buildConfigString( engineOptions, config ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger config string, rc: %d", rc ) ;
      PD_LOG( PDEVENT, "WiredTiger config string: %s", config.c_str() ) ;

      rc = _engine.open( engineOptions.getDBPath(), config.c_str() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger engine, rc: %d", rc ) ;

      _engineOptions = engineOptions ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_OPENENGINE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CLOSE, "_dmsWTStorageService::close" )
   INT32 _dmsWTStorageService::closeEngine( const dmsCloseEngineOptions &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CLOSE ) ;

      rc = _engine.close( NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to close WiredTiger engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CLOSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CREATECS, "_dmsWTStorageService::createCS" )
   INT32 _dmsWTStorageService::createCS( const dmsCSMetadata &metadata,
                                         const dmsCreateCSOptions &options,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CREATECS ) ;

      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CREATECS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_DROPCS, "_dmsWTStorageService::dropCS" )
   INT32 _dmsWTStorageService::dropCS( const dmsCSMetadata &metadata,
                                       const dmsDropCSOptions &options,
                                       IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_DROPCS ) ;

      ossPoolList<ossPoolString> uriList ;

      rc = _dumpURIListByCS( metadata.getCSUID(), uriList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump WiredTiger URI list, rc: %d", rc ) ;

      rc = _engine.dropStores( uriList, "force,checkpoint_wait=false" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop WiredTiger stores, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_DROPCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CREATECL, "_dmsWTStorageService::createCL" )
   INT32 _dmsWTStorageService::createCL( const dmsCLMetadata &metadata,
                                         const dmsCreateCLOptions &options,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CREATECL ) ;

      dmsWTStore store ;
      ossPoolString dataURI, dataConfig ;
      shared_ptr< ICollection > collPtr ;

      rc = _buildDataURI( metadata.getCSUID(),
                          metadata.getCLOrigInnerID(),
                          metadata.getCLOrigLID(),
                          dataURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger data URI, rc: %d", rc ) ;

      rc = _buildDataConfigString( _engineOptions, options, dataConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger data config string, rc: %d", rc ) ;

      rc = _engine.createStore( dataURI.c_str(), dataConfig.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create WiredTiger data store, rc: %d", rc ) ;

      collPtr = make_shared<dmsWTCollection>( metadata, &_engine, store ) ;
      PD_CHECK( collPtr, SDB_OOM, error, PDERROR, "Failed to create collection object" ) ;

      {
         ossScopedRWLock lock( &_collMapMutex, EXCLUSIVE ) ;
         _collMap.insert( make_pair( metadata.getKey(), collPtr ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CREATECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_DROPCL, "_dmsWTStorageService::dropCL" )
   INT32 _dmsWTStorageService::dropCL( const dmsCLMetadata &metadata,
                                       const dmsDropCLOptions &options,
                                       IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_DROPCL ) ;

      ossPoolString dataURI ;

      rc = _buildDataURI( metadata.getCSUID(),
                          metadata.getCLOrigInnerID(),
                          metadata.getCLOrigLID(),
                          dataURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger data URI, rc: %d", rc ) ;

      rc = _engine.dropStore( dataURI.c_str(), "force,checkpoint_wait=false" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop WiredTiger data store, rc: %d", rc ) ;

      {
         ossScopedRWLock lock( &_collMapMutex, EXCLUSIVE ) ;
         _collMap.erase( metadata.getKey() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_DROPCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_TRUNCCL, "_dmsWTStorageService::truncateCL" )
   INT32 _dmsWTStorageService::truncateCL( const dmsCLMetadata &metadata,
                                           const dmsTruncCLOptions &options,
                                           IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_TRUNCCL ) ;

      ossPoolString dataURI ;

      rc = _buildDataURI( metadata.getCSUID(),
                          metadata.getCLOrigInnerID(),
                          metadata.getCLOrigLID(),
                          dataURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger data URI, rc: %d", rc ) ;

      rc = _engine.truncateStore( dataURI.c_str(), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate WiredTiger data store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_TRUNCCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_CREATEIDX, "_dmsWTStorageService::createIdx" )
   INT32 _dmsWTStorageService::createIdx( const dmsIdxMetadata &metadata,
                                          const dmsCreateIdxOptions &options,
                                          IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_CREATEIDX ) ;

      dmsWTStore store ;
      ossPoolString idxURI, idxConfig ;

      rc = _buildIdxURI( metadata.getCSUID(),
                         metadata.getCLOrigInnerID(),
                         metadata.getCLOrigLID(),
                         metadata.getIdxInnerID(),
                         idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger index URI, "
                   "rc: %d", rc ) ;

      rc = _buildIdxConfigString( _engineOptions, options, idxConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger index config "
                   "string, rc: %d", rc ) ;

      rc = _engine.createStore( idxURI.c_str(), idxConfig.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create WiredTiger index store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_CREATEIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_DROPIDX, "_dmsWTStorageService::dropIdx" )
   INT32 _dmsWTStorageService::dropIdx( const dmsIdxMetadata &metadata,
                                        const dmsDropIdxOptions &options,
                                        IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_DROPIDX ) ;

      ossPoolString idxURI ;

      rc = _buildIdxURI( metadata.getCSUID(),
                         metadata.getCLOrigInnerID(),
                         metadata.getCLOrigLID(),
                         metadata.getIdxInnerID(),
                         idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger index URI, "
                   "rc: %d", rc ) ;

      rc = _engine.dropStore( idxURI.c_str(), "force,checkpoint_wait=false" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop WiredTiger index store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_DROPIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_TRUNCIDX, "_dmsWTStorageService::truncateIdx" )
   INT32 _dmsWTStorageService::truncateIdx( const dmsIdxMetadata &metadata,
                                            const dmsTruncateIdxOptions &options,
                                            IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_TRUNCIDX ) ;

      ossPoolString idxURI ;

      rc = _buildIdxURI( metadata.getCSUID(),
                         metadata.getCLOrigInnerID(),
                         metadata.getCLOrigLID(),
                         metadata.getIdxInnerID(),
                         idxURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger index URI, rc: %d", rc ) ;

      rc = _engine.truncateStore( idxURI.c_str(), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate WiredTiger idnex store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_TRUNCIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_GETCOLL, "_dmsWTStorageService::getCollection" )
   INT32 _dmsWTStorageService::getCollection( const dmsCLMetadataKey &metadataKey,
                                              IExecutor *executor,
                                              shared_ptr< ICollection > &collPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_GETCOLL ) ;

      collPtr.reset() ;

      {
         ossScopedRWLock lock( &_collMapMutex, SHARED ) ;
         _DMS_WT_COLL_MAP_ITER iter = _collMap.find( metadataKey ) ;
         if ( iter != _collMap.end() )
         {
            collPtr = iter->second ;
         }
      }

      PD_CHECK( collPtr, SDB_DMS_NOTEXIST, error, PDEVENT,
                "Failed to get collection, collection [UID: %llx, LID: %x] not exist",
                metadataKey.getCLOrigUID(), metadataKey.getCLOrigLID() ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_GETCOLL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE_LOADCOLL, "_dmsWTStorageService::loadCollection" )
   INT32 _dmsWTStorageService::loadCollection( const dmsCLMetadata &metadata,
                                               IExecutor *executor,
                                               shared_ptr< ICollection > &collPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE_LOADCOLL ) ;

      dmsWTStore store ;
      ossPoolString dataURI ;

      collPtr.reset() ;

      rc = _buildDataURI( metadata.getCSUID(),
                          metadata.getCLOrigInnerID(),
                          metadata.getCLOrigLID(),
                          dataURI ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build WiredTiger data URI, rc: %d", rc ) ;

      rc = _engine.loadStore( dataURI.c_str(), store ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load WiredTiger data store [%s], rc: %d",
                   dataURI.c_str(), rc ) ;

      collPtr = make_shared<dmsWTCollection>( metadata, &_engine, store ) ;
      PD_CHECK( collPtr, SDB_OOM, error, PDERROR, "Failed to create collection object" ) ;

      {
         ossScopedRWLock lock( &_collMapMutex, EXCLUSIVE ) ;
         _collMap.insert( make_pair( metadata.getKey(), collPtr ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE_LOADCOLL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__INITENGINEOPTIONS, "_dmsWTStorageService::_initEngineOptions" )
   INT32 _dmsWTStorageService::_initEngineOptions( dmsWTEngineOptions &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__INITENGINEOPTIONS ) ;

      pmdOptionsCB *optionCB = pmdGetOptionCB() ;

      boost::filesystem::path dbPath( optionCB->getDbPath() ) ;
      options.setDBPath( dbPath ) ;

      options.setCacheSizeMB( optionCB->getWTCacheSize() ) ;
      options.setEvictTarget( optionCB->getWTEvictTarget() ) ;
      options.setEvictTrigger( optionCB->getWTEvictTrigger() ) ;
      options.setEvictDirtyTarget( optionCB->getWTEvictDirtyTarget() ) ;
      options.setEvictDirtyTrigger( optionCB->getWTEvictDirtyTrigger() ) ;
      options.setEvictUpdatesTarget( optionCB->getWTEvictUpdatesTarget() ) ;
      options.setEvictUpdatesTrigger( optionCB->getWTEvictUpdatesTrigger() ) ;
      options.setEvictThreadsMin( optionCB->getWTEvictThreadsMin() ) ;
      options.setEvictThreadsMax( optionCB->getWTEvictThreadsMax() ) ;
      options.setCheckPointInterval( optionCB->getWTCheckPointInterval() ) ;
      options.setCheckPointLogSize( optionCB->getWTCheckPointLogSize() ) ;

      options.fixOptions() ;


      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__INITENGINEOPTIONS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__CHKDBPATH, "_dmsWTStorageService::_checkDBPath" )
   INT32 _dmsWTStorageService::_checkDBPath( const boost::filesystem::path &dbPath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__CHKDBPATH ) ;

      boost::filesystem::path journalPath = dbPath / "journal" ;
      if ( boost::filesystem::exists( journalPath ) )
      {
         goto done ;
      }
      try
      {
            boost::filesystem::create_directory( journalPath ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to create journal directory, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__CHKDBPATH, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__BLDCONFSTR, "_dmsWTStorageService::_buildConfigString" )
   INT32 _dmsWTStorageService::_buildConfigString( const dmsWTEngineOptions &options,
                                                   ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__BLDCONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "create," ;
         ss << "cache_size=" << options.getCacheSizeMB() << "MB," ;
         ss << "session_max=33000," ;
         ss << "eviction=(threads_min=" << options.getEvictThreadsMin()
            << ",threads_max=" << options.getEvictThreadsMax() << ")," ;
         ss << "eviction_target=" << options.getEvictTarget() << "," ;
         ss << "eviction_trigger=" << options.getEvictTrigger() << "," ;
         ss << "eviction_dirty_target=" << options.getEvictDirtyTarget() << "," ;
         ss << "eviction_dirty_trigger=" << options.getEvictDirtyTrigger() << "," ;
         ss << "eviction_updates_target=" << options.getEvictUpdatesTarget() << "," ;
         ss << "eviction_updates_trigger=" << options.getEvictUpdatesTrigger() << "," ;
         ss << "config_base=false," ;
         ss << "statistics=(fast)," ;
         ss << "log=(enabled=true,remove=true,path=journal,compressor=snappy)," ;
         ss << "builtin_extension_config=(zstd=(compression_level=6))," ;
         ss << "file_manager=(close_idle_time=600,close_scan_interval=10,close_handle_minimum=2000)," ;
         ss << "statistics_log=(wait=0)," ;
         ss << "json_output=(error,message)" ;

         configString = ss.str() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build WiredTiger connection string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__BLDCONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__BLDDATACONFSTR, "_dmsWTStorageService::_buildDataConfigString" )
   INT32 _dmsWTStorageService::_buildDataConfigString( const dmsWTEngineOptions &options,
                                                       const dmsCreateCLOptions &createCLOptions,
                                                       ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__BLDDATACONFSTR ) ;

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
         PD_LOG( PDERROR, "Failed to build WiredTiger data config string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__BLDDATACONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__BLDDATAURI, "_dmsWTStorageService::_buildDataURI" )
   INT32 _dmsWTStorageService::_buildDataURI( utilCSUniqueID csUID,
                                              utilCLInnerID clInnerID,
                                              UINT32 clLID,
                                              ossPoolString &dataURI )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__BLDDATAURI ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "table:" ;
         dmsWTBuildDataIdent( csUID, clInnerID, clLID, ss ) ;
         dataURI = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build WiredTiger data URI, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__BLDDATAURI, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__BLDIDXCONFSTR, "_dmsWTStorageService::_buildIdxConfigString" )
   INT32 _dmsWTStorageService::_buildIdxConfigString( const dmsWTEngineOptions &options,
                                                      const dmsCreateIdxOptions &createCLOptions,
                                                      ossPoolString &configString )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__BLDIDXCONFSTR ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "type=file,internal_page_max=16k,leaf_page_max=16k,";
         ss << "checksum=on,";
         ss << "prefix_compression=true,";
         ss << "key_format=u,";
         ss << "value_format=u,";
         ss << "app_metadata=(formatVersion=" << DMS_WT_FORMART_VER_CUR << ")," ;
         ss << "log=(enabled=true)," ;

         configString = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build WiredTiger index config string, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__BLDIDXCONFSTR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__BLDIDXURI, "_dmsWTStorageService::_buildIdxURI" )
   INT32 _dmsWTStorageService::_buildIdxURI( utilCSUniqueID csUID,
                                             utilCLInnerID clInnerID,
                                             UINT32 clLID,
                                             utilIdxInnerID idxInnerID,
                                             ossPoolString &idxURI )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__BLDIDXURI ) ;

      try
      {
         ossPoolStringStream ss ;

         ss << "table:" ;
         dmsWTBuildIndexIdent( csUID, clInnerID, clLID, idxInnerID, ss ) ;
         idxURI = ss.str();
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build WiredTiger index URI, "
                 "occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__BLDIDXURI, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGESERVICE__DUMPURILISTBYCS, "_dmsWTStorageService::_dumpURIListByCS" )
   INT32 _dmsWTStorageService::_dumpURIListByCS( utilCSUniqueID csUID,
                                                 ossPoolList< ossPoolString > &uriList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGESERVICE__DUMPURILISTBYCS ) ;

      ossPoolStringStream prefixSS ;
      ossPoolString prefix ;

      prefixSS << "table:" ;
      dmsWTBuildIdentPrefix( csUID, prefixSS ) ;
      prefix = prefixSS.str();

      rc = _engine.dumpURIListByPrefix( prefix.c_str(), uriList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to dump WiredTiger URI list, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGESERVICE__DUMPURILISTBYCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
