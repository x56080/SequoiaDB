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

   Source File Name = dmsCB.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsMmapEngine.hpp"
#include "dmsCB.hpp"
#include "dmsTrace.hpp"
#include "rtnCB.hpp"
#include "utilRenameLogger.hpp"

namespace engine
{
#define DMS_RENAME_BLOCKWRITE_INTERAL ( 0.1 * OSS_ONE_SEC )
#define DMS_RENAME_BLOCKWRITE_TIMES ( 30 )

   enum DMS_LOCK_LEVEL
   {
      DMS_LOCK_NONE = 0,
      DMS_LOCK_WRITE = 1, // for writable
      DMS_LOCK_WHOLE = 2  // for backup or reorg
   };

   _SDB_DMSCB::_SDB_DMSCB()
   : _stateMtx( MON_LATCH_DMSCB_STATEMTX )
   , _writeCounter( 0 )
   , _dmsCBState( DMS_STATE_NORMAL )
   , _tempSUMgr( this )
   , _statSUMgr( this )
   , _rbsSUMgr()
   , _localSUMgr( this )
   {
      _blockEvent.signal();
   }

   _SDB_DMSCB::~_SDB_DMSCB()
   {
      // make sure dms control block is finalized
      fini();
   }

   INT32 _SDB_DMSCB::init()
   {
      INT32 rc = SDB_OK;
      if ( pmdGetKRCB()->isRestore() )
      {
         goto done;
      }
      // mmap engine
      {
         std::unique_ptr< dmsMmapEngine > mmap{ SDB_OSS_NEW dmsMmapEngine };
         _mmapEngine = mmap.get();
         rc = mmap->open( _cm );
         PD_RC_CHECK( rc, PDERROR, "failed to open mmap engine, rc: %d", rc );

         _engineSocket.addEngine( std::move( mmap ) );
         PD_LOG( PDDEBUG, "add mmap engine succesfully" );
      }

      // 2. init temp cs mgr
      rc = _tempSUMgr.init();
      PD_RC_CHECK( rc, PDERROR, "Failed to init temp cb, rc: %d", rc );

      // 3. init stat cs cb
      if ( SDB_ROLE_DATA == pmdGetDBRole() || SDB_ROLE_STANDALONE == pmdGetDBRole() )
      {
         rc = _statSUMgr.init();
         PD_RC_CHECK( rc, PDERROR, "Failed to init stat cb, rc: %d", rc );
      }

      rc = _localSUMgr.init();
      PD_RC_CHECK( rc, PDERROR, "Failed to init local su manager, rc: %d", rc );

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::active()
   {
      INT32 rc = SDB_OK;

      if ( _statSUMgr.initialized() )
      {
         rc = regHandler( DMS_ENGINE_MMAP, &_statSUMgr );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to register event handler of "
                      "statistics manager to DMS, rc: %d",
                      rc );
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::deactive()
   {
      if ( _statSUMgr.initialized() )
      {
         unregHandler( DMS_ENGINE_MMAP, &_statSUMgr );
      }
      return SDB_OK;
   }

   INT32 _SDB_DMSCB::fini()
   {
      INT32 rc = SDB_OK;
      // check if MVCC is supported
      // finish and flush Rollback Segment CS mgr. We must do it here
      // instead of fini because we need DPS to flush logs to disk.
      // see the order in _SDB_KRCB::destroy. DPS is alway the first
      // to start and last to shut down.
      if ( pmdGetOptionCB()->mvccOn() )
      {
         INT32 tmpRC = _rbsSUMgr.fini();
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Finish RBS failed, rc: %d", tmpRC );
         }
      }

      dmsCloseDBOptions options;
      rc = _engineSocket.closeEngines( pmdGetThreadEDUCB(), options );
      PD_RC_CHECK(rc, PDERROR, "failed to close engines");
      _localSUMgr.fini();
      _tempSUMgr.fini();

   done:
      return rc;
   error:
      goto done;
   }

   void _SDB_DMSCB::onConfigChange()
   {
      _getMmapEngine()->onConfigChange();
   }

   dmsTempSUMgr *_SDB_DMSCB::getTempSUMgr()
   {
      return &_tempSUMgr;
   }

   dmsStatSUMgr *_SDB_DMSCB::getStatSUMgr()
   {
      return &_statSUMgr;
   }

   dmsRBSMgr *_SDB_DMSCB::getRBSSUMgr()
   {
      return &_rbsSUMgr;
   }

   dmsLocalSUMgr *_SDB_DMSCB::getLocalSUMgr()
   {
      return &_localSUMgr;
   }

   INT32 _SDB_DMSCB::createCS( IExecutor *executor,
                               const CHAR *name,
                               utilCSUniqueID uniqueId,
                               const dmsCreateCSOptions &o,
                               const bson::BSONObj &adjunct )
   {
      INT32 rc = SDB_OK;
      BOOLEAN isWritable = FALSE;
      IDataStorageEngine *engine = nullptr;
      dmsSuConstraintMap::CONTEXT_CREATE ctx = nullptr;
      DMS_SU_DESCRIPTOR desc = nullptr;
      pmdEDUCB *cb = dynamic_cast< pmdEDUCB * >( executor );
      if ( executor )
      {
         SDB_ASSERT( cb, "can not be nullptr" );
      }

      // make sure the collectionspace length is not out of range
      UINT32 length = ossStrlen( name );
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG( PDERROR, "Invalid length for collectionspace: %s, rc: %d", name, rc );
         rc = SDB_INVALIDARG;
         goto error;
      }

      if ( dmsCheckCSName( name, o.sysCall ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Collection space name is invalid[%s], rc: %d", name, rc );
         goto error;
      }

      rc = writable( cb );
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc );
      isWritable = TRUE;

      rc = _cm.prepareToCreate( name, uniqueId, ctx );
      PD_RC_CHECK( rc, PDERROR, "failed to prepare to create collection space[%s], rc: %d", name,
                   rc );
      engine = _engineSocket.getEngine( o.etype );
      SDB_ASSERT( engine, "can not be nullptr" );
      rc = engine->createCS( executor, name, uniqueId, o, adjunct, desc );
      if ( rc == SDB_OK )
      {
         ctx->commit( desc );
      }
      else
      {
         ctx->abort();
         PD_LOG( PDERROR, "failed to create collection space[%s] on engine[type: %d], rc: %d", name,
                 o.etype, rc );
         goto error;
      }
   done:
      if ( isWritable )
      {
         writeDown( cb );
      }
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::dropCS( IExecutor *executor,
                             const CHAR *name,
                             const dmsRemoveCSOptions &options )
   {
      INT32 rc = SDB_OK;
      BOOLEAN isWritable = FALSE;
      IDataStorageEngine *engine = nullptr;
      dmsSuConstraintMap::CONTEXT_DROP ctx;
      pmdEDUCB *cb = dynamic_cast< pmdEDUCB * >( executor );
      if ( executor )
      {
         SDB_ASSERT( cb, "can not be nullptr" );
      }

      // make sure the collectionspace length is not out of range
      UINT32 length = ossStrlen( name );
      if ( length <= 0 || length > DMS_SU_NAME_SZ )
      {
         PD_LOG( PDERROR, "Invalid length for collectionspace: %s, rc: %d", name, rc );
         rc = SDB_INVALIDARG;
         goto error;
      }
      if ( dmsCheckCSName( name, TRUE ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Collectionspace name is invalid[%s], rc: %d", name, rc );
         goto error;
      }

      rc = writable( cb );
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc );
      isWritable = TRUE;

      rc = _cm.prepareToDrop( name, ctx );
      PD_RC_CHECK( rc, PDERROR, "failed to prepare to drop collection space[%s], rc: %d", name,
                   rc );
      engine = _engineSocket.getEngine( ctx->getDescriptor()->engine );
      SDB_ASSERT( engine, "can not be nullptr" );
      rc = engine->removeCS( executor, name, options );
      if ( rc == SDB_OK )
      {
         ctx->commit();
      }
      else
      {
         DMS_ENGINE_TYPE engineType = ctx->getDescriptor()->engine;
         ctx->abort();
         PD_LOG( PDERROR, "failed to drop collection space[%s] on engine[type: %d], rc: %d", name,
                 engineType, rc );
         goto error;
      }

   done:
      if ( isWritable )
      {
         writeDown( cb );
      }
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::renameCS( IExecutor *executor,
                               const CHAR *oldName,
                               const CHAR *newName,
                               BOOLEAN isBlockWrite )
   {
      INT32 rc = SDB_OK;
      BOOLEAN lockDMS = FALSE;
      utilRenameLogger logger;
      IDataStorageEngine *engine = nullptr;
      dmsSuConstraintMap::CONTEXT_RENAME ctx;
      DMS_SU_DESCRIPTOR desc = nullptr;
      pmdEDUCB *cb = dynamic_cast< pmdEDUCB * >( executor );
      if ( executor )
      {
         SDB_ASSERT( cb, "can not be nullptr" );
      }

      /// dms lock
      if ( isBlockWrite )
      {
         // When two threads concurrently do rename, blockWrite() will report
         // -148. We retry multiple times to reduce the error.
         INT16 i = 0;
         while ( ( rc = blockWrite( cb ) ) && ( i < DMS_RENAME_BLOCKWRITE_TIMES ) )
         {
            ossSleep( DMS_RENAME_BLOCKWRITE_INTERAL );
            i++;
         }
         PD_RC_CHECK( rc, PDERROR, "Block dms write failed, rc: %d", rc );
         PD_LOG( PDINFO, "Block write operation succeed" );
      }
      else
      {
         rc = writable( cb );
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc );
      }
      lockDMS = TRUE;

      /// log to .SEQUOIADB_RENAME_INFO
      {
         utilRenameLog aLog( oldName, newName );

         rc = logger.init();
         PD_RC_CHECK( rc, PDERROR, "Failed to init rename logger, rc: %d", rc );

         rc = logger.log( aLog );
         PD_RC_CHECK( rc, PDERROR, "Failed to log rename info to file, rc: %d", rc );
      }

      rc = _cm.prepareToRename( oldName, newName, ctx );
      PD_RC_CHECK( rc, PDERROR, "failed to prepare to rename collection space[%s] to [%s], rc: %d",
                   oldName, newName, rc );
      engine = _engineSocket.getEngine( ctx->getOldDescriptor()->engine );
      SDB_ASSERT( engine, "can not be nullptr" );
      rc = engine->renameCS( executor, oldName, newName, isBlockWrite, desc );
      if ( rc == SDB_OK )
      {
         ctx->commit( desc );
      }
      else
      {
         DMS_ENGINE_TYPE engineType = ctx->getOldDescriptor()->engine;
         ctx->abort();
         PD_LOG( PDERROR, "failed to rename collection space[%s] on engine[type: %d], rc: %d",
                 oldName, engineType, rc );
         goto error;
      }

      PD_LOG( PDEVENT, "Rename cs[%s] to [%s] succeed", oldName, newName );

   done:
      /// remove .SEQUOIADB_RENAME_INFO
      rc = logger.clear();
      PD_RC_CHECK( rc, PDERROR, "Failed to clear rename info, rc: %d", rc );

      if ( lockDMS )
      {
         if ( isBlockWrite )
         {
            unblockWrite( cb );
            PD_LOG( PDINFO, "Unblock write operation succeed" );
         }
         else
         {
            writeDown( cb );
         }
         lockDMS = FALSE;
      }
      return rc;
   error : {
      INT32 tmpRC = logger.clear();
      if ( tmpRC )
      {
         PD_LOG( PDERROR, "Failed to clear rename info, rc: %d", tmpRC );
      }
   }
      goto done;
   }

   INT32 _SDB_DMSCB::openCL( IExecutor *executor,
                             const CHAR *clFullName,
                             const dmsOpenCLOptions &o,
                             DATA_COLLECTION_PTR &ptr )
   {
      INT32 rc = SDB_OK;
      utilStringView csName( clFullName, strchr( clFullName, '.' ) - clFullName );
      DMS_SU_DESCRIPTOR desc = _cm.getSuDescriptor( csName );
      if ( desc )
      {
         IDataStorageEngine *engine = nullptr;
         engine = _engineSocket.getEngine( desc->engine );
         SDB_ASSERT( engine, "can not be nullptr" );
         rc = engine->openCL( executor, clFullName, o, ptr );
         PD_RC_CHECK( rc, PDERROR, "failed to open collection[%s], rc: %d", clFullName, rc );
      }
      else
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::openCL( IExecutor *executor,
                             utilCLUniqueID uniqueId,
                             const dmsOpenCLOptions &o,
                             DATA_COLLECTION_PTR &ptr )
   {
      INT32 rc = SDB_OK;
      utilCSUniqueID csUID = utilGetCSUniqueID( uniqueId );
      DMS_SU_DESCRIPTOR desc = _cm.getSuDescriptor( csUID );
      if ( desc )
      {
         IDataStorageEngine *engine = nullptr;
         engine = _engineSocket.getEngine( desc->engine );
         SDB_ASSERT( engine, "can not be nullptr" );
         rc = engine->openCL( executor, uniqueId, o, ptr );
         PD_RC_CHECK( rc, PDERROR, "failed to open collection[%s], rc: %d", uniqueId, rc );
      }
      else
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::createCL( IExecutor *executor,
                               const CHAR *clFullName,
                               utilCLUniqueID clUniqueID,
                               const dmsCreateCLOptions &o,
                               const bson::BSONObj &adjunct )
   {
      INT32 rc = SDB_OK;
      BOOLEAN isWritable = FALSE;
      pmdEDUCB *cb = dynamic_cast< pmdEDUCB * >( executor );
      if ( executor )
      {
         SDB_ASSERT( cb, "can not be nullptr" );
      }

      rc = writable( cb );
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc );
      isWritable = TRUE;

      {
         utilStringView csName( clFullName, strchr( clFullName, '.' ) - clFullName );
         DMS_SU_DESCRIPTOR desc = _cm.getSuDescriptor( csName );
         if ( desc )
         {
            IDataStorageEngine *engine = nullptr;
            engine = _engineSocket.getEngine( desc->engine );
            SDB_ASSERT( engine, "can not be nullptr" );
            rc = engine->createCL( executor, clFullName, clUniqueID, o, adjunct );
            PD_RC_CHECK( rc, PDERROR, "failed to create collection[%s], rc: %d", clFullName, rc );
         }
         else
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
      }
   done:
      if ( isWritable )
      {
         writeDown( cb );
      }
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::dropCL( IExecutor *executor,
                             const CHAR *clFullName,
                             const dmsRemoveCLOptions &o )
   {
      INT32 rc = SDB_OK;
      BOOLEAN isWritable = FALSE;
      pmdEDUCB *cb = dynamic_cast< pmdEDUCB * >( executor );
      if ( executor )
      {
         SDB_ASSERT( cb, "can not be nullptr" );
      }

      if ( dmsCheckFullCLName( clFullName, TRUE ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Collection name is invalid[%s], rc: %d", clFullName, rc );
         goto error;
      }

      rc = writable( cb );
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc: %d", rc );
      isWritable = TRUE;

      {
         utilStringView csName( clFullName, strchr( clFullName, '.' ) - clFullName );
         DMS_SU_DESCRIPTOR desc = _cm.getSuDescriptor( csName );
         if ( desc )
         {
            IDataStorageEngine *engine = nullptr;
            engine = _engineSocket.getEngine( desc->engine );
            SDB_ASSERT( engine, "can not be nullptr" );
            rc = engine->removeCL( executor, clFullName, o );
            PD_RC_CHECK( rc, PDERROR, "failed to create collection[%s], rc: %d", clFullName, rc );
         }
         else
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
      }
   done:
      if ( isWritable )
      {
         writeDown( cb );
      }
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::nameToSUAndLock( const CHAR *pName,
                                      dmsStorageUnitID &suID,
                                      _dmsStorageUnit **su,
                                      OSS_LATCH_MODE lockType,
                                      INT32 millisec )
   {
      return _getMmapEngine()->nameToSUAndLock( pName, suID, su, lockType, millisec );
   }

   INT32 _SDB_DMSCB::idToSUAndLock( utilCSUniqueID csUniqueID,
                                    dmsStorageUnitID &suID,
                                    _dmsStorageUnit **su,
                                    OSS_LATCH_MODE lockType,
                                    INT32 millisec )
   {
      return _getMmapEngine()->idToSUAndLock( csUniqueID, suID, su );
   }

   INT32 _SDB_DMSCB::verifySUAndLock( const dmsEventSUItem *pSUItem,
                                      _dmsStorageUnit **ppSU,
                                      OSS_LATCH_MODE lockType,
                                      INT32 millisec )
   {
      return _getMmapEngine()->verifySUAndLock( pSUItem, ppSU, lockType, millisec );
   }

   INT32 _SDB_DMSCB::nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      desc = _cm.getSuDescriptor( pName );
      if ( !desc )
      {
         rc = SDB_DMS_CS_NOTEXIST;
         PD_LOG( PDERROR, "the collection space[%s] does not exist,rc: %d", pName, rc );
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 _SDB_DMSCB::getNullCSUniqueIDCnt() const
   {
      return _cm.getNullCSUniqueID();
   }

   _dmsStorageUnit *_SDB_DMSCB::suLock( dmsStorageUnitID suID )
   {
      return _getMmapEngine()->suLock( suID );
   }

   void _SDB_DMSCB::suUnlock( dmsStorageUnitID suID, OSS_LATCH_MODE lockType )
   {
      return _getMmapEngine()->suUnlock( suID, lockType );
   }

   INT32 _SDB_DMSCB::changeUniqueID( const CHAR *csName,
                                     utilCSUniqueID csUniqueID,
                                     const BSONObj &clInfoObj,
                                     BOOLEAN changeOtherCL,
                                     const ossPoolVector< BSONObj > *pIdxInfoVec,
                                     BOOLEAN changeIdx,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB,
                                     BOOLEAN isLoadCS )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_CHANGE_UNIQUE_ID ctx = nullptr;
      IDataStorageEngine *engine = nullptr;

      rc = _cm.prepareToChangeUniqueID( csName, csUniqueID, ctx );
      PD_RC_CHECK( rc, PDERROR,
                   "failed to prepare to change unique id of collection space[%s], rc: %d", csName,
                   rc );
      if ( !ctx )
      {
         PD_LOG( PDDEBUG, "the current unique id of collection space[%s] is already equal to [%d]",
                 csName, csUniqueID );
         goto done;
      }

      engine = _engineSocket.getEngine( ctx->getOldDescriptor()->engine );
      SDB_ASSERT( engine && engine->getEngineType() == DMS_ENGINE_MMAP,
                  "only support on mmap engine" );

      rc = _getMmapEngine()->changeUniqueID( csName, csUniqueID, clInfoObj, changeOtherCL,
                                             pIdxInfoVec, changeIdx, cb, dpsCB );
      if ( rc == SDB_OK )
      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         rc = _getMmapEngine()->nameToSuDescriptor( csName, desc );
         PD_RC_CHECK( rc, PDERROR, "failed to get descriptor of collection space[%s], rc: %d",
                      csName, rc );
         ctx->commit( desc );
      }
      else
      {
         ctx->abort();
         PD_LOG( PDERROR, "failed to change unique id on collection space[%s] to id[%d], rc: %d",
                 csName, csUniqueID, rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::addCollectionSpace( const CHAR *pName,
                                         UINT32 topSequence,
                                         _dmsStorageUnit *su,
                                         _pmdEDUCB *cb,
                                         SDB_DPSCB *dpsCB,
                                         BOOLEAN isCreate )
   {
      INT32 rc = SDB_OK;
      DMS_SU_DESCRIPTOR desc = nullptr;
      rc =
         _getMmapEngine()->addCollectionSpace( pName, topSequence, su, cb, dpsCB, isCreate, desc );
      PD_RC_CHECK( rc, PDERROR, "failed to add collection space[%s], rc: %d", pName, rc );

      SDB_ASSERT( desc && desc->isValid(), "must be valid" );
      rc = _cm.addSuDescriptor( desc );
      PD_RC_CHECK( rc, PDERROR, "failed to add descriptor of collection space[%s], rc: %d", pName,
                   rc );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::dropCollectionSpace( const CHAR *pName,
                                          _pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB,
                                          dmsDropCSOptions *options )
   {
      INT32 rc = SDB_OK;
      rc = _getMmapEngine()->dropCollectionSpace( pName, cb, dpsCB, options );
      PD_RC_CHECK( rc, PDERROR, "failed to drop collection space[%s], rc: %d", pName, rc );
      _cm.removeSuDescriptor( pName );

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::dropEmptyCollectionSpace( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      rc = _getMmapEngine()->dropEmptyCollectionSpace( pName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR, "failed to drop empty collection space[%s], rc: %d", pName, rc );
      _cm.removeSuDescriptor( pName );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::unloadCollectonSpace( const CHAR *pName, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      rc =  _getMmapEngine()->unloadCollectonSpace( pName, cb );
      PD_RC_CHECK( rc, PDERROR, "failed to unload collection space[%s], rc: %d", pName, rc );
      _cm.removeSuDescriptor( pName );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::renameCollectionSpace( const CHAR *pName,
                                            const CHAR *pNewName,
                                            _pmdEDUCB *cb,
                                            SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_RENAME ctx = nullptr;
      IDataStorageEngine *engine = nullptr;
      
      rc = _cm.prepareToRename(pName, pNewName, ctx);
      PD_RC_CHECK( rc, PDERROR, "failed to prepare to rename collection space[%s] to [%s], rc: %d",
                   pName, pNewName, rc );
      SDB_ASSERT( ctx, "must be valid");
      engine = _engineSocket.getEngine( ctx->getOldDescriptor()->engine );
      SDB_ASSERT( engine && engine->getEngineType() == DMS_ENGINE_MMAP,
                  "only support on mmap engine" );

      rc = _getMmapEngine()->renameCollectionSpace( pName, pNewName, cb, dpsCB );
      if ( rc == SDB_OK )
      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         rc = _getMmapEngine()->nameToSuDescriptor( pNewName, desc );
         PD_RC_CHECK( rc, PDERROR, "failed to get descriptor of collection space[%s], rc: %d",
                      pNewName, rc );
         ctx->commit( desc );
      }
      else
      {
         ctx->abort();
         PD_LOG( PDERROR, "failed to rename collection space[%s] to [%s], rc: %d", pName, pNewName,
                 rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::renameCollectionSpaceP1( const CHAR *pName,
                                              const CHAR *pNewName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      return _getMmapEngine()->renameCollectionSpaceP1( pName, pNewName, cb, dpsCB );
   }

   INT32 _SDB_DMSCB::renameCollectionSpaceP1Cancel( const CHAR *pName,
                                                    const CHAR *pNewName,
                                                    _pmdEDUCB *cb,
                                                    SDB_DPSCB *dpsCB )
   {
      return _getMmapEngine()->renameCollectionSpaceP1Cancel( pName, pNewName, cb, dpsCB );
   }

   INT32 _SDB_DMSCB::renameCollectionSpaceP2( const CHAR *pName,
                                              const CHAR *pNewName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_RENAME ctx = nullptr;
      IDataStorageEngine *engine = nullptr;
      
      rc = _cm.prepareToRename(pName, pNewName, ctx);
      PD_RC_CHECK( rc, PDERROR, "failed to prepare to rename collection space[%s] to [%s], rc: %d",
                   pName, pNewName, rc );
      
      engine = _engineSocket.getEngine( ctx->getOldDescriptor()->engine );
      SDB_ASSERT( engine && engine->getEngineType() == DMS_ENGINE_MMAP,
                  "only support on mmap engine" );
      rc = _getMmapEngine()->renameCollectionSpaceP2( pName, pNewName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR, "failed to rename collection space[%s] to name[%s] in phase 2, rc: %d", pName,
                   pNewName, rc );
      if ( rc == SDB_OK )
      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         rc = _getMmapEngine()->nameToSuDescriptor( pNewName, desc );
         PD_RC_CHECK( rc, PDERROR, "failed to get descriptor of collection space[%s], rc: %d",
                      pNewName, rc );
         ctx->commit( desc );
      }
      else
      {
         ctx->abort();
         PD_LOG( PDERROR, "failed to rename collection space[%s] to [%s], rc: %d", pName, pNewName,
                 rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::restoreCollectionSpace( const CHAR *pName )
   {
      return _getMmapEngine()->restoreCollectionSpace( pName );
   }

   INT32 _SDB_DMSCB::returnCollectionSpaceP1( dmsReturnOptions &options,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      return _getMmapEngine()->returnCollectionSpaceP1( options, cb, dpsCB );
   }
   INT32 _SDB_DMSCB::returnCollectionSpaceP1Cancel( dmsReturnOptions &options,
                                                    _pmdEDUCB *cb,
                                                    SDB_DPSCB *dpsCB )
   {
      return _getMmapEngine()->returnCollectionSpaceP1Cancel( options, cb, dpsCB );
   }

   INT32 _SDB_DMSCB::returnCollectionSpaceP2( dmsReturnOptions &options,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_RENAME ctx = nullptr;
      IDataStorageEngine *engine = nullptr;
      const CHAR *oldName = options._recycleItem.getRecycleName();
      const CHAR *newName = options._recycleItem.getOriginName();
      rc = _cm.prepareToRename(oldName, newName, ctx );
      PD_RC_CHECK( rc, PDERROR, "failed to prepare to rename collection space[%s] to [%s], rc: %d",
                   oldName, newName, rc );
      SDB_ASSERT( ctx, "must be valid");
      engine = _engineSocket.getEngine( ctx->getOldDescriptor()->engine );
      SDB_ASSERT( engine && engine->getEngineType() == DMS_ENGINE_MMAP,
                  "only support on mmap engine" );

      rc = _getMmapEngine()->returnCollectionSpaceP2( options, cb, dpsCB );
      if ( rc == SDB_OK )
      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         rc = _getMmapEngine()->nameToSuDescriptor( newName, desc );
         PD_RC_CHECK( rc, PDERROR, "failed to get descriptor of collection space[%s], rc: %d",
                      newName, rc );
         ctx->commit( desc );
      }
      else
      {
         ctx->abort();
         PD_LOG( PDERROR, "failed to return collection space[%s], rc: %d", newName, rc );
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _SDB_DMSCB::returnCollectionSpace( dmsReturnOptions &options,
                                            _pmdEDUCB *cb,
                                            SDB_DPSCB *dpsCB )
   {
      return _getMmapEngine()->returnCollectionSpace( options, cb, dpsCB );
   }

   INT32 _SDB_DMSCB::dumpInfo( MON_CL_SIM_LIST &collectionList, BOOLEAN sys )
   {
      return _getMmapEngine()->dumpInfo( collectionList, sys );
   }

   INT32 _SDB_DMSCB::dumpInfo( MON_CS_SIM_LIST &csList,
                               BOOLEAN sys,
                               BOOLEAN dumpCL,
                               BOOLEAN dumpIdx )
   {
      return _getMmapEngine()->dumpInfo( csList, sys, dumpCL, dumpIdx );
   }

   INT32 _SDB_DMSCB::dumpInfo( MON_CL_LIST &collectionList, BOOLEAN sys )
   {
      return _getMmapEngine()->dumpInfo( collectionList, sys );
   }

   INT32 _SDB_DMSCB::dumpInfo( MON_CS_LIST &csList, BOOLEAN sys )
   {
      return _getMmapEngine()->dumpInfo( csList, sys );
   }

   INT32 _SDB_DMSCB::dumpInfo( MON_SU_LIST &storageUnitList, BOOLEAN sys )
   {
      return _getMmapEngine()->dumpInfo( storageUnitList, sys );
   }

   void _SDB_DMSCB::dumpInfo( INT64 &totalFileSize )
   {
      return _getMmapEngine()->dumpInfo( totalFileSize );
   }

   void _SDB_DMSCB::dumpPageMapCSInfo( MON_CSNAME_VEC &vecCS )
   {
      return _getMmapEngine()->dumpPageMapCSInfo( vecCS );
   }

   void _SDB_DMSCB::clearSUCaches( UINT32 mask )
   {
      return _getMmapEngine()->clearSUCaches( mask );
   }

   void _SDB_DMSCB::clearSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask )
   {
      return _getMmapEngine()->clearSUCaches( monCSList, mask );
   }

   void _SDB_DMSCB::changeSUCaches( UINT32 mask )
   {
      return _getMmapEngine()->changeSUCaches( mask );
   }

   void _SDB_DMSCB::changeSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask )
   {
      return _getMmapEngine()->changeSUCaches( monCSList, mask );
   }

   INT32 _SDB_DMSCB::dropCollectionSpaceP1( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      return _getMmapEngine()->dropCollectionSpaceP1( pName, cb, dpsCB );
   }

   INT32 _SDB_DMSCB::dropCollectionSpaceP1Cancel( const CHAR *pName,
                                                  _pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      return _getMmapEngine()->dropCollectionSpaceP1Cancel( pName, cb, dpsCB );
   }

   INT32 _SDB_DMSCB::dropCollectionSpaceP2( const CHAR *pName,
                                            _pmdEDUCB *cb,
                                            SDB_DPSCB *dpsCB,
                                            dmsDropCSOptions *options )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_DROP ctx;
      IDataStorageEngine *engine = nullptr;
      rc = _cm.prepareToDrop( pName, ctx );
      PD_RC_CHECK( rc, PDERROR, "failed to prepare to drop collection space[%s], rc: %d", pName,
                   rc );
      engine = _engineSocket.getEngine( ctx->getDescriptor()->engine );
      SDB_ASSERT( engine, "can not be nullptr" );

      rc = _getMmapEngine()->dropCollectionSpaceP2( pName, cb, dpsCB, options );
      if ( rc == SDB_OK )
      {
         ctx->commit();
      }
      else
      {
         DMS_ENGINE_TYPE engineType = ctx->getDescriptor()->engine;
         ctx->abort();
         PD_LOG( PDERROR, "failed to drop collection space[%s] on engine[type: %d], rc: %d", pName,
                 engineType, rc );
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN _SDB_DMSCB::dispatchDictJob( dmsDictJob &job )
   {
      return _getMmapEngine()->dispatchDictJob( job );
   }

   void _SDB_DMSCB::pushDictJob( dmsDictJob job )
   {
      return _getMmapEngine()->pushDictJob( job );
   }

   void _SDB_DMSCB::setIxmKeySorterCreator( dmsIxmKeySorterCreator *creator )
   {
      _getMmapEngine()->setIxmKeySorterCreator( creator );
   }

   INT32 _SDB_DMSCB::createIxmKeySorter( INT64 bufSize,
                                         const _dmsIxmKeyComparer &comparer,
                                         dmsIxmKeySorter **ppSorter )
   {
      return _getMmapEngine()->createIxmKeySorter( bufSize, comparer, ppSorter );
   }
   void _SDB_DMSCB::releaseIxmKeySorter( dmsIxmKeySorter *pSorter )
   {
      _getMmapEngine()->releaseIxmKeySorter( pSorter );
   }

   void _SDB_DMSCB::setScannerCheckerCreator( IDmsScannerCheckerCreator *pCreator )
   {
      _getMmapEngine()->setScannerCheckerCreator( pCreator );
   }

   INT32 _SDB_DMSCB::createScannerChecker( UINT32 suLID,
                                           UINT32 mbLID,
                                           const CHAR *csName,
                                           const CHAR *clShortName,
                                           const CHAR *optrDesc,
                                           _pmdEDUCB *cb,
                                           IDmsScannerChecker **ppChecker )
   {
      return _getMmapEngine()->createScannerChecker( suLID, mbLID, csName, clShortName, optrDesc,
                                                     cb, ppChecker );
   }

   void _SDB_DMSCB::releaseScannerChecker( IDmsScannerChecker *pChecker )
   {
      _getMmapEngine()->releaseScannerChecker( pChecker );
   }

   INT32 _SDB_DMSCB::getMaxDMSLSN( DPS_LSN_OFFSET &maxLsn )
   {
      return _getMmapEngine()->getMaxDMSLSN( maxLsn );
   }

   typedef std::vector< SDB_DMS_CSCB * >::iterator CSCB_ITERATOR;

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_WRITABLE, "_SDB_DMSCB::writable" )
   INT32 _SDB_DMSCB::writable( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_WRITABLE );
      BOOLEAN hasBlock = FALSE;

      BOOLEAN locked = FALSE;

      if ( cb && cb->getLockItem( SDB_LOCK_DMS )->getMode() >= DMS_LOCK_WRITE )
      {
         cb->getLockItem( SDB_LOCK_DMS )->incCount();
         _stateMtx.get();
         ++_writeCounter;
         _stateMtx.release();
         // already writable
         goto done;
      }

   retry:
      _stateMtx.get();
      locked = TRUE;

      switch ( _dmsCBState )
      {
      case DMS_STATE_READONLY : {
         if ( SDB_DB_OFFLINE_BK == PMD_DB_STATUS() )
         {
            rc = SDB_RTN_IN_BACKUP;
         }
         else if ( SDB_DB_REBUILDING == PMD_DB_STATUS() )
         {
            rc = SDB_RTN_IN_REBUILD;
            goto done;
         }
         else
         {
            rc = SDB_DMS_STATE_NOT_COMPATIBLE;
         }
      }
      break;
      default :
         break;
      }
      if ( SDB_OK == rc )
      {
         ++_writeCounter;
         if ( cb )
         {
            cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_WRITE );
            cb->getLockItem( SDB_LOCK_DMS )->incCount();
         }
      }
      else if ( cb )
      {
         _stateMtx.release();
         locked = FALSE;

         while ( TRUE )
         {
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT;
               break;
            }

            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_DMS, "Waiting for dms writable" );
               hasBlock = TRUE;
            }

            rc = _blockEvent.wait( OSS_ONE_SEC );
            if ( SDB_OK == rc )
            {
               goto retry;
            }
         }
      }

   done:
      if ( locked )
      {
         _stateMtx.release();
      }
      if ( hasBlock )
      {
         cb->unsetBlock();
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_WRITABLE, rc );
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_WRITEDOWN, "_SDB_DMSCB::writeDown" )
   void _SDB_DMSCB::writeDown( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_WRITEDOWN );
      _stateMtx.get();
      --_writeCounter;
      SDB_ASSERT( 0 <= _writeCounter, "write counter should not < 0" );
      _stateMtx.release();

      if ( cb && cb->getLockItem( SDB_LOCK_DMS )->getMode() >= DMS_LOCK_WRITE )
      {
         SDB_ASSERT( cb->getLockItem( SDB_LOCK_DMS )->lockCount() > 0, "Dms lock count error" );
         UINT32 count = cb->getLockItem( SDB_LOCK_DMS )->decCount();

         if ( 0 == count && DMS_LOCK_WRITE == cb->getLockItem( SDB_LOCK_DMS )->getMode() )
         {
            cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_NONE );
         }
      }
      PD_TRACE_EXIT( SDB__SDB_DMSCB_WRITEDOWN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_BLOCKWRITE, "_SDB_DMSCB::blockWrite" )
   INT32 _SDB_DMSCB::blockWrite( _pmdEDUCB *cb, SDB_DB_STATUS byStatus, INT32 timeout )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_BLOCKWRITE );
      INT32 timeSpent = 0; // milliseconds
      BOOLEAN hasBlock = FALSE;

      if ( cb && SDB_DB_NORMAL == byStatus &&
           DMS_LOCK_WHOLE == cb->getLockItem( SDB_LOCK_DMS )->getMode() )
      {
         cb->getLockItem( SDB_LOCK_DMS )->incCount();
         goto done;
      }

      _stateMtx.get();
      if ( DMS_STATE_NORMAL != _dmsCBState )
      {
         if ( SDB_DB_OFFLINE_BK == byStatus && SDB_DB_OFFLINE_BK == PMD_DB_STATUS() )
         {
            rc = SDB_BACKUP_HAS_ALREADY_START;
         }
         else if ( SDB_DB_REBUILDING == byStatus && SDB_DB_REBUILDING == PMD_DB_STATUS() )
         {
            rc = SDB_REBUILD_HAS_ALREADY_START;
         }
         else
         {
            rc = SDB_DMS_STATE_NOT_COMPATIBLE;
         }
         _stateMtx.release();
         goto done;
      }
      _dmsCBState = DMS_STATE_READONLY;
      PMD_SET_DB_STATUS( byStatus );
      _stateMtx.release();

      while ( TRUE )
      {
         if ( cb && cb->isInterrupted() )
         {
            _dmsCBState = DMS_STATE_NORMAL;
            PMD_SET_DB_STATUS( SDB_DB_NORMAL );
            _blockEvent.signal();
            rc = SDB_APP_INTERRUPT;
            break;
         }
         else if ( timeout != -1 && timeSpent >= timeout )
         {
            _dmsCBState = DMS_STATE_NORMAL;
            PMD_SET_DB_STATUS( SDB_DB_NORMAL );
            _blockEvent.signal();
            rc = SDB_TIMEOUT;
            break;
         }
         _stateMtx.get();
         if ( 0 == _writeCounter )
         {
            _blockEvent.reset();
            _stateMtx.release();
            if ( cb )
            {
               cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_WHOLE );
               cb->getLockItem( SDB_LOCK_DMS )->incCount();
            }
            goto done;
         }
         else
         {
            if ( cb )
            {
               hasBlock = TRUE;
               cb->setBlock( EDU_BLOCK_DMS, "" );
               cb->printInfo( EDU_INFO_DOING, "Waiting to block dms write(WriteCounter:%u)",
                              _writeCounter );
            }
            _stateMtx.release();
            ossSleepmillis( DMS_CHANGESTATE_WAIT_LOOP );
            timeSpent += DMS_CHANGESTATE_WAIT_LOOP;
         }
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock();
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_BLOCKWRITE, rc );
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_UNBLOCKWRITE, "_SDB_DMSCB::unblockWrite" )
   void _SDB_DMSCB::unblockWrite( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_UNBLOCKWRITE );

      SDB_ASSERT( ( cb->getLockItem( SDB_LOCK_DMS )->lockCount() > 0 &&
                    DMS_LOCK_WHOLE == cb->getLockItem( SDB_LOCK_DMS )->getMode() ),
                  "The edu's lock mode or lock count is invalid" );

      if ( cb && cb->getLockItem( SDB_LOCK_DMS )->decCount() > 0 )
      {
         goto done;
      }

      _stateMtx.get();
      _dmsCBState = DMS_STATE_NORMAL;
      PMD_SET_DB_STATUS( SDB_DB_NORMAL );
      _stateMtx.release();
      if ( cb )
      {
         cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_NONE );
      }

   done:
      _blockEvent.signalAll();
      PD_TRACE_EXIT( SDB__SDB_DMSCB_UNBLOCKWRITE );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_REGBACKUP, "_SDB_DMSCB::registerBackup" )
   INT32 _SDB_DMSCB::registerBackup( _pmdEDUCB *cb, BOOLEAN offline )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_REGBACKUP );

      if ( offline )
      {
         rc = blockWrite( cb, SDB_DB_OFFLINE_BK );
         if ( SDB_OK == rc )
         {
            PD_LOG( PDINFO, "Block write operation succeed" );
         }
      }
      else
      {
         _stateMtx.get();
         if ( DMS_STATE_NORMAL != _dmsCBState )
         {
            if ( SDB_DB_OFFLINE_BK == PMD_DB_STATUS() || DMS_STATE_ONLINE_BACKUP == _dmsCBState )
            {
               rc = SDB_BACKUP_HAS_ALREADY_START;
            }
            else
            {
               rc = SDB_DMS_STATE_NOT_COMPATIBLE;
            }
         }
         else
         {
            PD_LOG( PDINFO, "Change dms state from [%d] to [%d]", _dmsCBState,
                    DMS_STATE_ONLINE_BACKUP );
            _dmsCBState = DMS_STATE_ONLINE_BACKUP;
         }
         _stateMtx.release();
      }

      PD_TRACE_EXITRC( SDB__SDB_DMSCB_REGBACKUP, rc );
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_BACKUPDOWN, "_SDB_DMSCB::backupDown" )
   void _SDB_DMSCB::backupDown( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_BACKUPDOWN );
      if ( DMS_LOCK_WHOLE == cb->getLockItem( SDB_LOCK_DMS )->getMode() )
      {
         unblockWrite( cb );
         PD_LOG( PDINFO, "Unblock write operation succeed" );
      }
      else
      {
         _stateMtx.get();
         PD_LOG( PDINFO, "Change dms state from [%d] to [%d]", _dmsCBState, DMS_STATE_NORMAL );
         _dmsCBState = DMS_STATE_NORMAL;
         _stateMtx.release();
      }
      PD_TRACE_EXIT( SDB__SDB_DMSCB_BACKUPDOWN );
   }

