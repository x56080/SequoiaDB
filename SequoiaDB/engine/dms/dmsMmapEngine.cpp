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

   Source File Name = dmsMmapEngine.cpp

   Descriptive Name = Data Management Service Mmap Engine

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data management control block, which is the metatdata information for DMS
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/29/2022  ZHY Initial Draft
   Last Changed =

*******************************************************************************/
#include "dmsMmapEngine.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCollectionHandler.hpp"
#include "ossLatch.hpp"
#include "ixm.hpp"
#include "msgDef.h"
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dpsOp2Record.hpp"
#include "rtn.hpp"
#include "ossLatch.hpp"
#include "rtnExtDataHandler.hpp"
#include "rtnRecover.hpp"
#include "boost/filesystem.hpp"

#include <list>
#include <memory>

using namespace std;
namespace engine
{
#define DMS_CSCB_STATUS_NONE ( 0 )
#define DMS_CSCB_STATUS_DELETING ( 1 )
#define DMS_CSCB_STATUS_RENAMING ( 2 )

   namespace fs = boost::filesystem;

   extern pmdEDUCB *castToEDUCB( IExecutor * );
   extern SDB_DPSCB *castToDPSCB( IDataProtectionService * );

   /*
      _dmsDefaultScannerChecker define
   */
   class _dmsDefaultScannerChecker : public IDmsScannerChecker
   {
      public:
         _dmsDefaultScannerChecker() {}
         virtual ~_dmsDefaultScannerChecker() {}
         // never need interrupt
         BOOLEAN needInterrupt()
         {
            return FALSE;
         }
   };
   typedef class _dmsDefaultScannerChecker dmsDefaultScannerChecker;

   IDmsScannerChecker *_dmsGetDefaultScannerChecker()
   {
      static dmsDefaultScannerChecker s_defaultScannerChecker;
      return &s_defaultScannerChecker;
   }

   /*
      _SDB_DMS_CSCB implement
   */
   _SDB_DMS_CSCB::~_SDB_DMS_CSCB()
   {
      if ( _su )
      {
         SDB_OSS_DEL _su;
      }
   }

   /*
      _dmsMmapEngine implement
   */

   _dmsMmapEngine::_dmsMmapEngine()
   : _mutex( MON_LATCH_SDB_DMSCB_MUTEX )
   , _logicalSUID( 0 )
   , _ixmKeySorterCreator( NULL )
   , _scannerCheckerCreator( NULL )
   {
      for ( UINT32 i = 0; i < DMS_MAX_CS_NUM; ++i )
      {
         _cscbVec.push_back( NULL );
         _tmpCscbVec.push_back( NULL );
         _tmpCscbStatusVec.push_back( DMS_CSCB_STATUS_NONE );
         // free in desctructor
         _latchVec.push_back( new ( std::nothrow ) ossRWMutex() );
         _freeList.push( i );
      }
   }

   _dmsMmapEngine::~_dmsMmapEngine()
   {
      SDB_ASSERT( _handlers.empty(), "all handlers should be unregistered" );
      // make sure dms control block is finalized
      SDB_ASSERT( _cscbNameMap.empty(), "all collection spaces should be released" );
   }

   INT32 _dmsMmapEngine::open( dmsSuConstraintMap &cm )
   {
      INT32 rc = SDB_OK;

      if ( pmdGetKRCB()->isRestore() )
      {
         goto done;
      }

      // 1. load all
      if ( SDB_ROLE_COORD != pmdGetDBRole() )
      {
         rc = _loadCollectionSpaces(
            pmdGetOptionCB()->getDbPath(), pmdGetOptionCB()->getIndexPath(),
            pmdGetOptionCB()->getLobPath(), pmdGetOptionCB()->getLobMetaPath(), cm );
         PD_RC_CHECK( rc, PDERROR, "Failed to load collectionspaces, rc: %d", rc );
      }

      rc = _pageMapDispatcher.active();
      if ( rc )
      {
         PD_LOG( PDERROR, "Active page map dispatcher failed, rc: %d", rc );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::close( IExecutor *executor, const dmsCloseDBOptions &options )
   {
      _CSCBNameMapCleanup();

      for ( UINT32 i = 0; i < DMS_MAX_CS_NUM; ++i )
      {
         if ( _latchVec[ i ] )
         {
            SDB_OSS_DEL _latchVec[ i ];
            _latchVec[ i ] = NULL;
         }
      }

      return SDB_OK;
   }

   INT32 _dmsMmapEngine::createCS( IExecutor *executor,
                                   const CHAR *pCollectionSpace,
                                   const utilCSUniqueID &csUniqueID,
                                   const dmsCreateCSOptions &o,
                                   const bson::BSONObj &adjunct,
                                   DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( pCollectionSpace, "collection space can't be NULL" );
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsStorageUnit *su = NULL;
      pmdOptionsCB *optCB = pmdGetOptionCB();
      INT32 pageSize = o.dataPageSize;
      INT32 lobPageSize = o.lobdPageSize;
      pmdEDUCB *cb = castToEDUCB( executor );
      SDB_DPSCB *dpsCB = castToDPSCB( o.dpsCB );

      // let's see if the CS already exist or not
      rc = nameToSUAndLock( pCollectionSpace, suID, &su );
      if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         // make sure assign su to NULL so that we won't delete it at exit
         su = NULL;
         // collectionspace already exist
         PD_LOG( PDERROR, "Collection space %s is already exist", pCollectionSpace );
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }

      // new storage unit, will insert into dmsCB->addCollectionSpace
      su = SDB_OSS_NEW dmsStorageUnit( pCollectionSpace, csUniqueID, 1, pmdGetBuffPool(), pageSize,
                                       lobPageSize, o.stype, rtnGetExtDataHandler() );
      if ( !su )
      {
         PD_LOG( PDERROR, "Failed to allocate new storage unit" );
         rc = SDB_OOM;
         goto error;
      }

      rc = su->open( pmdGetOptionCB()->getDbPath(), pmdGetOptionCB()->getIndexPath(),
                     pmdGetOptionCB()->getLobPath(), pmdGetOptionCB()->getLobMetaPath(),
                     pmdGetSyncMgr(), TRUE );

      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create collection space %s at %s, rc: %d", pCollectionSpace,
                 pmdGetOptionCB()->getDbPath(), rc );
         goto error;
      }
      /// set config
      su->setSyncConfig( optCB->getSyncInterval(), optCB->getSyncRecordNum(),
                         optCB->getSyncDirtyRatio() );
      su->setSyncDeep( optCB->isSyncDeep() );
      // set MVCC support
      su->setMVCCSupport( optCB->mvccOn() );

      /// add collctionspace
      rc = addCollectionSpace( pCollectionSpace, 1, su, cb, dpsCB, TRUE, desc );
      if ( rc )
      {
         if ( SDB_DMS_CS_EXIST == rc )
         {
            PD_LOG( PDWARNING,
                    "Failed to add collectionspace because it's "
                    "already exist: %s",
                    pCollectionSpace );
         }
         else
         {
            PD_LOG( PDERROR, "Failed to add collection space, rc = %d", rc );
         }
         /// need to remove the files
         su->remove();
         goto error;
      }

      PD_LOG( PDEVENT,
              "Create collectionspace[name: %s, id: %u] succeed, "
              "PageSize:%u, LobPageSize:%u",
              pCollectionSpace, csUniqueID, pageSize, lobPageSize );