   INT32 _SDB_DMSCB::registerRebuild( _pmdEDUCB *cb )
   {
      return blockWrite( cb, SDB_DB_REBUILDING );
   }

   void _SDB_DMSCB::rebuildDown( _pmdEDUCB *cb )
   {
      unblockWrite( cb );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_REGFULLSYNC, "_SDB_DMSCB::registerFullSync" )
   INT32 _SDB_DMSCB::registerFullSync( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_REGFULLSYNC );
      BOOLEAN hasBlock = FALSE;

   retry:
      /// Full-sync can't blockWrite, because create/drop index when
      /// full-sync need to writable in async thread tasks
      _stateMtx.get();

      if ( DMS_STATE_NORMAL != _dmsCBState )
      {
         _stateMtx.release();

         rc = SDB_DMS_STATE_NOT_COMPATIBLE;
         while ( cb )
         {
            if ( cb->isInterrupted() )
            {
               rc = SDB_APP_INTERRUPT;
               break;
            }
            if ( !hasBlock )
            {
               cb->setBlock( EDU_BLOCK_DMS, "Waiting for dms fullsync" );
               hasBlock = TRUE;
            }
            rc = _blockEvent.wait( OSS_ONE_SEC );
            if ( SDB_OK == rc )
            {
               goto retry;
            }
         }
      }
      else
      {
         _dmsCBState = DMS_STATE_FULLSYNC;
         PMD_SET_DB_STATUS( SDB_DB_FULLSYNC );
         cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_WHOLE );
         cb->getLockItem( SDB_LOCK_DMS )->incCount();