   done:
      // Unlock the existing storage unit
      if ( DMS_INVALID_CS != suID )
      {
         suUnlock( suID );
      }
      return rc;
   error:
      if ( su )
      {
         SDB_OSS_DEL( su );
         su = NULL;
      }
      goto done;
   }

   INT32 _dmsMmapEngine::testCS( IExecutor *executor, const CHAR *name, utilCSUniqueID &uniqueId )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsStorageUnit *su = NULL;
      uniqueId = UTIL_UNIQUEID_NULL;
      rc = nameToSUAndLock( name, suID, &su );
      PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit [%s], rc: %d", name, rc );
      uniqueId = su->CSUniqueID();

   done:
      // Unlock the existing storage unit
      if ( DMS_INVALID_CS != suID )
      {
         suUnlock( suID );
      }
      return rc;
   error:
      uniqueId = UTIL_UNIQUEID_NULL;
      if ( su )
      {
         SDB_OSS_DEL( su );
         su = NULL;
      }
      goto done;
   }

   INT32 _dmsMmapEngine::testCS( IExecutor *executor, utilCSUniqueID uniqueId )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsStorageUnit *su = NULL;
      uniqueId = UTIL_UNIQUEID_NULL;
      rc = idToSUAndLock( uniqueId, suID, &su );
      PD_RC_CHECK( rc, PDERROR, "failed to find collection[%d]", uniqueId );

   done:
      // Unlock the existing storage unit
      if ( DMS_INVALID_CS != suID )
      {
         suUnlock( suID );
      }
      return rc;
   error:
      uniqueId = UTIL_UNIQUEID_NULL;
      if ( su )
      {
         SDB_OSS_DEL( su );
         su = NULL;
      }
      goto done;
   }

   INT32 _dmsMmapEngine::listCS( IExecutor *executor, DATA_CURSOR_PTR &cursor )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 _dmsMmapEngine::listCL( IExecutor *executor, const CHAR *csName, DATA_CURSOR_PTR &cursor )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 _dmsMmapEngine::getCSCount( IExecutor *executor, UINT32 &count )
   {
      INT32 rc = SDB_OK;
      ossScopedLock( &_mutex, SHARED );
      count = _cscbNameMap.size();
      return rc;
   }

   INT32 _dmsMmapEngine::removeCS( IExecutor *executor,
                                   const CHAR *name,
                                   const dmsRemoveCSOptions &options )
   {

      INT32 rc = delCollectionSpace( executor, name, options.dpsCB, options.sysCall, TRUE,
                                     options.ensureEmpty, options.recycleOptions );
      return rc;
   }

   INT32 _dmsMmapEngine::removeEmptyCS( IExecutor *executor,
                                        const CHAR *name,
                                        const dmsRemoveCSOptions &options )
   {
      INT32 rc = delCollectionSpace( executor, name, options.dpsCB, options.sysCall, TRUE,
                                     options.ensureEmpty );
      if ( SDB_LOCK_FAILED == rc )
      {
         rc = SDB_DMS_CS_NOT_EMPTY;
      }
      return rc;
   }

   INT32 _dmsMmapEngine::renameCS( IExecutor *executor,
                                   const CHAR *csName,
                                   const CHAR *newCSName,
                                   BOOLEAN isBlockWrite,
                                   DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      pmdEDUCB *cb = castToEDUCB( executor );

      /// dms rename
      rc = renameCollectionSpace( csName, newCSName, cb, sdbGetDPSCB() );
      PD_RC_CHECK( rc, PDERROR, "Failed to rename collectionspace from %s to %s, rc: %d", csName,
                   newCSName, rc );
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::unloadCS( IExecutor *executor,
                                   const CHAR *name,
                                   const dmsRemoveCSOptions &options )
   {
      INT32 rc = delCollectionSpace( executor, name, options.dpsCB, options.sysCall, FALSE,
                                     options.ensureEmpty );
      return rc;
   }

   INT32 _dmsMmapEngine::restoreCS( IExecutor *executor, const CHAR *name )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 _dmsMmapEngine::returnCS( IExecutor *executor,
                                   dmsReturnOptions &options,
                                   BOOLEAN blockWrite )
   {
      SDB_ASSERT( FALSE, "todo" );
      return SDB_OK;
   }

   INT32 _dmsMmapEngine::openCL( IExecutor *executor,
                                 const CHAR *clFullName,
                                 const dmsOpenCLOptions &o,
                                 DATA_COLLECTION_PTR &ptr )
   {
      INT32 rc = SDB_OK;
      ptr.reset();
      dmsStorageUnit *su = nullptr;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsMBContext *mbContext = nullptr;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
      UINT32 dotPos = strchr( clFullName, '.' ) - clFullName;
      ossMemcpy( csName, clFullName, dotPos );
      const CHAR *clShortName = clFullName + dotPos + 1;
      rc = nameToSUAndLock( csName, suID, &su, SHARED );
      PD_RC_CHECK( rc, PDWARNING, "Failed to loop up su by collection space name[%s], rc: %d",
                   csName, rc );
      rc = su->data()->getMBContext( &mbContext, clShortName, o.mbLockType );
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s.%s] mb context failed, rc: %d", csName,
                   clShortName, rc );
      ptr = std::make_shared< dmsCollectionHandler >( su, suID, mbContext, this );
      if ( !ptr )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "out of memory" );
         goto error;
      }
   done:
      return rc;
   error:
      if ( DMS_INVALID_CS != suID )
      {
         suUnlock(suID);
      }
      ptr.reset();
      goto done;
   }

   INT32 _dmsMmapEngine::openCL( IExecutor *executor,
                                 utilCLUniqueID uniqueId,
                                 const dmsOpenCLOptions &o,
                                 DATA_COLLECTION_PTR &ptr )
   {
      INT32 rc = SDB_OK;
      ptr.reset();
      dmsStorageUnit *su = nullptr;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsMBContext *mbContext = nullptr;
      utilCSUniqueID csuid = utilGetCSUniqueID( uniqueId );
      rc = idToSUAndLock( csuid, suID, &su, SHARED );
      PD_RC_CHECK( rc, PDERROR, "Failed to loop up su by cs unique id[%u], rc: %d", csuid, rc );
      rc = su->data()->getMBContextByID( &mbContext, uniqueId, o.mbLockType );
      PD_RC_CHECK( rc, PDERROR, "Get collection[%llu] mb context failed, rc: %d", uniqueId, rc );
      ptr = std::make_shared< dmsCollectionHandler >( su, suID, mbContext, this );
      if ( !ptr )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "out of memory" );
         goto error;
      }
   done:
      return rc;
   error:
      if ( DMS_INVALID_CS != suID )
      {
         suUnlock(suID);
      }
      ptr.reset();
      goto done;
   }

   INT32 _dmsMmapEngine::getCLCount( IExecutor *executor, const CHAR *csName, UINT32 &count )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnit *su = nullptr;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      rc = nameToSUAndLock( csName, suID, &su, SHARED );
      PD_RC_CHECK( rc, PDWARNING, "Failed to loop up su by collection space name[%s], rc: %d",
                   csName, rc );
      count = su->data()->getCollectionNum();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::createCS( IExecutor *executor,
                                   const CHAR *name,
                                   const utilCSUniqueID &uniqueId,
                                   const dmsCreateCSOptions &o,
                                   const bson::BSONObj &adjunct )
   {
      return SDB_NOT_SUPPORTED;
   }

   INT32 _dmsMmapEngine::removeCS( IExecutor *executor, const CHAR *csName )
   {
      return SDB_NOT_SUPPORTED;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__LGCSCBNMMAP, "_dmsMmapEngine::_logCSCBNameMap" )
   void _dmsMmapEngine::_logCSCBNameMap()
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__LGCSCBNMMAP );

      CSCB_MAP_CONST_ITER it;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         PD_LOG( PDDEBUG, "%s\n", it->first );
      }
      PD_TRACE_EXIT( SDB__SDB_DMSCB__LGCSCBNMMAP );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMINST, "_dmsMmapEngine::_CSCBNameInsert" )
   INT32 _dmsMmapEngine::_CSCBNameInsert( const CHAR *pName,
                                          UINT32 topSequence,
                                          _dmsStorageUnit *su,
                                          dmsStorageUnitID &suID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__CSCBNMINST );
      SDB_DMS_CSCB *cscb = NULL;
      utilCSUniqueID csUniqueID = su->CSUniqueID();

      if ( 0 == _freeList.size() )
      {
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }
      cscb = SDB_OSS_NEW SDB_DMS_CSCB( pName, topSequence, su );
      if ( !cscb )
      {
         PD_LOG( PDERROR, "Failed to allocate memory to insert cscb" );
         rc = SDB_OOM;
         goto error;
      }

      // We get from front and return to back so that suID is not reused
      // immediately.
      suID = _freeList.front();
      su->_setCSID( suID );
      su->_setLogicalCSID( _logicalSUID++ );
      _freeList.pop();
      _cscbNameMap[ cscb->_name ] = suID;
      _cscbVec[ suID ] = cscb;
      if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         _cscbIDMap[ csUniqueID ] = suID;
      }
   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__CSCBNMINST, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::_CSCBNameLookup( const CHAR *pName,
                                          SDB_DMS_CSCB **cscb,
                                          dmsStorageUnitID *pSuID,
                                          BOOLEAN exceptDeleting )
   {
      return _CSCBLookup( pName, UTIL_UNIQUEID_NULL, cscb, pSuID, exceptDeleting );
   }

   INT32 _dmsMmapEngine::_CSCBIdLookup( utilCSUniqueID csUniqueID,
                                        SDB_DMS_CSCB **cscb,
                                        dmsStorageUnitID *pSuID,
                                        BOOLEAN exceptDeleting )
   {
      return _CSCBLookup( NULL, csUniqueID, cscb, pSuID, exceptDeleting );
   }

   // look up by name or unique id
   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBLOOK, "_dmsMmapEngine::_CSCBLookup" )
   INT32 _dmsMmapEngine::_CSCBLookup( const CHAR *pName,
                                      utilCSUniqueID csUniqueID,
                                      SDB_DMS_CSCB **cscb,
                                      dmsStorageUnitID *pSuID,
                                      BOOLEAN exceptDeleting )
   {
      SDB_ASSERT( cscb, "cscb can't be null!" );

      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__CSCBLOOK );

      dmsStorageUnitID suID = DMS_INVALID_SUID;

      if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         CSCB_ID_MAP_CONST_ITER it = _cscbIDMap.find( csUniqueID );
         if ( it != _cscbIDMap.end() )
         {
            suID = it->second;
         }
      }
      else if ( pName )
      {
         CSCB_MAP_CONST_ITER it = _cscbNameMap.find( pName );
         if ( it != _cscbNameMap.end() )
         {
            suID = it->second;
         }
      }

      if ( DMS_INVALID_SUID == suID )
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      if ( pSuID )
      {
         *pSuID = suID;
      }

      if ( _cscbVec[ suID ] )
      {
         *cscb = _cscbVec[ suID ];
         goto done;
      }
      else if ( _tmpCscbVec[ suID ] )
      {
         if ( !exceptDeleting )
         {
            *cscb = _tmpCscbVec[ suID ];
            goto done;
         }

         BYTE disabledStatus = _tmpCscbStatusVec[ suID ];
         switch ( disabledStatus )
         {
         case DMS_CSCB_STATUS_DELETING :
            rc = SDB_DMS_CS_DELETING;
            break;
         case DMS_CSCB_STATUS_RENAMING :
            rc = SDB_DMS_CS_RENAMING;
            break;
         default :
            rc = SDB_SYS;
            SDB_ASSERT( FALSE, "This is impossible in this case" );
            break;
         }
         goto error;
      }
      else
      {
         /// This is impossible in this case
         SDB_ASSERT( FALSE, "This is impossible in this case" );
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__CSCBLOOK, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::_CSCBNameLookupAndLock( const CHAR *pName,
                                                 dmsStorageUnitID &suID,
                                                 SDB_DMS_CSCB **cscb,
                                                 OSS_LATCH_MODE lockType,
                                                 INT32 millisec )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( cscb, "cscb can't be null!" );

      rc = _CSCBNameLookup( pName, cscb, &suID, TRUE );
      if ( rc )
      {
         goto error;
      }

      if ( EXCLUSIVE == lockType )
      {
         rc = _latchVec[ suID ]->lock_w( millisec );
      }
      else
      {
         rc = _latchVec[ suID ]->lock_r( millisec );
      }
      if ( rc )
      {
         goto error;
      }

   done:
      return rc;
   error:
      suID = DMS_INVALID_CS;
      *cscb = NULL;
      goto done;
   }

   INT32 _dmsMmapEngine::_CSCBIdLookupAndLock( utilCSUniqueID csUniqueID,
                                               dmsStorageUnitID &suID,
                                               SDB_DMS_CSCB **cscb,
                                               OSS_LATCH_MODE lockType,
                                               INT32 millisec )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( cscb, "cscb can't be null!" );

      rc = _CSCBIdLookup( csUniqueID, cscb, &suID, TRUE );
      if ( rc )
      {
         goto error;
      }

      if ( EXCLUSIVE == lockType )
      {
         rc = _latchVec[ suID ]->lock_w( millisec );
      }
      else
      {
         rc = _latchVec[ suID ]->lock_r( millisec );
      }
      if ( rc )
      {
         goto error;
      }

   done:
      return rc;
   error:
      suID = DMS_INVALID_CS;
      *cscb = NULL;
      goto done;
   }

   void _dmsMmapEngine::_CSCBRelease( dmsStorageUnitID suID, OSS_LATCH_MODE lockType )
   {
      if ( EXCLUSIVE == lockType )
      {
         _latchVec[ suID ]->release_w();
      }
      else
      {
         _latchVec[ suID ]->release_r();
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__MVCSCB2TMP, "_dmsMmapEngine::_moveCSCB2TmpList" )
   INT32 _dmsMmapEngine::_moveCSCB2TmpList( const CHAR *pName, BYTE status )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__MVCSCB2TMP );

      dmsStorageUnitID suID = DMS_INVALID_SUID;
      SDB_DMS_CSCB *pCSCB = NULL;
      BOOLEAN metaLock = FALSE;

      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, TRUE );
      _mutex.release_shared();

      if ( rc )
      {
         goto error;
      }

   retry:
      // now let's lock the collectionspace, if we can't lock it, let's return
      // false. we shouldn't wait forever
      if ( SDB_OK != _latchVec[ suID ]->lock_w( OSS_ONE_SEC ) )
      {
         rc = SDB_LOCK_FAILED;
         goto error;
      }
      if ( !_mutex.try_get() )
      {
         _latchVec[ suID ]->release_w();
         ossSleep( 50 );
         goto retry;
      }
      metaLock = TRUE;
      _latchVec[ suID ]->release_w();

      // there is a small timing hole before getting the latch, so we have
      // to get current suID again to verify
      {
         dmsStorageUnitID suTmpID = DMS_INVALID_SUID;
         SDB_DMS_CSCB *tmpCSCB = NULL;
         rc = _CSCBNameLookup( pName, &tmpCSCB, &suTmpID, TRUE );
         if ( rc )
         {
            goto error;
         }
         else if ( suTmpID != suID )
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
      }

      SDB_ASSERT( pCSCB->_su, "su can't be null" );
      SDB_ASSERT( pCSCB->_name, "cs name can't be null" );

      _tmpCscbVec[ suID ] = pCSCB;
      _cscbVec[ suID ] = NULL;
      _tmpCscbStatusVec[ suID ] = status;

      _mutex.release();
      metaLock = FALSE;

   done:
      if ( metaLock )
      {
         _mutex.release();
         metaLock = FALSE;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__MVCSCB2TMP, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__RESTORECSCBFRTMP, "_dmsMmapEngine::_restoreCSCBFromTmpList" )
   INT32 _dmsMmapEngine::_restoreCSCBFromTmpList( const CHAR *pName )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__RESTORECSCBFRTMP );

      dmsStorageUnitID suID = DMS_INVALID_SUID;
      SDB_DMS_CSCB *pCSCB = NULL;

      ossScopedLock _lock( &_mutex, EXCLUSIVE );
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, FALSE );
      if ( rc )
      {
         SDB_ASSERT( FALSE, "Impossible in the case" );
         goto error;
      }

      if ( _tmpCscbVec[ suID ] != pCSCB )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" );
         rc = SDB_SYS;
         goto error;
      }
      else
      {
         _tmpCscbVec[ suID ] = NULL;
         _cscbVec[ suID ] = pCSCB;
         _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE;
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__RESTORECSCBFRTMP, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBRENAME, "_dmsMmapEngine::_CSCBRename" )
   INT32 _dmsMmapEngine::_CSCBRename( const CHAR *pName,
                                      const CHAR *pNewName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__CSCBRENAME );

      dmsStorageUnitID suID = DMS_INVALID_SUID;
      UINT32 csLID = ~0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isReserved = FALSE;
      BOOLEAN isLocked = FALSE;
      UINT32 logRecSize = 0;
      dpsMergeInfo info;
      dpsLogRecord &record = info.getMergeBlock().record();
      SDB_DMS_CSCB *pCSCB = NULL;
      IDmsExtDataHandler *extHandler = NULL;

      // reserved log-size
      if ( NULL != dpsCB )
      {
         rc = dpsCSRename2Record( pName, pNewName, record );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build record:%d", rc );
            goto error;
         }
         rc = dpsCB->checkSyncControl( record.alignedLen(), cb );
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc );

         logRecSize = record.alignedLen();
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to reserved log space(length=%u)", logRecSize );
         isReserved = TRUE;
      }

      _mutex.get();
      isLocked = TRUE;

      /// check old name and new name
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, TRUE );
      if ( rc )
      {
         goto error;
      }

      rc = _CSCBNameLookup( pNewName, &pCSCB, NULL, TRUE );
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK;
      }
      else if ( SDB_OK == rc )
      {
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }
      else
      {
         goto error;
      }

      SDB_ASSERT( pCSCB->_su, "su can't be null" );

      /// rename cs file
      rc = pCSCB->_su->renameCS( pNewName );
      if ( rc )
      {
         PD_LOG( PDERROR,
                 "Rename collection space[%s] to [%s] failed, "
                 "rc: %d",
                 pName, pNewName, rc );
         goto error;
      }

      /// rename in map
      // 1) erase map must before reset the name, because map'key is CBCB's name
      _cscbNameMap.erase( pName );
      // 2) rename the CSCB's name
      ossStrncpy( pCSCB->_name, pNewName, DMS_COLLECTION_SPACE_NAME_SZ );
      pCSCB->_name[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0;
      // 3) insert new name to map
      _cscbNameMap[ pCSCB->_name ] = suID;

      /// write log
      csLID = pCSCB->_su->LogicalCSID();
      if ( dpsCB )
      {
         info.setInfoEx( csLID, ~0, DMS_INVALID_EXTENT, cb );
         rc = dpsCB->prepare( info );
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to insert cscrt into log, rc = %d", rc );
            goto error;
         }
         _mutex.release();
         isLocked = FALSE;

         dpsCB->writeData( info );
      }

      // Release the mutex first, since event handler needs the mutex
      if ( isLocked )
      {
         _mutex.release();
         isLocked = FALSE;
      }

      extHandler = pCSCB->_su->data()->getExtDataHandler();
      if ( extHandler )
      {
         extHandler->onRenameCS( pName, pNewName, cb, NULL );
      }

      pCSCB->_su->getEventHolder()->onRenameCS( DMS_EVENT_MASK_ALL, pName, pNewName, cb, dpsCB );

   done:
      if ( isLocked )
      {
         _mutex.release();
         isLocked = FALSE;
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
         isReserved = FALSE;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__CSCBRENAME, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::_CSCBRenameP1( const CHAR *pName,
                                        const CHAR *pNewName,
                                        _pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB )
   {
      return _moveCSCB2TmpList( pName, DMS_CSCB_STATUS_RENAMING );
   }

   INT32 _dmsMmapEngine::_CSCBRenameP1Cancel( const CHAR *pName,
                                              const CHAR *pNewName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB )
   {
      return _restoreCSCBFromTmpList( pName );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBRENAMEP2, "_dmsMmapEngine::_CSCBRenameP2" )
   INT32 _dmsMmapEngine::_CSCBRenameP2( const CHAR *pName,
                                        const CHAR *pNewName,
                                        _pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__CSCBRENAMEP2 );

      dmsStorageUnitID suID = DMS_INVALID_SUID;
      dmsStorageUnitID newSuID = DMS_INVALID_SUID;
      UINT32 csLID = ~0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isReserved = FALSE;
      BOOLEAN isLocked = FALSE;
      UINT32 logRecSize = 0;
      dpsMergeInfo info;
      dpsLogRecord &record = info.getMergeBlock().record();
      SDB_DMS_CSCB *pCSCB = NULL;
      SDB_DMS_CSCB *pNewCSCB = NULL;

      /// reserved log-size
      if ( NULL != dpsCB )
      {
         rc = dpsCSRename2Record( pName, pNewName, record );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build record:%d", rc );
            goto error;
         }
         rc = dpsCB->checkSyncControl( record.alignedLen(), cb );
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc );

         logRecSize = record.alignedLen();
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to reserved log space(length=%u)", logRecSize );
         isReserved = TRUE;
      }

      _mutex.get();
      isLocked = TRUE;

      /// check old name and new name
      rc = _CSCBNameLookup( pName, &pCSCB, &suID, FALSE );
      if ( rc )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" );
         goto error;
      }
      rc = _CSCBNameLookup( pNewName, &pNewCSCB, &newSuID, FALSE );
      if ( SDB_DMS_CS_NOTEXIST == rc )
      {
         rc = SDB_OK;
      }
      else
      {
         rc = ( SDB_OK == rc ) ? SDB_DMS_CS_EXIST : rc;
         SDB_ASSERT( FALSE, "Impossible in this case" );
         goto error;
      }

      if ( pCSCB != _tmpCscbVec[ suID ] )
      {
         SDB_ASSERT( FALSE, "Impossible in this case" );
         rc = SDB_SYS;
         goto error;
      }

      /// rename in map
      // 1) erase map must before reset the name, because map'key is CBCB's name
      _cscbNameMap.erase( pName );
      // 2) rename the CSCB's name
      ossStrncpy( pCSCB->_name, pNewName, DMS_COLLECTION_SPACE_NAME_SZ );
      pCSCB->_name[ DMS_COLLECTION_SPACE_NAME_SZ ] = 0;
      // 3) insert new name to map
      _cscbNameMap[ pCSCB->_name ] = suID;
      // 4) enable the cscb
      _tmpCscbVec[ suID ] = NULL;
      _cscbVec[ suID ] = pCSCB;
      _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE;

      /// write log
      SDB_ASSERT( pCSCB->_su, "su can't be null" );
      csLID = pCSCB->_su->LogicalCSID();

      if ( dpsCB )
      {
         info.setInfoEx( csLID, ~0, DMS_INVALID_EXTENT, cb );
         rc = dpsCB->prepare( info );
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to insert cscrt into log, rc = %d", rc );
            goto error;
         }
         _mutex.release();
         isLocked = FALSE;

         dpsCB->writeData( info );
      }

   done:
      if ( isLocked )
      {
         _mutex.release();
         isLocked = FALSE;
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
         isReserved = FALSE;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__CSCBRENAMEP2, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::_CSCBNameRemoveP1( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      return _moveCSCB2TmpList( pName, DMS_CSCB_STATUS_DELETING );
   }

   INT32 _dmsMmapEngine::_CSCBNameRemoveP1Cancel( const CHAR *pName,
                                                  _pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      return _restoreCSCBFromTmpList( pName );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMREMVP2, "_dmsMmapEngine::_CSCBNameRemoveP2" )
   INT32 _dmsMmapEngine::_CSCBNameRemoveP2( const CHAR *pName,
                                            dmsDropCSOptions *options,
                                            _pmdEDUCB *cb,
                                            SDB_DPSCB *dpsCB,
                                            SDB_DMS_CSCB *&pCSCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__CSCBNMREMVP2 );
      dmsStorageUnitID suID = DMS_INVALID_SUID;
      UINT32 csLID = ~0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isReserved = FALSE;
      BOOLEAN isLocked = FALSE;
      UINT32 logRecSize = 0;
      dpsMergeInfo info;
      dpsLogRecord &record = info.getMergeBlock().record();
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL;

      if ( NULL != dpsCB )
      {
         BSONObj *boOptions = NULL;

         if ( NULL != options )
         {
            rc = options->prepareOptions();
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to prepare drop collection space "
                         "options, rc: %d",
                         rc );
            boOptions = &( options->_boOptions );
         }

         // reserved log-size
         rc = dpsCSDel2Record( pName, boOptions, record );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build record:%d", rc );
            goto error;
         }

         rc = dpsCB->checkSyncControl( record.alignedLen(), cb );
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc );

         logRecSize = record.alignedLen();
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to reserved log space(length=%u)", logRecSize );
         isReserved = TRUE;
      }

      _mutex.get();
      isLocked = TRUE;

      if ( ( NULL == options ) || ( !( options->isTakenOver() ) ) )
      {
         rc = _CSCBNameLookup( pName, &pCSCB, &suID, FALSE );
         if ( rc )
         {
            SDB_ASSERT( FALSE, "Impossible in this case" );
            goto error;
         }
         if ( pCSCB != _tmpCscbVec[ suID ] )
         {
            SDB_ASSERT( FALSE, "Impossible in this case" );
            rc = SDB_SYS;
            goto error;
         }

         SDB_ASSERT( pCSCB->_su, "su can't be null" );

         // get unique id from su. Because if the cl is in _cscbIDMap, and
         // we don't erase it, it may cause core dump.
         csUniqueID = pCSCB->_su->CSUniqueID();

         _tmpCscbVec[ suID ] = NULL;
         _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE;
         _cscbNameMap.erase( pName );
         if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
         {
            _cscbIDMap.erase( csUniqueID );
         }
         _freeList.push( suID );
      }

      // log here
      csLID = pCSCB->_su->LogicalCSID();
      if ( dpsCB )
      {
         info.setInfoEx( csLID, ~0, DMS_INVALID_EXTENT, cb );
         rc = dpsCB->prepare( info );
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to insert cscrt into log, rc = %d", rc );
            goto error;
         }
         _mutex.release();
         isLocked = FALSE;

         dpsCB->writeData( info );
      }

   done:
      if ( isLocked )
      {
         _mutex.release();
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__CSCBNMREMVP2, rc );
      return rc;
   error:
      pCSCB = NULL;
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__CSCBNMMAPCLN, "_dmsMmapEngine::_CSCBNameMapCleanup" )
   void _dmsMmapEngine::_CSCBNameMapCleanup()
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__CSCBNMMAPCLN );

      CSCB_MAP_CONST_ITER it;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnitID suID = ( *it ).second;

         _freeList.push( suID );
         if ( _cscbVec[ suID ] )
         {
            _cscbVec[ suID ]->_su->unsetEventHandlers();
            SDB_OSS_DEL _cscbVec[ suID ];
            _cscbVec[ suID ] = NULL;
         }
         if ( _tmpCscbVec[ suID ] )
         {
            _tmpCscbVec[ suID ]->_su->unsetEventHandlers();
            SDB_OSS_DEL _tmpCscbVec[ suID ];
            _tmpCscbVec[ suID ] = NULL;
         }
         _tmpCscbStatusVec[ suID ] = DMS_CSCB_STATUS_NONE;
      }
      _cscbNameMap.clear();
      _cscbIDMap.clear();
      PD_TRACE_EXIT( SDB__SDB_DMSCB__CSCBNMMAPCLN );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__GETCSLIST, "_dmsMmapEngine::_getCSList" )
   INT32 _dmsMmapEngine::_getCSList( ossPoolVector< ossPoolString > &csNameVec )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__GETCSLIST );

      INT32 rc = SDB_OK;

      ossScopedLock lock( &_mutex, SHARED );
      for ( CSCB_MAP_CONST_ITER itr = _cscbNameMap.begin(); itr != _cscbNameMap.end(); ++itr )
      {
         try
         {
            csNameVec.push_back( ossPoolString( itr->first ) );
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Get collectionspaces list occur exception: %s", e.what() );
            rc = SDB_OOM;
            goto error;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__GETCSLIST, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::idToSUAndLock( utilCSUniqueID csUniqueID,
                                        dmsStorageUnitID &suID,
                                        _dmsStorageUnit **su,
                                        OSS_LATCH_MODE lockType,
                                        INT32 millisec )
   {
      INT32 rc = SDB_OK;
      SDB_DMS_CSCB *cscb = NULL;

      SDB_ASSERT( su, "su can't be null!" );
      if ( !UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         return SDB_INVALIDARG;
      }

      ossScopedLock _lock( &_mutex, SHARED );
      rc = _CSCBIdLookupAndLock( csUniqueID, suID, &cscb, lockType, millisec );
      if ( SDB_OK == rc )
      {
         *su = cscb->_su;
      }

      return rc;
   }

   INT32 _dmsMmapEngine::nameToSUAndLock( const CHAR *pName,
                                          dmsStorageUnitID &suID,
                                          _dmsStorageUnit **su,
                                          OSS_LATCH_MODE lockType,
                                          INT32 millisec )
   {
      INT32 rc = SDB_OK;
      SDB_DMS_CSCB *cscb = NULL;
      SDB_ASSERT( su, "su can't be null!" );
      if ( !pName )
      {
         return SDB_INVALIDARG;
      }

      ossScopedLock _lock( &_mutex, SHARED );
      rc = _CSCBNameLookupAndLock( pName, suID, &cscb, lockType, millisec );
      if ( SDB_OK == rc )
      {
         *su = cscb->_su;
      }

      return rc;
   }

   INT32 _dmsMmapEngine::verifySUAndLock( const dmsEventSUItem *pSUItem,
                                          _dmsStorageUnit **ppSU,
                                          OSS_LATCH_MODE lockType,
                                          INT32 millisec )
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT( pSUItem, "pSUItem is invalid" );
      SDB_ASSERT( ppSU, "ppSU is invalid" );

      const CHAR *pCSName = pSUItem->_pCSName;
      dmsStorageUnitID origSUID = pSUItem->_suID;
      UINT32 origSULID = pSUItem->_suLID;

      dmsStorageUnit *pSU = NULL;
      dmsStorageUnitID suID = DMS_INVALID_SUID;
      UINT32 suLID = DMS_INVALID_LOGICCSID;

      rc = nameToSUAndLock( pCSName, suID, &pSU, lockType, millisec );
      PD_RC_CHECK( rc, PDWARNING, "Failed to get collection space [%s] in %d, rc: %d", pCSName,
                   lockType, rc );

      suLID = pSU->LogicalCSID();

      PD_CHECK( suID == origSUID && suLID == origSULID, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Collection space [%s] had been updated, "
                "original [ ID: %d, LID: %u ], new [ ID: %d, LID: %u ]",
                pCSName, origSUID, origSULID, suID, suLID );

   done:
      ( *ppSU ) = pSU;
      return rc;

   error:
      if ( DMS_INVALID_SUID != suID )
      {
         suUnlock( suID, lockType );
      }
      pSU = NULL;
      goto done;
   }

   INT32 _dmsMmapEngine::nameToSuDescriptor( const CHAR *pName, DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      desc.reset();

      if ( NULL == pName )
      {
         rc = SDB_INVALIDARG;
      }
      else
      {
         ossScopedLock _lock( &_mutex, SHARED );
         SDB_DMS_CSCB *cscb = NULL;
         rc = _CSCBNameLookup( pName, &cscb, NULL, TRUE );
         if ( SDB_OK == rc )
         {
            desc = makeSharedPtrFromPool< dmsSuDescriptor >(
               getEngineType(), cscb->_name, cscb->_su->CSUniqueID(), cscb->_su->LogicalCSID() );
            if ( !desc )
            {
               rc = SDB_OOM;
               PD_LOG( PDERROR, "failed to allocate memory for descriptor of collection space[%s]",
                       cscb->_name );
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      desc.reset();
      goto done;
   }

   INT32 _dmsMmapEngine::createCL( IExecutor *executor,
                                   const CHAR *clFullName,
                                   utilCLUniqueID clUniqueID,
                                   const dmsCreateCLOptions &o,
                                   const bson::BSONObj &adjunct )
   {
      INT32 rc = SDB_OK;
      INT32 rcTmp = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsStorageUnit *su = NULL;
      UINT16 collectionID = DMS_INVALID_MBID;
      UINT32 logicalID = DMS_INVALID_CLID;
      BSONObj shardIdxDef = o.shardIdxDef ? *o.shardIdxDef : BSONObj();
      CHAR attrStr[ 64 + 1 ] = { 0 };
      pmdEDUCB *cb = castToEDUCB( executor );
      SDB_DPSCB *dpsCB = castToDPSCB( o.dpsCB );
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
      UINT32 dotPos = strchr( clFullName, '.' ) - clFullName;
      ossMemcpy( csName, clFullName, dotPos );
      const CHAR *clShortName = clFullName + dotPos + 1;
      rc = nameToSUAndLock( csName, suID, &su, SHARED );
      PD_RC_CHECK( rc, PDWARNING, "Failed to loop up su by collection space name[%s], rc: %d",
                   csName, rc );

      if ( DMS_STORAGE_CAPPED != su->type() && OSS_BIT_TEST( o.attributes, DMS_MB_ATTR_CAPPED ) )
      {
         PD_LOG( PDERROR,
                 "Capped collection[%s] can only be created on "
                 "capped collection space[%s]",
                 clShortName, su->CSName() );
         rc = SDB_OPERATION_INCOMPATIBLE;
         goto error;
      }

      if ( UTIL_CLUNIQUEID_LOCAL == clUniqueID )
      {
         if ( UTIL_CSUNIQUEID_LOCAL != su->CSUniqueID() )
         {
            clUniqueID = utilBuildCLUniqueID( su->CSUniqueID(), UTIL_CLINNERID_LOCAL );
         }
      }

      rc = su->data()->addCollection( clShortName, &collectionID, clUniqueID, o.attributes, cb,
                                      dpsCB, 0, o.sysCall, o.compressor, &logicalID, o.extOptions,
                                      o.idIdxDef, o.addIdxIDIfNotExist );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to create collection [name:%s, id:%llu], rc: %d", clFullName,
                 clUniqueID, rc );
         goto error;
      }

      if ( !shardIdxDef.isEmpty() )
      {
         rc = su->createIndex( clShortName, shardIdxDef, cb, dpsCB, TRUE, NULL,
                               SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE, NULL, NULL, FALSE,
                               o.addIdxIDIfNotExist );
         if ( SDB_IXM_REDEF == rc || SDB_IXM_EXIST_COVERD_ONE == rc )
         {
            /// same defined index already exists.
            PD_LOG( PDINFO, "Failed to create shared index %s: %s, rc: %d", clFullName,
                    shardIdxDef.toString().c_str(), rc );
            rc = SDB_OK;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDINFO, "Failed to create shared index %s: %s, rc: %d", clFullName,
                    shardIdxDef.toString().c_str(), rc );
            goto error_rollback;
         }
         PD_LOG( PDEVENT, "Create index[%s] for collection[%s] succeed",
                 shardIdxDef.toString().c_str(), clFullName );
      }

      if ( OSS_BIT_TEST( o.attributes, DMS_MB_ATTR_COMPRESSED ) &&
           UTIL_COMPRESSOR_LZW == o.compressor )
      {
         /*
          * If the compression type is snappy, set it directly. If it's lzw, push
          * it to the dictionary creating list.
          */
         pushDictJob( dmsDictJob( suID, su->LogicalCSID(), collectionID, logicalID ) );
      }

      mbAttr2String( o.attributes, attrStr, sizeof( attrStr ) - 1 );
      PD_LOG( PDEVENT,
              "Create collection[name: %s, id: %llu] succeed, "
              "ShardingKey:%s, Attr:%s(0x%08x), CompressType:%s(%d)%s%s%s%s",
              clFullName, clUniqueID,
              shardIdxDef.getObjectField( IXM_FIELD_NAME_KEY ).toString().c_str(), attrStr,
              o.attributes, utilCompressType2String( (UINT8)o.compressor ), o.compressor,
              o.extOptions && !o.extOptions->isEmpty() ? ", External options:" : "",
              o.extOptions && !o.extOptions->isEmpty() ? o.extOptions->toString().c_str() : "",
              o.idIdxDef && !o.idIdxDef->isEmpty() ? ", Id Index:" : "",
              o.idIdxDef && !o.idIdxDef->isEmpty() ? o.idIdxDef->toString().c_str() : "" );

   done:
      if ( DMS_INVALID_CS != suID )
      {
         suUnlock( suID );
      }
      return rc;
   error_rollback:

      rcTmp = removeCL( executor, clFullName, dmsRemoveCLOptions() );
      if ( SDB_OK != rcTmp && SDB_DMS_NOTEXIST != rcTmp )
      {
         PD_LOG( PDERROR, "Failed to rollback creating collection %s, rc = %d", clFullName, rcTmp );
      }
      goto done;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::removeCL( IExecutor *executor,
                                   const CHAR *clFullName,
                                   const dmsRemoveCLOptions &o )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsStorageUnit *su = NULL;
      dmsMBContext *mbContext = NULL;
      pmdEDUCB *cb = castToEDUCB( executor );
      SDB_DPSCB *dpsCB = castToDPSCB( o.dpsCB );

      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
      UINT32 dotPos = strchr( clFullName, '.' ) - clFullName;
      ossMemcpy( csName, clFullName, dotPos );
      const CHAR *clShortName = clFullName + dotPos + 1;
      rc = nameToSUAndLock( csName, suID, &su, SHARED );
      PD_RC_CHECK( rc, PDWARNING, "Failed to loop up su by collection space name[%s], rc: %d",
                   csName, rc );

      if ( UTIL_UNIQUEID_NULL != o.clUniqueID )
      {
         rc = su->data()->getMBContext( &mbContext, clShortName );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get mbContext for collection "
                      "%s, rc: %d",
                      clFullName, rc );

         PD_CHECK( mbContext->mb()->_clUniqueID == o.clUniqueID, SDB_DMS_NOTEXIST, error, PDWARNING,
                   "Collection %s with unique ID %llu had been dropped, "
                   "current unique ID is %llu",
                   clFullName, o.clUniqueID, mbContext->mb()->_clUniqueID );
      }

      rc = su->data()->dropCollection( clShortName, cb, dpsCB, TRUE, mbContext, o.recycleOptions );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to drop collection %s, rc: %d", clFullName, rc );
         goto error;
      }

   done:
      if ( NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext );
      }
      if ( DMS_INVALID_CS != suID )
      {
         suUnlock( suID );
      }

      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::testCL( IExecutor *executor,
                                 const CHAR *clFullName,
                                 utilCLUniqueID &uniqueId )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnit *su = nullptr;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsMBContext *mbContext = nullptr;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {};
      UINT32 dotPos = strchr( clFullName, '.' ) - clFullName;
      ossMemcpy( csName, clFullName, dotPos );
      const CHAR *clShortName = clFullName + dotPos + 1;
      rc = nameToSUAndLock( csName, suID, &su, SHARED );
      PD_RC_CHECK( rc, PDWARNING, "Failed to loop up su by collection space name[%s], rc: %d",
                   csName, rc );
      rc = su->data()->getMBContext( &mbContext, clShortName, SHARED );
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s.%s] mb context failed, rc: %d", csName,
                   clShortName, rc );
      uniqueId = mbContext->mb()->_clUniqueID;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::testCL( IExecutor *executor, utilCLUniqueID uniqueId )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnit *su = nullptr;
      dmsStorageUnitID suID = DMS_INVALID_CS;
      dmsMBContext *mbContext = nullptr;
      utilCSUniqueID csuid = utilGetCSUniqueID( uniqueId );
      rc = idToSUAndLock( csuid, suID, &su, SHARED );
      PD_RC_CHECK( rc, PDERROR, "Failed to loop up su by cs unique id[%u], rc: %d", csuid, rc );
      rc = su->data()->getMBContextByID( &mbContext, uniqueId, SHARED );
      PD_RC_CHECK( rc, PDERROR, "Get collection[%llu] mb context failed, rc: %d", uniqueId, rc );

   done:
      return rc;
   error:
      goto done;
   }

   _dmsStorageUnit *_dmsMmapEngine::suLock( dmsStorageUnitID suID )
   {
      ossScopedLock _lock( &_mutex, SHARED );
      if ( NULL == _cscbVec[ suID ] )
      {
         return NULL;
      }
      _latchVec[ suID ]->lock_r();
      return _cscbVec[ suID ]->_su;
   }

   void _dmsMmapEngine::suUnlock( dmsStorageUnitID suID, OSS_LATCH_MODE lockType )
   {
      _CSCBRelease( suID, lockType );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGCSUID, "_dmsMmapEngine::changeCSUniqueID" )
   INT32 _dmsMmapEngine::changeCSUniqueID( _dmsStorageUnit *su, utilCSUniqueID csUniqueID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CHGCSUID );

      SDB_DMS_CSCB *cscb = NULL;
      dmsStorageUnitID suID = DMS_INVALID_SUID;
      dmsStorageInfo *suInfo = NULL;
      const CHAR *csname = su->CSName();
      utilCSUniqueID orgUniqueID = su->CSUniqueID();

      if ( orgUniqueID == csUniqueID )
      {
         goto done;
      }

      // change unique id in storage unit
      suInfo = su->_getStorageInfo();
      suInfo->_csUniqueID = csUniqueID;

      su->_pDataSu->updateCSUniqueIDFromInfo();
      su->_pIndexSu->updateCSUniqueIDFromInfo();
      su->_pLobSu->updateCSUniqueIDFromInfo();

      PD_LOG( PDEVENT, "Change cs[%s] unique id from [%u] to [%u]", csname, orgUniqueID,
              csUniqueID );

      // get su id
      rc = _CSCBNameLookup( csname, &cscb, &suID );
      if ( rc || DMS_INVALID_SUID == suID )
      {
         PD_LOG( PDERROR, "Failed to look up cs[%s], suID: %d, rc: %d", csname, suID, rc );
         goto error;
      }

      // change map
      if ( UTIL_IS_VALID_CSUNIQUEID( orgUniqueID ) )
      {
         _cscbIDMap.erase( orgUniqueID );
      }
      if ( UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         _cscbIDMap[ csUniqueID ] = suID;
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CHGCSUID, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGIDXUID, "_dmsMmapEngine::_changeIndexUniqueID" )
   INT32 _dmsMmapEngine::_changeIndexUniqueID( _dmsStorageUnit *su,
                                               const ossPoolVector< ossPoolString > &changedClVec,
                                               const ossPoolVector< BSONObj > &idxInfoVec,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CHGIDXUID );

      // convert index info which is from catalog
      MAP_CLNAME_IDX clIdxMap;
      utilBson2IdxNameId( idxInfoVec, clIdxMap );

      // loop every collection
      for ( ossPoolVector< ossPoolString >::const_iterator itr = changedClVec.begin();
            itr != changedClVec.end(); itr++ )
      {
         MAP_IDXNAME_DEF idxDefMap;
         const CHAR *clShortName = itr->c_str();
         CHAR clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 };
         ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s", su->CSName(), clShortName );

         MAP_CLNAME_IDX::iterator it = clIdxMap.find( clFullName );
         if ( it != clIdxMap.end() )
         {
            idxDefMap = it->second;
         }

         // get indexes from local
         MON_IDX_LIST localIdxList;
         rc = su->getIndexes( clShortName, localIdxList );
         PD_RC_CHECK( rc, PDWARNING, "Failed to get collection[%s]'s indexes, rc: %d", clFullName,
                      rc );

         // loop every index
         for ( MON_IDX_LIST::iterator it = localIdxList.begin(); it != localIdxList.end(); it++ )
         {
            const CHAR *idxName = it->getIndexName();
            // Use local index definition ( filter out UniqueID ) to assign
            // initial value to "idxDefToCreate". If the index doesn't exist on
            // catalog, we also need to call createIndex(). The createIndex()
            // function will check whether the index UniqueID is valid or not,
            // if it's invalid, a new UniqueID will be generated for the index.
            BSONObj idxDefToCreate =
               it->_indexDef.filterFieldsUndotted( BSON( IXM_FIELD_NAME_UNIQUEID << 1 ), false );

            // if the index has unique id at catalog, use it
            MAP_IDXNAME_DEF::iterator i = idxDefMap.find( idxName );
            if ( i != idxDefMap.end() )
            {
               if ( ixmIsSameDef( i->second, idxDefToCreate, TRUE ) )
               {
                  idxDefToCreate = i->second;
               }
            }

            // change unique id by createIndex()
            rc = su->createIndex( clShortName, idxDefToCreate, cb, NULL, TRUE, NULL,
                                  SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE, NULL, NULL, FALSE, FALSE );
            if ( rc )
            {
               PD_LOG( PDWARNING, "Failed to upgrade index's unique id, rc: %d", rc );
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CHGIDXUID, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::_loadCollectionSpaces( const CHAR *dataPath,
                                                const CHAR *indexPath,
                                                const CHAR *lobPath,
                                                const CHAR *lobMetaPath,
                                                dmsSuConstraintMap &cm )
   {
      INT32 rc = SDB_OK;
      CHAR csName[ DMS_SU_FILENAME_SZ + 1 ] = { 0 };
      UINT32 sequence = 0;
      dmsStorageUnit *storageUnit = NULL;
      pmdOptionsCB *optCB = pmdGetOptionCB();

      SDB_ASSERT( dataPath, "data path can't be NULL" );
      SDB_ASSERT( indexPath, "index path can't be NULL" );
      SDB_ASSERT( lobPath, "lob path can't be NULL" );
      SDB_ASSERT( lobMetaPath, "lob meta path can't be NULL" );

      utilRenameLogManager logManager;
      rc = logManager.load();
      PD_RC_CHECK( rc, PDERROR, "Failed to load rename logs, rc: %d", rc );

      try
      {
         fs::path dbDir( dataPath );
         fs::directory_iterator end_iter;
         if ( !fs::exists( dbDir ) || !fs::is_directory( dbDir ) )
         {
            PD_RC_CHECK( SDB_INVALIDARG, PDERROR,
                         "Given path %s is not a "
                         "directory or not exist",
                         dataPath );
         }

         // load all file
         for ( fs::directory_iterator dir_iter( dbDir ); dir_iter != end_iter; ++dir_iter )
         {
            if ( !fs::is_regular_file( dir_iter->status() ) )
            {
               continue;
            }
            // 1) file name must be <collectionspace>.<sequence>.<data>
            const std::string fileName = dir_iter->path().filename().string();
            if ( rtnVerifyCollectionSpaceFileName( fileName.c_str(), csName, DMS_SU_FILENAME_SZ,
                                                   sequence ) )
            {
               // check if rename is interrupted
               if ( logManager.hasRenamed() )
               {
                  utilRenameLog renameLog;
                  rc = logManager.getRenameLog( csName, renameLog );
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to get rename log "
                               "for collection space [%s], rc: %d",
                               csName, rc );

                  if ( renameLog.isValid() )
                  {
                     PD_LOG( PDEVENT, "Got rename log [%s] -> [%s]", renameLog.oldName,
                             renameLog.newName );

                     // try correct file names
                     rc = rtnCorrectCollectionSpaceFile( dataPath, indexPath, lobPath, lobMetaPath,
                                                         sequence, renameLog );
                     PD_RC_CHECK( rc, PDERROR,
                                  "Failed to correct collection "
                                  "space file [%s] -> [%s], rc: %d",
                                  renameLog.oldName, renameLog.newName, rc );

                     if ( 0 == ossStrcmp( csName, renameLog.oldName ) )
                     {
                        ossStrcpy( csName, renameLog.newName );
                     }

                     rc = logManager.clear( renameLog );
                     if ( SDB_OK != rc )
                     {
                        // clear failed, we can ignore
                        PD_LOG( PDWARNING,
                                "Failed to clear rename log "
                                "[%s] -> [%s], rc: %d",
                                renameLog.oldName, renameLog.newName, rc );
                        rc = SDB_OK;
                     }
                  }
               }

               /// skip SYSTEMP file
               if ( 0 == ossStrcmp( csName, SDB_DMSTEMP_NAME ) )
               {
                  continue;
               }

               storageUnit = SDB_OSS_NEW dmsStorageUnit(
                  csName, UTIL_UNIQUEID_NULL, sequence, pmdGetBuffPool(), DMS_PAGE_SIZE_DFT,
                  DMS_DEFAULT_LOB_PAGE_SZ, DMS_STORAGE_NORMAL, rtnGetExtDataHandler() );
               PD_CHECK( storageUnit, SDB_OOM, error, PDERROR,
                         "Failed to allocate dmsStorageUnit for %s",
                         dir_iter->path().string().c_str() );

               // open collection space file failed, need report error
               // and restart
               rc = storageUnit->open( dataPath, indexPath, lobPath, lobMetaPath, pmdGetSyncMgr(),
                                       FALSE );
               if ( rc )
               {
                  SDB_OSS_DEL storageUnit;
                  storageUnit = NULL;
                  PD_LOG( PDSEVERE, "Failed to open storage unit[%s], rc: %d",
                          dir_iter->path().string().c_str(), rc );
                  PMD_RESTART_DB( rc );
                  goto error;
               }
               /// set config
               storageUnit->setSyncConfig( optCB->getSyncInterval(), optCB->getSyncRecordNum(),
                                           optCB->getSyncDirtyRatio() );
               storageUnit->setSyncDeep( optCB->isSyncDeep() );
               // set MVCC support
               storageUnit->setMVCCSupport( optCB->mvccOn() );

               /// add collectionspace
               DMS_SU_DESCRIPTOR desc = nullptr;
               rc = addCollectionSpace( csName, sequence, storageUnit, NULL, NULL, FALSE, desc );
               if ( rc )
               {
                  SDB_OSS_DEL storageUnit;
                  storageUnit = NULL;
                  PD_LOG( PDSEVERE,
                          "Failed to add collection "
                          "space[%s], rc: %d",
                          csName, rc );
                  PMD_RESTART_DB( rc );
                  goto error;
               }

               rc = cm.addSuDescriptor( desc );
               PD_RC_CHECK( rc, PDERROR, "failed to add su descriptor, cs name: %s, rc: %d", csName,
                            rc );

               // Note: do not call onLoad here

               storageUnit = NULL;

               /*
                * Scan all the collections, to check if any one should be
                * put into the dictionary creating list. This should be
                * done if the system restarted before the dictionary was
                * created.
                */
               rc = _resumeClDictCreate( csName );
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to resume dictionary creating "
                            "job for %s, rc: %d",
                            csName, rc );
            } // if ( rtnVerifyCollectionSpaceFileName
            else if ( SDB_FILE_UNKNOW == rtnParseFileName( fileName.c_str() ) )
            {
               PD_LOG( PDWARNING,
                       "Found unknow file[%s] when load collection "
                       "spaces, ignored",
                       dir_iter->path().string().c_str() );
            }
         } // for ( fs::directory_iterator dir_iter(dbDir)
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Failed to iterate directory %s: %s", dataPath, e.what() );
      }

      if ( logManager.hasRenamed() )
      {
         logManager.clearAll();
      }

   done:
      return rc;
   error:
      if ( storageUnit )
      {
         SDB_OSS_DEL storageUnit;
      }
      goto done;
   }

   INT32 _dmsMmapEngine::_resumeClDictCreate( const CHAR *csName )
   {
      INT32 rc = SDB_OK;

      dmsStorageUnit *su = NULL;
      dmsStorageUnitID suID = DMS_INVALID_SUID;
      dmsMBContext *context = NULL;
      dmsMB *mb = NULL;

      rc = nameToSUAndLock( csName, suID, &su );
      if ( rc )
      {
         PD_LOG( PDERROR,
                 "Failed to get and lock collectionspace[%s], "
                 "rc: %d",
                 csName, rc );
         goto error;
      }

      for ( UINT16 mbID = 0; mbID < DMS_MME_SLOTS; ++mbID )
      {
         // If the collection does not exist, lock will failed.
         rc =
            su->data()->getMBContext( &context, mbID, DMS_INVALID_CLID, DMS_INVALID_CLID, SHARED );
         if ( rc )
         {
            if ( SDB_DMS_NOTEXIST == rc )
            {
               rc = SDB_OK;
               continue;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to get dms mb context, rc: %d", rc );
               goto error;
            }
         }

         mb = context->mb();

         /*
          * Three conditions should be matched to resume dictionary creating job
          * for a collection:
          * (1) 'Compressed' option is set as true
          * (2) 'CompressionType' is set as 'lzw'
          * (3) The dictionary extent id is invalid currently, which means the
          *     dictionary has not been created yet.
          *
          * The in use flag in mb is checked when taking the lock, so no need to
          * check here.
          */
         if ( OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) &&
              ( UTIL_COMPRESSOR_LZW == mb->_compressorType ) &&
              ( DMS_INVALID_EXTENT == mb->_dictExtentID ) )
         {
            pushDictJob(
               dmsDictJob( su->CSID(), su->LogicalCSID(), context->mbID(), context->clLID() ) );
         }

         su->data()->releaseMBContext( context );
      }

   done:
      if ( context )
      {
         su->data()->releaseMBContext( context );
      }
      if ( DMS_INVALID_SUID != suID )
      {
         _CSCBRelease( suID, SHARED );
         suID = DMS_INVALID_SUID;
      }
      return rc;
   error:
      goto done;
   }

   // input: clInfoObj
   // [
   //    { "Name": "bar1", "UniqueID": 2667174690817 } ,
   //    { "Name": "bar2", "UniqueID": 2667174690818 }
   // ]
   // input: pIdxInfoVec
   // [
   //   { Collection: "foo.bar", IndexDef: {xxx} },
   //   { Collection: "foo.ba1", IndexDef: {xxx} }
   // ]
   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGUID, "_dmsMmapEngine::changeUniqueID" )
   INT32 _dmsMmapEngine::changeUniqueID( const CHAR *csname,
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
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CHGUID );

      BOOLEAN isReserved = FALSE;
      dpsMergeInfo info;
      dpsLogRecord &record = info.getMergeBlock().record();
      UINT32 logRecSize = 0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      SDB_DMS_CSCB *cscb = NULL;
      SDB_DMS_CSCB *tmpCSCB = NULL;
      dmsStorageUnitID suID = DMS_INVALID_SUID;
      dmsStorageUnitID suTmpID = DMS_INVALID_SUID;
      dmsStorageUnit *su = NULL;
      dmsStorageUnit *suTmp = NULL;
      BOOLEAN isMetaLocked = FALSE;
      ossPoolVector< ossPoolString > changedCLVec;

      _mutex.get_shared();
      rc = _CSCBNameLookup( csname, &cscb, &suID, TRUE );
      _mutex.release_shared();

      if ( rc )
      {
         goto error;
      }
      su = cscb->_su;

      // reserved log-size
      if ( dpsCB )
      {
         rc = dpsAddUniqueID2Record( csname, csUniqueID, clInfoObj, record );
         PD_RC_CHECK( rc, PDERROR, "Failed to build record:%d", rc );

         rc = dpsCB->checkSyncControl( record.alignedLen(), cb );
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc );

         logRecSize = record.alignedLen();
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to reserved log space(length=%u)", logRecSize );
         isReserved = TRUE;
      }

      // change cl unique id
      rc = nameToSUAndLock( csname, suTmpID, &suTmp, SHARED );
      if ( rc )
      {
         goto error;
      }
      else if ( suTmpID != suID )
      {
         suUnlock( suTmpID );
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      rc = su->data()->changeCLUniqueID( utilBson2ClNameId( clInfoObj ), changeOtherCL, csUniqueID,
                                         isLoadCS, changedCLVec );
      if ( rc )
      {
         // ignore error
         PD_LOG( PDWARNING, "Failed to change collection unique id, rc: %d", rc );
      }

      suUnlock( suTmpID );

      // change cs unique id
      _mutex.get();
      isMetaLocked = TRUE;

      // there is a small timing hole before getting the mutex, so we have
      // to get current suID again to verify
      rc = _CSCBNameLookup( csname, &tmpCSCB, &suTmpID, TRUE );
      if ( rc )
      {
         goto error;
      }
      else if ( suTmpID != suID )
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      rc = changeCSUniqueID( su, csUniqueID );
      PD_RC_CHECK( rc, PDERROR, "Failed to change cs unique id, rc: %d", rc );

      // write dps
      if ( dpsCB )
      {
         info.setInfoEx( cscb->_su->LogicalCSID(), ~0, DMS_INVALID_EXTENT, cb );
         rc = dpsCB->prepare( info );
         PD_RC_CHECK( rc, PDERROR, "Failed to insert cscrt into log, rc: %d", rc );

         _mutex.release();
         isMetaLocked = FALSE;

         dpsCB->writeData( info );
      }

      if ( isMetaLocked )
      {
         _mutex.release();
         isMetaLocked = FALSE;
      }

      if ( changeIdx )
      {
         rc = nameToSUAndLock( su->CSName(), suTmpID, &suTmp, SHARED );
         if ( rc )
         {
            goto error;
         }
         else if ( suTmpID != suID )
         {
            suUnlock( suTmpID );
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }

         ossPoolVector< BSONObj > emptyVec;
         INT32 rc1 =
            _changeIndexUniqueID( su, changedCLVec, pIdxInfoVec ? *pIdxInfoVec : emptyVec, cb );
         if ( rc1 )
         {
            // ignore error
            PD_LOG( PDWARNING, "Failed to change index unique id, rc: %d", rc1 );
         }

         suUnlock( suTmpID );
      }

   done:
      if ( rc == SDB_OK )
      {
         PD_LOG( PDEVENT, "Change unique id, cs name: %s, cs unique id: %u, cl info: %s", csname,
                 csUniqueID, clInfoObj.toString().c_str() );
      }
      if ( isMetaLocked )
      {
         _mutex.release();
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CHGUID, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_ADDCS, "_dmsMmapEngine::addCollectionSpace" )
   INT32 _dmsMmapEngine::addCollectionSpace( const CHAR *pName,
                                             UINT32 topSequence,
                                             _dmsStorageUnit *su,
                                             _pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB,
                                             BOOLEAN isCreate,
                                             DMS_SU_DESCRIPTOR &desc )
   {
      INT32 rc = SDB_OK;
      dmsStorageUnitID suID;
      SDB_DMS_CSCB *cscb = NULL;
      BOOLEAN isReserved = FALSE;
      BOOLEAN isLocked = FALSE;
      UINT32 logRecSize = 0;
      dpsMergeInfo info;
      dpsLogRecord &record = info.getMergeBlock().record();
      INT32 pageSize = 0;
      INT32 lobPageSz = 0;
      INT32 type = 0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      utilCSUniqueID csUniqueID = 0;
      desc.reset();

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_ADDCS );

      if ( !pName || !su )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      csUniqueID = su->CSUniqueID();
      pageSize = su->getPageSize();
      lobPageSz = su->getLobPageSize();
      type = su->type();

      if ( NULL != dpsCB )
      {
         // reserved log-size
         rc = dpsCSCrt2Record( pName, csUniqueID, pageSize, lobPageSz, type, record );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build record:%d", rc );
            goto error;
         }
         rc = dpsCB->checkSyncControl( record.alignedLen(), cb );
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc );

         logRecSize = record.alignedLen();
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR, "failed to reserved log space(length=%u)", logRecSize );
         isReserved = TRUE;
      }

      _mutex.get();
      isLocked = TRUE;

      rc = _CSCBNameLookup( pName, &cscb );
      if ( SDB_OK == rc )
      {
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }
      else if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         goto error;
      }

      rc = _CSCBIdLookup( csUniqueID, &cscb );
      if ( SDB_OK == rc )
      {
         rc = SDB_DMS_CS_UNIQUEID_CONFLICT;
         PD_LOG( PDERROR, "CS unique id[%u] already exists[name: %s], rc: %d", csUniqueID,
                 cscb->_name, rc );
         goto error;
      }
      else if ( rc != SDB_DMS_CS_NOTEXIST )
      {
         goto error;
      }

      rc = _CSCBNameInsert( pName, topSequence, su, suID );
      if ( SDB_OK == rc )
      {
         UINT32 suLID = su->LogicalCSID();
         desc =
            makeSharedPtrFromPool< dmsSuDescriptor >( getEngineType(), pName, csUniqueID, suLID );
         // write dps
         if ( dpsCB )
         {
            info.setInfoEx( suLID, ~0, DMS_INVALID_EXTENT, cb );
            rc = dpsCB->prepare( info );
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to insert cscrt into log, rc = %d", rc );
               goto error;
            }
            _mutex.release();
            isLocked = FALSE;
            dpsCB->writeData( info );
         }
      }

      su->setEventHandlers( &_handlers );

      if ( isLocked )
      {
         _mutex.release();
         isLocked = FALSE;
      }
      if ( isCreate )
      {
         su->getEventHolder()->onCreateCS( DMS_EVENT_MASK_ALL, cb, dpsCB );
      }
      else
      {
         su->getEventHolder()->onLoadCS( DMS_EVENT_MASK_ALL, cb, dpsCB );
      }

   done:
      if ( isLocked )
      {
         _mutex.release();
      }
      if ( isReserved )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_ADDCS, rc );
      return rc;
   error:
      desc.reset();
      su->unsetEventHandlers();
      goto done;
   }

   INT32 _dmsMmapEngine::delCollectionSpace( IExecutor *executor,
                                             const CHAR *pCollectionSpace,
                                             IDataProtectionService *dps,
                                             BOOLEAN sysCall,
                                             BOOLEAN dropFile,
                                             BOOLEAN ensureEmpty,
                                             dmsDropCSOptions *options )
   {
      INT32 rc = SDB_OK;

      UINT32 retryTime = 0;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB();
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB();
      UINT32 suLogicalID = DMS_INVALID_LOGICCSID;
      DMS_SU_DESCRIPTOR desc = nullptr;
      pmdEDUCB *cb = castToEDUCB( executor );
      SDB_DPSCB *dpsCB = castToDPSCB( dps );

      SDB_ASSERT( pCollectionSpace, "collection space can't be NULL" );

      rc = nameToSuDescriptor( pCollectionSpace, desc );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get logical ID for "
                   "collection space [%s], rc: %d",
                   pCollectionSpace, rc );
      if ( desc )
      {
         suLogicalID = desc->logicalID;
      }
      SDB_ASSERT( DMS_INVALID_LOGICCSID != suLogicalID, "logical ID should be valid" );

      // let's find out whether the collection space is held by this
      // EDU. If so we have to get rid of those contexts
      if ( NULL != cb )
      {
         rtnDelContextForCollectionSpace( pCollectionSpace, suLogicalID, cb );
      }

      while ( TRUE )
      {
         if ( ( PMD_IS_DB_DOWN() ) || ( NULL != cb && cb->isInterrupted() ) )
         {
            PD_LOG( PDWARNING,
                    "Failed to drop collection space [%s], "
                    "it is interrupted",
                    pCollectionSpace );
            rc = SDB_APP_INTERRUPT;
            goto error;
         }

         // - tell others to close contexts on the same collection space
         // - tell other waiting transactions to give up
         if ( ( rtnCB->preDelContext( pCollectionSpace, suLogicalID ) > 0 ) ||
              ( NULL != cb && cb->getTransExecutor()->useTransLock() &&
                transCB->transLockKillWaiters( suLogicalID, DMS_INVALID_MBID, NULL,
                                               SDB_DPS_TRANS_LOCK_INCOMPATIBLE ) ) )
         {
            ossSleep( 200 );
         }

         if ( dropFile )
         {
            if ( ensureEmpty )
            {
               rc = dropEmptyCollectionSpace( pCollectionSpace, cb, dpsCB );
            }
            else
            {
               rc = dropCollectionSpace( pCollectionSpace, cb, dpsCB, options );
            }
         }
         else
         {
            rc = unloadCollectonSpace( pCollectionSpace, cb );
         }

         if ( SDB_LOCK_FAILED == rc && retryTime < 100 )
         {
            ++retryTime;
            rc = SDB_OK;
            continue;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to %s collectionspace %s, "
                      "rc: %d",
                      dropFile ? "drop" : "unload", pCollectionSpace, rc );
         break;
      }

   done:
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DELCS, "_dmsMmapEngine::_delCollectionSpace" )
   INT32 _dmsMmapEngine::_delCollectionSpace( const CHAR *pName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB,
                                              BOOLEAN removeFile,
                                              BOOLEAN onlyEmpty,
                                              dmsDropCSOptions *options )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DELCS );

      UINT32 csLID = ~0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isTransLocked = FALSE;
      SDB_DMS_CSCB *pCSCB = NULL;

      // get cs cb
      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, TRUE );
      _mutex.release_shared();
      if ( rc )
      {
         goto error;
      }

      SDB_ASSERT( pCSCB->_su, "su can't be null" );

      // check cs is empty or not
      if ( onlyEmpty && 0 != pCSCB->_su->data()->getCollectionNum() )
      {
         rc = SDB_DMS_CS_NOT_EMPTY;
         goto error;
      }

      // lock transaction, standalone need lock trans here
      csLID = pCSCB->_su->LogicalCSID();
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict;
         rc = pTransCB->transLockTryZ( cb, csLID, DMS_INVALID_MBID, NULL, &lockConflict );
         if ( rc )
         {
            PD_LOG(
               PDERROR,
               "Failed to lock collection-space, rc:%d" OSS_NEWLINE
               "Conflict( representative ):" OSS_NEWLINE "   EDUID:  %llu" OSS_NEWLINE
               "   TID:    %u" OSS_NEWLINE "   LockId: %s" OSS_NEWLINE "   Mode:   %s" OSS_NEWLINE,
               rc, lockConflict._eduID, lockConflict._tid, lockConflict._lockID.toString().c_str(),
               lockModeToString( lockConflict._lockType ) );
            goto error;
         }
         isTransLocked = TRUE;
      }

      // drop phase 1
      rc = _delCollectionSpaceP1( pName, cb, dpsCB, removeFile );
      if ( rc )
      {
         goto error;
      }

      // re-check cs is empty or not in lock
      if ( onlyEmpty && 0 != pCSCB->_su->data()->getCollectionNum() )
      {
         // it is not empty after phase 1, cancel deleting
         _delCollectionSpaceP1Cancel( pName, cb, dpsCB );
         rc = SDB_DMS_CS_NOT_EMPTY;
         goto error;
      }

      // drop phase 2
      rc = _delCollectionSpaceP2( pName, cb, dpsCB, removeFile, options );
      if ( rc )
      {
         _delCollectionSpaceP1Cancel( pName, cb, dpsCB );
         goto error;
      }

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, csLID );
         isTransLocked = FALSE;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DELCS, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::dropCollectionSpace( const CHAR *pName,
                                              _pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB,
                                              dmsDropCSOptions *options )
   {
      INT32 rc = _delCollectionSpace( pName, cb, dpsCB, TRUE, FALSE, options );
      return rc;
   }

   INT32 _dmsMmapEngine::unloadCollectonSpace( const CHAR *pName, _pmdEDUCB *cb )
   {
      return _delCollectionSpace( pName, cb, NULL, FALSE, FALSE );
   }

   INT32 _dmsMmapEngine::dropEmptyCollectionSpace( const CHAR *pName,
                                                   _pmdEDUCB *cb,
                                                   SDB_DPSCB *dpsCB )
   {
      INT32 rc = _delCollectionSpace( pName, cb, dpsCB, TRUE, TRUE );
      if ( SDB_LOCK_FAILED == rc )
      {
         rc = SDB_DMS_CS_NOT_EMPTY;
      }
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__DELCSP1, "_dmsMmapEngine::_delCollectionSpaceP1" )
   INT32 _dmsMmapEngine::_delCollectionSpaceP1( const CHAR *pName,
                                                _pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB,
                                                BOOLEAN removeFile )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB__DELCSP1 );
      IDmsExtDataHandler *extHandler = NULL;
      SDB_DMS_CSCB *pCSCB = NULL;

      if ( !pName )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _CSCBNameRemoveP1( pName, cb, dpsCB );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to drop cs[%s], rc: %d", pName, rc );
         goto error;
      }

      // The cscb of the cs is now in the deleting vector. Try to delete all the
      // capped collections of the text indices in it.
      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE );
      _mutex.release_shared();
      PD_RC_CHECK( rc, PDERROR, "Find collection space[ %s ] failed, rc: %d", pName, rc );

      extHandler = pCSCB->_su->data()->getExtDataHandler();
      if ( extHandler )
      {
         rc = extHandler->onDelCS( pCSCB->_name, cb, removeFile );
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            // if capped cs doesn't exist, we just ignor error
            rc = SDB_OK;
         }
         if ( rc )
         {
            // If external operation failed, we should resume by cancel the
            // the remove.
            PD_LOG( PDERROR,
                    "External operation on drop CS[ %s ] failed,"
                    " rc: %d",
                    pName, rc );
            INT32 rcTmp = _CSCBNameRemoveP1Cancel( pName, cb, dpsCB );
            if ( rcTmp )
            {
               PD_LOG( PDERROR, "Cancel remove cs name failed, rc: %d", rcTmp );
            }
            goto error;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__DELCSP1, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__DELCSP1CANCEL, "_dmsMmapEngine::_delCollectionSpaceP1Cancel" )
   INT32 _dmsMmapEngine::_delCollectionSpaceP1Cancel( const CHAR *pName,
                                                      _pmdEDUCB *cb,
                                                      SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      IDmsExtDataHandler *extHandler = NULL;
      SDB_DMS_CSCB *pCSCB = NULL;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB__DELCSP1CANCEL );
      if ( !pName )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      // Until now the cs is still not visible to other operations, as the cscb
      // is in the deleting vector. So abort the external operation before
      // the P1Cancel below. As once the cancel is done, the cs is exposed to
      // other operations, and parallel scenarios should be considered.
      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE );
      _mutex.release_shared();
      if ( rc )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Find collection space[ %s ] failed, rc: %d", pName, rc );
         goto error;
      }
      SDB_ASSERT( pCSCB, "Collection space CB is NULL" );
      if ( pCSCB )
      {
         extHandler = pCSCB->_su->data()->getExtDataHandler();
         if ( extHandler )
         {
            rc = extHandler->abortOperation( DMS_EXTOPR_TYPE_DROPCS, cb );
            PD_RC_CHECK( rc, PDERROR,
                         "External abort operation on drop "
                         "CS[ %s ] failed, rc: %d",
                         pName, rc );
         }
      }

      rc = _CSCBNameRemoveP1Cancel( pName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR, "failed to cancel remove cs(rc=%d)", rc );

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__DELCSP1CANCEL, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB__DELCSP2, "_dmsMmapEngine::_delCollectionSpaceP2" )
   INT32 _dmsMmapEngine::_delCollectionSpaceP2( const CHAR *pName,
                                                _pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB,
                                                BOOLEAN removeFile,
                                                dmsDropCSOptions *options )
   {
      INT32 rc = SDB_OK;
      SDB_DMS_CSCB *pCSCB = NULL;
      IDmsExtDataHandler *extHandler = NULL;

      dmsEventSUItem suItem;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB__DELCSP2 );
      if ( !pName )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE );
      _mutex.release_shared();
      PD_RC_CHECK( rc, PDERROR, "Find collection space[ %s ] failed, rc: %d", pName, rc );

      suItem.init( pName, pCSCB->_su->CSID(), pCSCB->_su->LogicalCSID(), pCSCB->_su->CSUniqueID() );

      rc = pCSCB->_su->getEventHolder()->onDropCS( DMS_EVENT_MASK_ALL, SDB_EVT_OCCUR_BEFORE, suItem,
                                                   options, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to call before drop "
                   "collection space events, rc: %d",
                   rc );

      rc = _CSCBNameRemoveP2( pName, options, cb, dpsCB, pCSCB );
      if ( rc )
      {
         goto error;
      }
      else if ( !pCSCB )
      {
         rc = SDB_SYS;
         goto error;
      }
      extHandler = pCSCB->_su->data()->getExtDataHandler();
      if ( extHandler )
      {
         rc = extHandler->done( DMS_EXTOPR_TYPE_DROPCS, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "External operation on drop CS[ %s ] failed,"
                      " rc: %d",
                      pName, rc );
      }

      if ( ( NULL == options ) || ( !( options->isTakenOver() ) ) )
      {
         if ( removeFile )
         {
            // if remove file failed, we can do nothing
            rc = pCSCB->_su->remove();

            pCSCB->_su->getEventHolder()->onDropCS( DMS_EVENT_MASK_ALL, SDB_EVT_OCCUR_AFTER, suItem,
                                                    options, cb, dpsCB );
         }
         else
         {
            pCSCB->_su->close();

            pCSCB->_su->getEventHolder()->onUnloadCS( DMS_EVENT_MASK_ALL, cb, dpsCB );
         }

         pCSCB->_su->unsetEventHandlers();

         SDB_OSS_DEL pCSCB;
         PD_RC_CHECK( rc, PDERROR, "remove failed(rc=%d)", rc );
      }
      else
      {
         if ( removeFile )
         {
            pCSCB->_su->getEventHolder()->onDropCS( DMS_EVENT_MASK_ALL, SDB_EVT_OCCUR_AFTER, suItem,
                                                    options, cb, dpsCB );
         }
         else
         {
            pCSCB->_su->getEventHolder()->onUnloadCS( DMS_EVENT_MASK_ALL, cb, dpsCB );
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB__DELCSP2, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::dropCollectionSpaceP1( const CHAR *pName, _pmdEDUCB *cb, SDB_DPSCB *dpsCB )
   {
      return _delCollectionSpaceP1( pName, cb, dpsCB, TRUE );
   }

   INT32 _dmsMmapEngine::dropCollectionSpaceP1Cancel( const CHAR *pName,
                                                      _pmdEDUCB *cb,
                                                      SDB_DPSCB *dpsCB )
   {
      return _delCollectionSpaceP1Cancel( pName, cb, dpsCB );
   }

   INT32 _dmsMmapEngine::dropCollectionSpaceP2( const CHAR *pName,
                                                _pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB,
                                                dmsDropCSOptions *options )
   {
      return _delCollectionSpaceP2( pName, cb, dpsCB, TRUE, options );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECS, "_dmsMmapEngine::renameCollectionSpace" )
   INT32 _dmsMmapEngine::renameCollectionSpace( const CHAR *pName,
                                                const CHAR *pNewName,
                                                _pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECS );

      UINT32 csLID = ~0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isTransLocked = FALSE;
      SDB_DMS_CSCB *pCSCB = NULL;

      // get cs cb
      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, TRUE );
      _mutex.release_shared();
      if ( rc )
      {
         goto error;
      }

      SDB_ASSERT( pCSCB->_su, "su can't be null" );

      // lock transaction, standalone need lock trans here
      csLID = pCSCB->_su->LogicalCSID();
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict;
         rc =
            pTransCB->transLockTrySAgainstWrite( cb, csLID, DMS_INVALID_MBID, NULL, &lockConflict );
         if ( rc )
         {
            PD_LOG(
               PDERROR,
               "Failed to lock collection-space, rc:%d" OSS_NEWLINE
               "Conflict( representative ):" OSS_NEWLINE "   EDUID:  %llu" OSS_NEWLINE
               "   TID:    %u" OSS_NEWLINE "   LockId: %s" OSS_NEWLINE "   Mode:   %s" OSS_NEWLINE,
               rc, lockConflict._eduID, lockConflict._tid, lockConflict._lockID.toString().c_str(),
               lockModeToString( lockConflict._lockType ) );
            goto error;
         }
         isTransLocked = TRUE;
      }

      rc = renameCollectionSpaceP1( pName, pNewName, cb, dpsCB );
      if ( rc )
      {
         goto error;
      }

      rc = renameCollectionSpaceP2( pName, pNewName, cb, dpsCB );
      if ( rc )
      {
         renameCollectionSpaceP1Cancel( pName, pNewName, cb, dpsCB );
         goto error;
      }

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, csLID );
         isTransLocked = FALSE;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECS, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECSP1, "_dmsMmapEngine::renameCollectionSpaceP1" )
   INT32 _dmsMmapEngine::renameCollectionSpaceP1( const CHAR *pName,
                                                  const CHAR *pNewName,
                                                  _pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECSP1 );

#ifdef _WINDOWS

      INT32 rcNew = SDB_OK;
      SDB_DMS_CSCB *pCSCB = NULL;
      BOOLEAN aquired = FALSE;

      if ( !pName || !pNewName )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      aquireCSMutex( pName );
      aquired = TRUE;

      /// check old cs and new cs
      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, TRUE );
      rcNew = _CSCBNameLookup( pNewName, &pCSCB, NULL, TRUE );
      _mutex.release_shared();

      if ( rc )
      {
         goto error;
      }

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST;
      }
      if ( rcNew )
      {
         rc = rcNew;
         goto error;
      }

      /// rename
      rc = _CSCBRenameP1( pName, pNewName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection space at phase 1 [%s] to [%s], "
                   "rc: %d",
                   pName, pNewName, rc );

   done:
      if ( aquired )
      {
         releaseCSMutex( pName );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1, rc );
      return rc;
   error:
      goto done;

#else

      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1, rc );
      return rc;

#endif
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECSP1C, "_dmsMmapEngine::renameCollectionSpaceP1Cancel" )
   INT32 _dmsMmapEngine::renameCollectionSpaceP1Cancel( const CHAR *pName,
                                                        const CHAR *pNewName,
                                                        _pmdEDUCB *cb,
                                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECSP1C );