         _stateMtx.release();
      }

      if ( hasBlock )
      {
         cb->unsetBlock();
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_REGFULLSYNC, rc );
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_FULLSYNCDOWN, "_SDB_DMSCB::fullSyncDown" )
   void _SDB_DMSCB::fullSyncDown( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_FULLSYNCDOWN );

      SDB_ASSERT( ( cb->getLockItem( SDB_LOCK_DMS )->lockCount() > 0 &&
                    DMS_LOCK_WHOLE == cb->getLockItem( SDB_LOCK_DMS )->getMode() ),
                  "The edu's lock mode or lock count is invalid" );

      ossScopedLock lock( &_stateMtx );
      if ( 0 == cb->getLockItem( SDB_LOCK_DMS )->decCount() )
      {
         cb->getLockItem( SDB_LOCK_DMS )->setMode( DMS_LOCK_NONE );
         _dmsCBState = DMS_STATE_NORMAL;
      }
      PMD_SET_DB_STATUS( SDB_DB_NORMAL );
      PD_TRACE_EXIT( SDB__SDB_DMSCB_FULLSYNCDOWN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_REGRESTORE, "_SDB_DMSCB::registerRestore" )
   INT32 _SDB_DMSCB::registerRestore( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACER_BEGIN( SDB__SDB_DMSCB_REGRESTORE, &rc );

      _stateMtx.get();
      if ( DMS_STATE_NORMAL != _dmsCBState )
      {
         _stateMtx.release();
         PD_LOG( PDERROR, "Unable to lock storage for restore" );
         return ( rc = SDB_DMS_STATE_NOT_COMPATIBLE );
      }
      _dmsCBState = DMS_STATE_RESTORE;
      _stateMtx.release();
      return rc;
   }

   void _SDB_DMSCB::restoreDown( _pmdEDUCB *cb )
   {
      _stateMtx.get();
      _dmsCBState = DMS_STATE_NORMAL;
      _stateMtx.release();
   }

   UINT8 _SDB_DMSCB::getCBState() const
   {
      return _dmsCBState;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLEARALLCRUDCB, "_SDB_DMSCB::clearAllCRUDCB" )
   void _SDB_DMSCB::clearAllCRUDCB()
   {
      _getMmapEngine()->clearAllCRUDCB();
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLEARSUCRUDCB, "_SDB_DMSCB::clearSUCRUDCB" )
   INT32 _SDB_DMSCB::clearSUCRUDCB( const CHAR *collectionSpace )
   {
      return _getMmapEngine()->clearSUCRUDCB( collectionSpace );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLEARMBCRUDCB, "_SDB_DMSCB::clearMBCRUDCB" )
   INT32 _SDB_DMSCB::clearMBCRUDCB( const CHAR *collection )
   {
      return _getMmapEngine()->clearMBCRUDCB( collection );
   }

   INT32 _SDB_DMSCB::regHandler( DMS_ENGINE_TYPE engineType, _IDmsEventHandler *pHandler )
   {
      if ( DMS_ENGINE_MMAP == engineType )
      {
         return _getMmapEngine()->regHandler( pHandler );
      }
      else
      {
         return SDB_OK;
      }
   }
   void _SDB_DMSCB::unregHandler( DMS_ENGINE_TYPE engineType, _IDmsEventHandler *pHandler )
   {
      _getMmapEngine()->unregHandler( pHandler );
   }

   dmsMmapEngine *_SDB_DMSCB::_getMmapEngine() const
   {
      return _mmapEngine;
   }
} // namespace engine