#ifdef _WINDOWS

      INT32 rcNew = SDB_OK;
      SDB_DMS_CSCB *pCSCB = NULL;
      BOOLEAN aquired = FALSE;

      if ( !pName || !pNewName )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      aquireCSMutex( pName );
      aquired = TRUE;

      /// check old cs and new cs
      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE );
      rcNew = _CSCBNameLookup( pNewName, &pCSCB, NULL, FALSE );
      _mutex.release_shared();

      if ( rc )
      {
         goto error;
      }

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST;
      }
      if ( rcNew )
      {
         rc = rcNew;
         goto error;
      }

      /// cancel rename
      rc = _CSCBRenameP1Cancel( pName, pNewName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to cancel rename collection space at phase 1 "
                   "[%s] to [%s], rc: %d",
                   pName, pNewName, rc );

   done:
      if ( aquired )
      {
         releaseCSMutex( pName );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1C, rc );
      return rc;
   error:
      goto done;

#else

      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP1C, rc );
      return rc;

#endif
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RENAMECSP2, "_dmsMmapEngine::renameCollectionSpaceP2" )
   INT32 _dmsMmapEngine::renameCollectionSpaceP2( const CHAR *pName,
                                                  const CHAR *pNewName,
                                                  _pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RENAMECSP2 );

#ifdef _WINDOWS

      INT32 rcNew = SDB_OK;
      SDB_DMS_CSCB *pCSCB = NULL;
      SDB_DMS_CSCB *pNewCSCB = NULL;
      IDmsExtDataHandler *extHandler = NULL;

      if ( !pName || !pNewName )
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      /// check old cs and new cs
      _mutex.get_shared();
      rc = _CSCBNameLookup( pName, &pCSCB, NULL, FALSE );
      rcNew = _CSCBNameLookup( pNewName, &pNewCSCB, NULL, FALSE );
      _mutex.release_shared();

      if ( rc )
      {
         goto error;
      }

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST;
      }
      if ( rcNew )
      {
         rc = rcNew;
         goto error;
      }

      SDB_ASSERT( pCSCB->_su, "su can't be null" );

      /// rename su
      rc = pCSCB->_su->renameCS( pNewName );
      PD_RC_CHECK( rc, PDERROR,
                   "Rename collection space[%s] to [%s] failed, "
                   "rc: %d",
                   pName, pNewName, rc );

      extHandler = pCSCB->_su->data()->getExtDataHandler();
      if ( extHandler )
      {
         rc = extHandler->onRenameCS( pName, pNewName, cb, NULL );
         PD_RC_CHECK( rc, PDERROR,
                      "External operation on rename cs failed, "
                      "rc: %d",
                      rc );
      }

      pCSCB->_su->getEventHolder()->onRenameCS( DMS_EVENT_MASK_ALL, pName, pNewName, cb, dpsCB );

      /// rename map
      rc = _CSCBRenameP2( pName, pNewName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection space at phase 2 [%s] to [%s], "
                   "rc: %d",
                   pName, pNewName, rc );

#else

      rc = _CSCBRename( pName, pNewName, cb, dpsCB );
      if ( rc )
      {
         goto error;
      }

#endif

      PD_LOG( PDEVENT, "Rename collection space[%s] to [%s] succeed", pName, pNewName );

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RENAMECSP2, rc );
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMmapEngine::restoreCollectionSpace( const CHAR *csName )
   {
      return _restoreCSCBFromTmpList( csName );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RTRNCSP1, "_dmsMmapEngine::returnCollectionSpaceP1" )
   INT32 _dmsMmapEngine::returnCollectionSpaceP1( dmsReturnOptions &options,
                                                  _pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RTRNCSP1 );

      INT32 rcNew = SDB_OK;
      SDB_DMS_CSCB *csCB = NULL;

      const CHAR *originName = options._recycleItem.getOriginName();
      const CHAR *recycleName = options._recycleItem.getRecycleName();

      /// check origin collection space and recycle collection space
      _mutex.get_shared();
      rc = _CSCBNameLookup( recycleName, &csCB, NULL, TRUE );
      // need check dropping storage unit either
      rcNew = _CSCBNameLookup( originName, &csCB, NULL, FALSE );
      _mutex.release_shared();

      PD_RC_CHECK( rc, PDERROR,
                   "Failed to check origin collection space "
                   "[%s], rc: %d",
                   recycleName, rc );

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST;
      }
      PD_CHECK( SDB_OK == rcNew, rcNew, error, PDERROR,
                "Failed to check "
                "recycle collection space [%s], rc: %d",
                originName, rcNew );

      /// prepare rename
      rc = _CSCBRenameP1( recycleName, originName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection space at phase 1 "
                   "from [%s] to [%s], rc: %d",
                   recycleName, originName, rc );

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RTRNCSP1, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RTRNCSP1CANCEL, "_dmsMmapEngine::returnCollectionSpaceP1Cancel" )
   INT32 _dmsMmapEngine::returnCollectionSpaceP1Cancel( dmsReturnOptions &options,
                                                        _pmdEDUCB *cb,
                                                        SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RTRNCSP1CANCEL );

      INT32 rcNew = SDB_OK;

      SDB_DMS_CSCB *origCSCB = NULL;
      SDB_DMS_CSCB *recyCSCB = NULL;

      const CHAR *originName = options._recycleItem.getOriginName();
      const CHAR *recycleName = options._recycleItem.getRecycleName();

      /// check old cs and new cs
      _mutex.get_shared();
      rc = _CSCBNameLookup( recycleName, &recyCSCB, NULL, FALSE );
      rcNew = _CSCBNameLookup( originName, &origCSCB, NULL, FALSE );
      _mutex.release_shared();

      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to check recycle collection "
                   "space [%s], rc: %d",
                   recycleName, rc );

      if ( SDB_DMS_CS_NOTEXIST == rcNew )
      {
         rcNew = SDB_OK;
      }
      else if ( SDB_OK == rcNew )
      {
         rcNew = SDB_DMS_CS_EXIST;
      }
      PD_CHECK( SDB_OK == rcNew, rcNew, error, PDWARNING,
                "Failed to check "
                "origin collection space [%s], rc: %d",
                originName, rcNew );

      /// cancel rename
      rc = _CSCBRenameP1Cancel( recycleName, originName, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to cancel rename collection space "
                   "at phase 1 [%s] to [%s], rc: %d",
                   recycleName, originName, rc );

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RTRNCSP1CANCEL, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RTRNCSP2, "_dmsMmapEngine::returnCollectionSpaceP2" )
   INT32 _dmsMmapEngine::returnCollectionSpaceP2( dmsReturnOptions &options,
                                                  _pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RTRNCSP2 );

      SDB_DMS_CSCB *csCB = NULL;
      dmsStorageUnitID suID = DMS_INVALID_SUID;
      UINT32 csLID = ~0;

      dpsTransCB *transCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isLogReserved = FALSE, isSULocked = FALSE;
      UINT32 logRecSize = 0;
      dpsMergeInfo info;
      dpsLogRecord &record = info.getMergeBlock().record();

      const CHAR *originName = options._recycleItem.getOriginName();
      const CHAR *recycleName = options._recycleItem.getRecycleName();

      PD_LOG( PDDEBUG,
              "Start return collection space P2 [origin: %s, "
              "recycle %s]",
              originName, recycleName );

      /// reserved log-size
      if ( NULL != dpsCB )
      {
         rc = options.prepareOptions();
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to prepare return options, "
                      "rc: %d",
                      rc );

         rc = dpsReturn2Record( &( options._boOptions ), record );
         PD_RC_CHECK( rc, PDERROR, "Failed to build log record, rc: %d", rc );

         rc = dpsCB->checkSyncControl( record.alignedLen(), cb );
         PD_RC_CHECK( rc, PDERROR, "Failed to check sync control, rc: %d", rc );

         logRecSize = record.alignedLen();
         rc = transCB->reservedLogSpace( logRecSize, cb );
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to reserve log space [length %u], "
                      "rc: %d",
                      rc );

         isLogReserved = TRUE;
      }

      {
         ossScopedLock lock( &_mutex, SHARED );

         rc = _CSCBNameLookup( recycleName, &csCB, &suID, FALSE );
         SDB_ASSERT( SDB_OK == rc, "impossible" );
         PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit [%s], rc: %d", recycleName, rc );

         if ( csCB != _tmpCscbVec[ suID ] )
         {
            SDB_ASSERT( FALSE, "impossible" );
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                      "Failed to check "
                      "storage unit [%s], it is not deleting",
                      recycleName );
         }

         csLID = csCB->_su->LogicalCSID();
      }

      /// rename su
      rc = csCB->_su->renameCS( originName );
      PD_RC_CHECK( rc, PDERROR,
                   "Rename collection space [%s] to [%s] failed, "
                   "rc: %d",
                   recycleName, originName, rc );

      /// rename map
      rc = _CSCBRenameP2( recycleName, originName, cb, NULL );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to rename collection space at "
                   "phase 2 [%s] to [%s], rc: %d",
                   recycleName, originName, rc );

      {
         ossScopedLock lock( &_mutex, SHARED );
         dmsStorageUnitID tmpSUID = DMS_INVALID_SUID;

         rc = _CSCBNameLookup( originName, &csCB, &tmpSUID, TRUE );
         SDB_ASSERT( SDB_OK == rc, "impossible" );
         PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit [%s], rc: %d", originName, rc );

         if ( suID != tmpSUID )
         {
            SDB_ASSERT( FALSE, "impossible" );
            PD_CHECK( FALSE, SDB_SYS, error, PDERROR,
                      "Failed to check "
                      "storage unit [%s], different suID, current [%d], "
                      "expected [%d]",
                      originName, suID, tmpSUID );
         }

         _latchVec[ suID ]->lock_w();
         isSULocked = TRUE;

         // enable MVCC when return
         csCB->_su->setMVCCSupport( transCB->isMVCCOn() );
      }

      if ( dpsCB )
      {
         info.setInfoEx( csLID, ~0, DMS_INVALID_EXTENT, cb );
         rc = dpsCB->prepare( info );
         PD_RC_CHECK( rc, PDERROR, "Failed to insert DPS log, rc: %d", rc );

         _latchVec[ suID ]->release_w();
         isSULocked = FALSE;

         dpsCB->writeData( info );
      }

      PD_LOG( PDDEBUG,
              "Finish return collection space P2 [origin: %s, "
              "recycle %s]",
              originName, recycleName );

   done:
      if ( isSULocked )
      {
         _latchVec[ suID ]->release_w();
         isSULocked = FALSE;
      }
      if ( isLogReserved )
      {
         transCB->releaseLogSpace( logRecSize, cb );
         isLogReserved = FALSE;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RTRNCSP2, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_RTRNCS, "_dmsMmapEngine::returnCollectionSpace" )
   INT32 _dmsMmapEngine::returnCollectionSpace( dmsReturnOptions &options,
                                                _pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_RTRNCS );

      UINT32 csLID = ~0;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB();
      BOOLEAN isTransLocked = FALSE;
      SDB_DMS_CSCB *csCB = NULL;

      const CHAR *originName = options._recycleItem.getOriginName();
      const CHAR *recycleName = options._recycleItem.getRecycleName();

      PD_LOG( PDDEBUG,
              "Start return collection space [origin: %s, "
              "recycle %s]",
              originName, recycleName );

      // get cs cb
      _mutex.get_shared();
      rc = _CSCBNameLookup( recycleName, &csCB, NULL, TRUE );
      _mutex.release_shared();
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get storage unit [%s], "
                   "rc: %d",
                   recycleName, rc );

      SDB_ASSERT( csCB->_su, "su can't be null" );

      // lock transaction, standalone need lock trans here
      csLID = csCB->_su->LogicalCSID();
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict;
         rc = pTransCB->transLockTryX( cb, csLID, DMS_INVALID_MBID, NULL, &lockConflict );
         if ( rc )
         {
            PD_LOG(
               PDERROR,
               "Failed to lock collection-space, rc:%d" OSS_NEWLINE
               "Conflict( representative ):" OSS_NEWLINE "   EDUID:  %llu" OSS_NEWLINE
               "   TID:    %u" OSS_NEWLINE "   LockId: %s" OSS_NEWLINE "   Mode:   %s" OSS_NEWLINE,
               rc, lockConflict._eduID, lockConflict._tid, lockConflict._lockID.toString().c_str(),
               lockModeToString( lockConflict._lockType ) );
            goto error;
         }
         isTransLocked = TRUE;
      }

      rc = returnCollectionSpaceP1( options, cb, dpsCB );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to return collection space P1, "
                   "from [%s] to [%s], rc: %d",
                   recycleName, originName, rc );

      rc = returnCollectionSpaceP2( options, cb, dpsCB );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR,
                 "Failed to return collection space P2 from [%s] "
                 "to [%s], rc: %d",
                 recycleName, originName, rc );
         returnCollectionSpaceP1Cancel( options, cb, dpsCB );
         goto error;
      }

      PD_LOG( PDDEBUG,
              "Finish return collection space [origin: %s, "
              "recycle %s]",
              originName, recycleName );

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, csLID );
         isTransLocked = FALSE;
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_RTRNCS, rc );
      return rc;

   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPCLSIMPLE, "_dmsMmapEngine::dumpInfo" )
   INT32 _dmsMmapEngine::dumpInfo( MON_CL_SIM_LIST &collectionList, BOOLEAN sys )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DUMPCLSIMPLE );
      INT32 rc = SDB_OK;
      CSCB_MAP_CONST_ITER it;

      ossScopedLock _lock( &_mutex, SHARED );

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL;
         dmsStorageUnitID suID = ( *it ).second;

         SDB_DMS_CSCB *cscb = _cscbVec[ suID ];
         if ( !cscb )
         {
            continue;
         }
         su = cscb->_su;
         SDB_ASSERT( su, "storage unit pointer can't be NULL" );

         if ( ( !sys && dmsIsSysCSName( su->CSName() ) ) ||
              ( ossStrcmp( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            continue;
         }
         rc = su->dumpInfo( collectionList, sys );
         if ( rc )
         {
            goto error;
         }
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPCLSIMPLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPCSSIMPLE, "_dmsMmapEngine::dumpInfo" )
   INT32 _dmsMmapEngine::dumpInfo( MON_CS_SIM_LIST &csList,
                                   BOOLEAN sys,
                                   BOOLEAN dumpCL,
                                   BOOLEAN dumpIdx )
   {
      INT32 rc = SDB_OK;
      INT32 tmpRC = SDB_OK;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DUMPCSSIMPLE );

      ossPoolVector< ossPoolString > csNameVec;
      dmsStorageUnit *su = NULL;
      dmsStorageUnitID suID = DMS_INVALID_CS;

      rc = _getCSList( csNameVec );
      if ( rc )
      {
         goto error;
      }

      for ( ossPoolVector< ossPoolString >::iterator itr = csNameVec.begin();
            itr != csNameVec.end(); ++itr )
      {
         // As we do not take the cs metadata mutex here, so cs may have been
         // dropped after we get the names.
         tmpRC = nameToSUAndLock( itr->c_str(), suID, &su );
         if ( tmpRC )
         {
            if ( SDB_DMS_CS_NOTEXIST != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to lock collectionspace[%s], rc: %d", itr->c_str(),
                       tmpRC );
            }
            continue;
         }

         SDB_ASSERT( su, "storage unit pointer can't be NULL" );
         if ( ( !sys && dmsIsSysCSName( su->CSName() ) ) ||
              ( ossStrcmp( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            suUnlock( suID );
            continue;
         }

         monCSSimple cs;
         rc = su->dumpInfo( cs, sys, dumpCL, dumpIdx );
         try
         {
            csList.insert( cs );
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Dump collectionspaces occur exception: %s", e.what() );
            rc = SDB_OOM;
         }

         suUnlock( suID );

         if ( rc )
         {
            goto error;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPCSSIMPLE, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO, "_dmsMmapEngine::dumpInfo" )
   INT32 _dmsMmapEngine::dumpInfo( MON_CL_LIST &collectionList, BOOLEAN sys )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DUMPINFO );

      INT32 rc = SDB_OK;
      CSCB_MAP_CONST_ITER it;

      ossScopedLock _lock( &_mutex, SHARED );

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL;
         dmsStorageUnitID suID = ( *it ).second;

         SDB_DMS_CSCB *cscb = _cscbVec[ suID ];
         if ( !cscb )
         {
            continue;
         }
         su = cscb->_su;
         SDB_ASSERT( su, "storage unit pointer can't be NULL" );

         if ( ( !sys && dmsIsSysCSName( su->CSName() ) ) ||
              ( ossStrcmp( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            continue;
         }
         rc = su->dumpInfo( collectionList, sys );
         if ( rc )
         {
            goto error;
         }
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPINFO, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO2, "_dmsMmapEngine::dumpInfo" )
   INT32 _dmsMmapEngine::dumpInfo( MON_CS_LIST &csList, BOOLEAN sys )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DUMPINFO2 );

      INT32 rc = SDB_OK;
      CSCB_MAP_CONST_ITER it;

      ossScopedLock _lock( &_mutex, SHARED );

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL;
         dmsStorageUnitID suID = ( *it ).second;
         SDB_DMS_CSCB *cscb = _cscbVec[ suID ];
         if ( !cscb )
         {
            continue;
         }
         su = cscb->_su;
         SDB_ASSERT( su, "storage unit pointer can't be NULL" );
         if ( !sys && dmsIsSysCSName( cscb->_name ) )
         {
            continue;
         }
         // do not dump temp cs
         else if ( dmsIsSysCSName( cscb->_name ) &&
                   0 == ossStrcmp( cscb->_name, SDB_DMSTEMP_NAME ) )
         {
            continue;
         }
         monCollectionSpace cs;
         rc = su->dumpInfo( cs, sys );
         try
         {
            csList.insert( cs );
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Dump collectionspaces occur exception: %s", e.what() );
            rc = SDB_OOM;
         }

         if ( rc )
         {
            goto error;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPINFO2, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO3, "_dmsMmapEngine::dumpInfo" )
   INT32 _dmsMmapEngine::dumpInfo( MON_SU_LIST &storageUnitList, BOOLEAN sys )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DUMPINFO3 );

      INT32 rc = SDB_OK;
      CSCB_MAP_CONST_ITER it;

      ossScopedLock _lock( &_mutex, SHARED );

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL;
         dmsStorageUnitID suID = ( *it ).second;
         SDB_DMS_CSCB *cscb = _cscbVec[ suID ];
         monStorageUnit storageUnit;
         if ( !cscb )
         {
            continue;
         }
         su = cscb->_su;
         SDB_ASSERT( su, "storage unit pointer can't be NULL" );

         if ( ( !sys && dmsIsSysCSName( su->CSName() ) ) ||
              ( ossStrcmp( su->CSName(), SDB_DMSTEMP_NAME ) == 0 ) )
         {
            continue;
         }

         su->dumpInfo( storageUnit );

         try
         {
            storageUnitList.insert( storageUnit );
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Dump storages occur exception: %s", e.what() );
            rc = SDB_OOM;
            goto error;
         }
      } // for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )

   done:
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_DUMPINFO3, rc );
      return rc;
   error:
      goto done;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_DUMPINFO4, "_dmsMmapEngine::dumpInfo" )
   void _dmsMmapEngine::dumpInfo( INT64 &totalFileSize )
   {
      totalFileSize = 0;
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_DUMPINFO4 );

      ossScopedLock _lock( &_mutex, SHARED );

      CSCB_MAP_CONST_ITER it;

      for ( it = _cscbNameMap.begin(); it != _cscbNameMap.end(); it++ )
      {
         dmsStorageUnit *su = NULL;
         dmsStorageUnitID suID = ( *it ).second;

         SDB_DMS_CSCB *cscb = _cscbVec[ suID ];
         if ( !cscb )
         {
            continue;
         }
         su = cscb->_su;
         SDB_ASSERT( su, "storage unit pointer can't be NULL" );
         totalFileSize += su->totalSize();
      }
      PD_TRACE_EXIT( SDB__SDB_DMSCB_DUMPINFO4 );
   }

   void _dmsMmapEngine::dumpPageMapCSInfo( MON_CSNAME_VEC &vecCS )
   {
      ossScopedLock _lock( &_mutex, SHARED );

      SDB_DMS_CSCB *cscb = NULL;
      for ( CSCB_MAP_CONST_ITER it = _cscbNameMap.begin(); it != _cscbNameMap.end(); ++it )
      {
         cscb = _cscbVec[ ( *it ).second ];
         if ( NULL == cscb || NULL == cscb->_su )
         {
            continue;
         }
         else if ( cscb->_su->index()->getPageMapUnit()->isEmpty() )
         {
            continue;
         }
         /// push back
         vecCS.push_back( monCSName( cscb->_name ) );
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLRSUCACHES, "_dmsMmapEngine::clearSUCaches" )
   void _dmsMmapEngine::clearSUCaches( UINT32 mask )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLRSUCACHES );

      MON_CS_SIM_LIST monCSList;
      dumpInfo( monCSList, TRUE, FALSE, FALSE );
      clearSUCaches( monCSList, mask );

      PD_TRACE_EXIT( SDB__SDB_DMSCB_CLRSUCACHES );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CLRSUCACHES_CSLIST, "_dmsMmapEngine::clearSUCaches" )
   void _dmsMmapEngine::clearSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLRSUCACHES_CSLIST );

      for ( MON_CS_SIM_LIST::const_iterator csIter = monCSList.begin(); csIter != monCSList.end();
            csIter++ )
      {
         INT32 rc = SDB_OK;
         dmsStorageUnit *pSU = NULL;
         const monCSSimple &monCS = ( *csIter );
         const CHAR *pCSName = monCS._name;
         dmsStorageUnitID suID = monCS._suID;
         dmsEventSUItem suItem( pCSName, suID, monCS._logicalID );

         rc = verifySUAndLock( &suItem, &pSU, EXCLUSIVE, OSS_ONE_SEC );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to get storage unit [%s], rc: %d", pCSName, rc );
            continue;
         }

         pSU->getEventHolder()->onClearSUCaches( mask );

         suUnlock( suID, EXCLUSIVE );
      }
      PD_TRACE_EXIT( SDB__SDB_DMSCB_CLRSUCACHES_CSLIST );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGSUCACHES, "_dmsMmapEngine::changeSUCaches" )
   void _dmsMmapEngine::changeSUCaches( UINT32 mask )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CHGSUCACHES );

      MON_CS_SIM_LIST monCSList;
      dumpInfo( monCSList, TRUE, FALSE, FALSE );
      changeSUCaches( monCSList, mask );

      PD_TRACE_EXIT( SDB__SDB_DMSCB_CHGSUCACHES );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_CHGSUCACHES_CSLIST, "_dmsMmapEngine::changeSUCaches" )
   void _dmsMmapEngine::changeSUCaches( const MON_CS_SIM_LIST &monCSList, UINT32 mask )
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CHGSUCACHES_CSLIST );

      for ( MON_CS_SIM_LIST::const_iterator csIter = monCSList.begin(); csIter != monCSList.end();
            csIter++ )
      {
         INT32 rc = SDB_OK;
         dmsStorageUnit *pSU = NULL;
         const monCSSimple &monCS = ( *csIter );
         const CHAR *pCSName = monCS._name;
         dmsStorageUnitID suID = monCS._suID;
         dmsEventSUItem suItem( pCSName, suID, monCS._logicalID );

         rc = verifySUAndLock( &suItem, &pSU, EXCLUSIVE, OSS_ONE_SEC );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to get storage unit [%s], rc: %d", pCSName, rc );
            continue;
         }

         pSU->getEventHolder()->onChangeSUCaches( mask );

         suUnlock( suID, EXCLUSIVE );
      }

      PD_TRACE_EXIT( SDB__SDB_DMSCB_CHGSUCACHES_CSLIST );
   }

   BOOLEAN _dmsMmapEngine::dispatchDictJob( dmsDictJob &job )
   {
      return _dictWaitQue.try_pop( job );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__SDB_DMSCB_PUSHDICTJOB, "_dmsMmapEngine::pushDictJob" )
   void _dmsMmapEngine::pushDictJob( dmsDictJob job )
   {
      _dictWaitQue.push( job );
   }

   void _dmsMmapEngine::setIxmKeySorterCreator( dmsIxmKeySorterCreator *creator )
   {
      _ixmKeySorterCreator = creator;
   }

   INT32 _dmsMmapEngine::createIxmKeySorter( INT64 bufSize,
                                             const _dmsIxmKeyComparer &comparer,
                                             dmsIxmKeySorter **ppSorter )
   {
      SDB_ASSERT( NULL != _ixmKeySorterCreator, "_ixmKeySorterCreator can't be NULL" );

      return _ixmKeySorterCreator->createSorter( bufSize, comparer, ppSorter );
   }

   void _dmsMmapEngine::releaseIxmKeySorter( dmsIxmKeySorter *pSorter )
   {
      SDB_ASSERT( NULL != _ixmKeySorterCreator, "_ixmKeySorterCreator can't be NULL" );

      if ( NULL != pSorter )
      {
         _ixmKeySorterCreator->releaseSorter( pSorter );
      }
   }

   void _dmsMmapEngine::setScannerCheckerCreator( IDmsScannerCheckerCreator *pCreator )
   {
      _scannerCheckerCreator = pCreator;
   }

   INT32 _dmsMmapEngine::createScannerChecker( UINT32 suLID,
                                               UINT32 mbLID,
                                               const CHAR *csName,
                                               const CHAR *clShortName,
                                               const CHAR *optrDesc,
                                               _pmdEDUCB *cb,
                                               IDmsScannerChecker **ppChecker )
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT( NULL != ppChecker, "output checker is invalid" );

      if ( NULL != _scannerCheckerCreator )
      {
         rc = _scannerCheckerCreator->createChecker( suLID, mbLID, csName, clShortName, optrDesc,
                                                     cb, ppChecker );
      }
      else
      {
         // use the default checker
         *ppChecker = _dmsGetDefaultScannerChecker();
      }

      return rc;
   }

   void _dmsMmapEngine::releaseScannerChecker( IDmsScannerChecker *pChecker )
   {
      if ( NULL != pChecker )
      {
         if ( pChecker == _dmsGetDefaultScannerChecker() )
         {
            // default checker, do nothing
         }
         else
         {
            SDB_ASSERT( NULL != _scannerCheckerCreator, "scanner checker creator is invalid" );
            _scannerCheckerCreator->releaseChecker( pChecker );
         }
      }
   }

   INT32 _dmsMmapEngine::getMaxDMSLSN( DPS_LSN_OFFSET &maxLsn )
   {
      INT32 rc = SDB_OK;
      MON_CS_SIM_LIST csList;
      MON_CS_SIM_LIST::iterator it;
      dmsStorageUnitID suID = DMS_INVALID_SUID;
      dumpInfo( csList, TRUE );

      for ( it = csList.begin(); it != csList.end(); ++it )
      {
         const monCSSimple &csInfo = *it;

         if ( 0 == ossStrcmp( csInfo._name, SDB_DMSTEMP_NAME ) )
         {
            continue;
         }

         dmsStorageUnit *su = NULL;
         suID = DMS_INVALID_SUID;
         rc = nameToSUAndLock( csInfo._name, suID, &su );
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to lock collectionspace[%s], rc: %d", csInfo._name, rc );
            goto error;
         }

         DPS_LSN_OFFSET tmpMaxLsn = DPS_INVALID_LSN_OFFSET;
         rtnRecoverUnit recoverUnit;
         rc = recoverUnit.init( su );
         PD_RC_CHECK( rc, PDERROR, "Failed to init recover unit:rc=%d", rc );

         tmpMaxLsn = recoverUnit.getMaxValidLsn();
         if ( DPS_INVALID_LSN_OFFSET != tmpMaxLsn )
         {
            if ( DPS_INVALID_LSN_OFFSET == maxLsn || maxLsn < tmpMaxLsn )
            {
               maxLsn = tmpMaxLsn;
            }
         }

         if ( DMS_INVALID_SUID != suID )
         {
            suUnlock( suID );
            suID = DMS_INVALID_SUID;
         }
      }

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         suUnlock( suID );
         suID = DMS_INVALID_SUID;
      }
      return rc;
   error:
      goto done;
   }

   void _dmsMmapEngine::clearAllCRUDCB()
   {
      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLEARALLCRUDCB );

      MON_CS_SIM_LIST monCSList;
      dumpInfo( monCSList, TRUE, FALSE, FALSE );
      for ( MON_CS_SIM_LIST::const_iterator csIter = monCSList.begin(); csIter != monCSList.end();
            csIter++ )
      {
         INT32 rc = SDB_OK;
         dmsStorageUnit *su = NULL;
         const monCSSimple &monCS = ( *csIter );
         dmsEventSUItem suItem( monCS._name, monCS._suID, monCS._logicalID );

         rc = verifySUAndLock( &suItem, &su, SHARED, OSS_ONE_SEC );
         if ( SDB_OK != rc )
         {
            PD_LOG( PDDEBUG, "Failed to get storage unit [%s], rc: %d", monCS._name, rc );
            continue;
         }

         su->clearMBCRUDCB();

         suUnlock( monCS._suID, SHARED );
      }

      PD_TRACE_EXIT( SDB__SDB_DMSCB_CLEARALLCRUDCB );
   }

   INT32 _dmsMmapEngine::clearSUCRUDCB( const CHAR *collectionSpace )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLEARSUCRUDCB );

      SDB_ASSERT( NULL != collectionSpace, "collection space is invalid" );

      dmsStorageUnitID suID = DMS_INVALID_SUID;
      dmsStorageUnit *su = NULL;

      rc = nameToSUAndLock( collectionSpace, suID, &su, SHARED, OSS_ONE_SEC );
      PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit [%s], rc: %d", collectionSpace, rc );

      su->clearMBCRUDCB();

   done:
      if ( DMS_INVALID_SUID != suID )
      {
         suUnlock( suID );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CLEARSUCRUDCB, rc );
      return rc;

   error:
      goto done;
   }

   INT32 _dmsMmapEngine::clearMBCRUDCB( const CHAR *collection )
   {
      INT32 rc = SDB_OK;

      PD_TRACE_ENTRY( SDB__SDB_DMSCB_CLEARMBCRUDCB );

      SDB_ASSERT( NULL != collection, "collection is invalid" );

      dmsStorageUnitID suID = DMS_INVALID_SUID;
      dmsStorageUnit *su = NULL;
      CHAR collectionSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 };
      CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 };
      dmsMBContext *mbContext = NULL;

      rc = rtnResolveCollectionName( collection, ossStrlen( collection ), collectionSpace,
                                     DMS_COLLECTION_SPACE_NAME_SZ, clShortName,
                                     DMS_COLLECTION_NAME_SZ );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to resolve collection name [%s], "
                   "rc: %d",
                   collection, rc );

      rc = nameToSUAndLock( collectionSpace, suID, &su, SHARED, OSS_ONE_SEC );
      PD_RC_CHECK( rc, PDERROR, "Failed to get storage unit [%s], rc: %d", collectionSpace, rc );

      rc = su->data()->getMBContext( &mbContext, clShortName, -1 );
      PD_RC_CHECK( rc, PDERROR, "Failed to get mb context [%s], rc: %d", collection, rc );

      mbContext->mbStat()->_crudCB.resetOnce();

   done:
      if ( NULL != su && NULL != mbContext )
      {
         su->data()->releaseMBContext( mbContext );
      }
      if ( DMS_INVALID_SUID != suID )
      {
         suUnlock( suID );
      }
      PD_TRACE_EXITRC( SDB__SDB_DMSCB_CLEARMBCRUDCB, rc );
      return rc;

   error:
      goto done;
   }

   INT32 _dmsMmapEngine::regHandler( _IDmsEventHandler *pHandler )
   {
      INT32 rc = SDB_OK;

      // only main thread can register handler
      SDB_ASSERT( pmdGetThreadEDUCB() && EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must register in main thread" );

      if ( NULL == pHandler )
      {
         goto done;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers.begin(); iter != _handlers.end(); ++iter )
      {
         if ( *iter == pHandler )
         {
            goto done;
         }
      }

      try
      {
         _handlers.push_back( pHandler );
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add handler, occur exception %s", e.what() );
         rc = ossException2RC( &e );
         goto error;
      }

   done:
      return rc;

   error:
      goto done;
   }

   void _dmsMmapEngine::unregHandler( _IDmsEventHandler *pHandler )
   {
      // only main thread can unregister handler
      SDB_ASSERT( pmdGetThreadEDUCB() && EDU_TYPE_MAIN == pmdGetThreadEDUCB()->getType(),
                  "Must register in main thread" );

      if ( NULL == pHandler )
      {
         return;
      }

      for ( DMS_HANDLER_LIST::iterator iter = _handlers.begin(); iter != _handlers.end(); ++iter )
      {
         if ( *iter == pHandler )
         {
            _handlers.erase( iter );
            break;
         }
      }
   }

   void _dmsMmapEngine::onConfigChange()
   {
      pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB();
      UINT32 syncInterval = optCB->getSyncInterval();
      UINT32 syncRecordNum = optCB->getSyncRecordNum();
      UINT32 syncDirtyRatio = optCB->getSyncDirtyRatio();
      BOOLEAN syncDeep = optCB->isSyncDeep();

      for ( vector< SDB_DMS_CSCB * >::iterator itr = _cscbVec.begin(); itr != _cscbVec.end();
            ++itr )
      {
         if ( NULL != ( *itr ) )
         {
            _dmsStorageUnit *su = ( *itr )->_su;
            su->setSyncConfig( syncInterval, syncRecordNum, syncDirtyRatio );
            su->setSyncDeep( syncDeep );

            /// update cache info
            dmsStorageInfo *pInfo = su->_getStorageInfo();
            utilCacheUnit *pCache = su->cacheUnit();

            pInfo->_overflowRatio = optCB->getOverFlowRatio();
            pInfo->_extentThreshold = optCB->getExtendThreshold() << 20;
            pInfo->_enableSparse = optCB->sparseFile();
            pInfo->_cacheMergeSize = optCB->getCacheMergeSize();
            pInfo->_pageAllocTimeout = optCB->getPageAllocTimeout();

            pCache->setAllocTimeout( pInfo->_pageAllocTimeout );
            pCache->updateMerge( pInfo->_directIO, pInfo->_cacheMergeSize );

            su->lob()->getLobData()->enableSparse( pInfo->_enableSparse );
         }
      }
   }

   /*
      get global SDB_DMSCB
   */
   SDB_DMSCB *sdbGetDMSCB()
   {
      static SDB_DMSCB s_dmsCB;
      return &s_dmsCB;
   }
} // namespace engine
