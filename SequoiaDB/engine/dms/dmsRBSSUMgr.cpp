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

   Source File Name = dmsRBSSUMgr.cpp

   Descriptive Name = DMS Rollback Segment Storage Unit Management

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   rollback segment creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/24/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsStorageUnit.hpp"
#include "../bson/bson.h"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "dmsRBSSUMgr.hpp"
#include "dmsScanner.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dmsStorageDataCapped.hpp"
#include "dpsUtil.hpp"
#include "ossMem.hpp"
#include "dmsRBSGCJob.hpp"
#include "dmsRBSMgr.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace bson ;
namespace fs = boost::filesystem ;

namespace engine
{

   // small wait time interval for rbs cl creation
   #define DMS_RBS_CREATECL_SMALL_INTERVAL ( 1 )

   _dmsRBSSUMgr::_dmsRBSSUMgr ()
      : _dmsSysSUMgr( sdbGetDMSCB() ),
        _index( 0 ),
        _rbsMgr( NULL ),
        _latch( MON_LATCH_RBSSUMGR_LATCH ) ,
        _numSyncAddCL( 0 )
   {
      // By default, start with second collection as the first one stores meta
      _currentCollection  = DMS_FIRST_RBS_CL ;
      _lastFreeCollection = DMS_MAX_RBS_CL ;
      // use default size for now, we may want to add config parm later on
      _maxCollectionSize  = DMS_DFT_RBSCL_SIZE ;

      _preparedCollection = DMS_FIRST_RBS_CL ;
      _prepInProgress = FALSE ;
   }

   // Initialization of RBS during node start up
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_INIT, "_dmsRBSSUMgr::init" )
   SINT32 _dmsRBSSUMgr::init( _dmsRBSMgr *rbsMgr, UINT32 index )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_INIT ) ;
      SINT32            rc    = SDB_OK ;
      dmsStorageUnitID  suID  = DMS_INVALID_CS ;
      pmdEDUCB         *eduCB = pmdGetThreadEDUCB() ;
      // we explicitly set dpsCB to NULL during init phase because we are 
      // going to assume that the SYSRBS and SYSRBS0000 exist. Every nodes 
      // with mvccon enabled must have both. So we will simply create them
      // during db start if they are not there. For newly started node, we
      // will also create SYSRBS0001 by default, which all done by this init
      // function. By using NULL dpsCB here, we will not generate LRs. During
      // runtime, we will generate LRs for creating SYSRBSxxxx and update of 
      // the meta pages (see updateMeta for details). 
      // If decision is changed, we can set to pmdGetKRCB()->getDPSCB()
      SDB_DPSCB        *dpsCB = NULL ;

      SDB_ASSERT ( _dmsCB, "dmsCB can't be NULL" ) ;
      SDB_ASSERT( NULL != rbsMgr, "rbsMgr is invalid" ) ;

      _rbsMgr = rbsMgr ;

      // initialize collection space name with index
      _index = index ;
      ossSnprintf( _metaCSName, 30, "%s%u", SDB_DMSRBS_NAME, index ) ;

      // exclusive lock temp cb. this function should be called during process
      // initialization, so it shouldn't be called in parallel by agents
      DMSSYSSUMGR_XLOCK() ;

      // first to load collection space
      rc = rtnLoadCollectionSpace( _metaCSName,
                                   pmdGetOptionCB()->getDbPath(),
                                   pmdGetOptionCB()->getIndexPath(),
                                   pmdGetOptionCB()->getLobPath(),
                                   pmdGetOptionCB()->getLobMetaPath(),
                                   eduCB, _dmsCB, FALSE ) ;
      // FIXME: remove
      PD_LOG ( PDDEBUG, "load RBS cs %s with rc:%d", _metaCSName, rc ) ;

      if ( SDB_OK == rc )
      {
         // Drop existing RBSCS
         rc = rtnDelCollectionSpaceCommand( _metaCSName, eduCB, _dmsCB,
                                            dpsCB, TRUE, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, 
                     "Failed to cleanup previous RBS collectionspace, rc: %d",
                     rc ) ;
            goto error ;
         }
      }
      else if ( SDB_DMS_CS_NOTEXIST != rc )
      {
         PD_LOG ( PDERROR, "Failed to load RBS CS, rc: %d",
                  rc ) ;
         goto error ;
      }

      // Create RBSCS and CL
      {
         UINT32 pageSize ;

#if SMALL_CAP
         pageSize = DMS_PAGE_SIZE4K ;
#else
         pageSize = DMS_PAGE_SIZE_MAX ;
#endif
         // Rollback Segment not exist, create one
         PD_LOG ( PDDEBUG, "Creating RBS cs %s.", _metaCSName ) ;

         rc = rtnCreateCollectionSpaceCommand( _metaCSName, eduCB, _dmsCB,
                                               dpsCB, UTIL_UNIQUEID_NULL,
                                               pageSize,
                                               DMS_DO_NOT_CREATE_LOB,
                                               DMS_STORAGE_CAPPED, TRUE ) ;

         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to create RBS collectionspace, rc: %d",
                  rc ) ;
            goto error ;
         }

         rc = rtnCollectionSpaceLock ( _metaCSName, _dmsCB, TRUE,
                                       &_su, suID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get collection space and lock for %s, "
                     "rc: %d", _metaCSName, rc ) ;
            goto error ;
         }

         // now create the first RBSCL, but need to build the options first
         rc = _initRBSCS( eduCB, dpsCB ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to create initial RBS CLs: %d",
                    rc ) ;
            goto error ;
         }

         // Memset hash bucket
         _rbsRecordBkt.reset() ;
      }

   done :
      if ( DMS_INVALID_CS != suID )
      {
         _dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_INIT, rc );
      return rc ;
   error :
      goto done ;
   }

   SINT32  _dmsRBSSUMgr::fini()
   {
      SINT32              rc = SDB_OK;
      return rc ;
   }

   // Create meta CL and first CL for SYSRBS
   // Caller must hold collectionspace lock
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__INITRBSCS, "_dmsRBSSUMgr::_initRBSCS" )
   SINT32 _dmsRBSSUMgr::_initRBSCS( pmdEDUCB *eduCB, SDB_DPSCB *dpsCB ) 
   {
      SINT32              rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__INITRBSCS );
      CHAR     clName[30] = {0} ;
      UINT16   collectionID = DMS_INVALID_CLID ;

      try
      {
         BSONObj        extOptions ;
         BSONObjBuilder builder ;
         UINT32         logicalID      = DMS_INVALID_CLID ;

         builder.append( FIELD_NAME_SIZE, _maxCollectionSize ) ;
         builder.append( FIELD_NAME_MAX, 0 ) ;
         builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
         extOptions = builder.done() ;

         _latchX() ;
         // add the first collection for RBS
         DMS_BUILD_RBS_CL_NAME( clName, DMS_FIRST_RBS_CL ) ;
         rc = _su->data()->addCollection ( clName, &collectionID,
                                           UTIL_UNIQUEID_NULL,
                                           DMS_MB_ATTR_CAPPED |
                                           DMS_MB_ATTR_NOIDINDEX,
                                           eduCB, dpsCB, 0, TRUE,
                                           UTIL_COMPRESSOR_INVALID,
                                           &logicalID,
                                           &extOptions ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to add RBS collection %s, rc: %d",
                     clName, rc ) ;
            goto error ;
         }
         // setup the curCL
         _currentCollection = DMS_FIRST_RBS_CL;
         _lastFreeCollection = DMS_MAX_RBS_CL ;
         _preparedCollection = DMS_FIRST_RBS_CL ;
         _numSyncAddCL.init(0) ;
         PD_LOG ( PDDEBUG, 
                  "Created RBS collection %s(%d) successfully, mbID(%d)",
                  clName, logicalID, collectionID );
      }

      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception when creating RBSCL: %s",
                 e.what() ) ;
         goto error ;
      }
      
   done :
      _releaseX() ;
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__INITRBSCS, rc );
      return rc ;
   error :
      goto done ;
   }

   // Find the current RBS CL and make sure it has enough space to append
   // a record with specified size. If the current one run out of space,
   // this function will automatically move to next CL.
   //
   // On normal return, the context for current RBSCL will be returned, but
   // not locked. Since it's unlocked, this is best effort to prepare
   // space, this is small chance in extremely busy system that the cl
   // maybe consumed quickly. It's the caller's responsibility to handle
   // failure or retry logic.
   // On error, nothing is held.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD, "_dmsRBSSUMgr::_prepareRBSCLForRecord" )
   SINT32 _dmsRBSSUMgr::_prepareRBSCLForRecord( UINT32          recordSize,
                                                pmdEDUCB     *  eduCB,
                                                SDB_DPSCB    *  dpsCB,
                                                dmsMBContext *& clContext,
                                                UINT16        & rbsclID )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD);
      SINT32       rc           = SDB_OK ;
      CHAR         clName[30]   = {0} ;
      BOOLEAN      mbLocked     = FALSE ;
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data();

      // candidate collection to save RBS record
      UINT16       tempCurCL    = DMS_MAX_RBS_CL ;

      SDB_ASSERT( NULL == clContext, "mbContext should be NULL" ) ;

   begin:
      // latch and lookup curCL for space first
      _latchS() ;
      tempCurCL = _currentCollection ;
      _releaseS() ;
      DMS_BUILD_RBS_CL_NAME( clName, tempCurCL ) ;

      SDB_ASSERT( ( clContext == NULL ), "stale clContext !" ) ;
      rc = _su->data()->getMBContext( &clContext, clName, SHARED ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get curCL(%s) mbLock, rc: %d",
                  clName, rc ) ;
         goto error ;
      }
      mbLocked = TRUE ;

      //if( !sd->clDataSpaceEnough( clContext, recordSize ) )
      if( sd->spaceEnough( clContext, recordSize ) )
      {
         clContext->mbUnlock() ;
         mbLocked = FALSE ;
      }
      else
      {
         // current CL does NOT have enough space, may need to create new CL
#ifdef _DEBUG
         {
            const dmsMBStatInfo *mbStatInfo =
                                   sd->getMBStatInfo( clContext->mbID() ) ;
            PD_LOG ( PDINFO,
                  "Out of space in %s(%d), allocating next RBSCL. "
                  "recordsize(%d), clfreespace(%d), clTotalPages(%d), "
                  "cltotalrecord(%d), cl max(%lld), squareroot(%d)",
                  clName, clContext->mbID(), recordSize,
                  mbStatInfo->_totalDataFreeSpace,
                  mbStatInfo->_totalDataPages,
                  mbStatInfo->_totalRecords,
                  (UINT64)sd->_options[clContext->mbID()]->_maxSize,
                  sd->pageSizeSquareRoot() ) ;
         }
#endif
         // release previous context
         _su->data()->releaseMBContext( clContext ) ;
         mbLocked = FALSE ;

         // take latch in X so that no one read stale data
         _latchX() ;
         // there is a prepared cl to use, move to it. 
         if ( _currentCollection != _preparedCollection ) 
         {
            _currentCollection = _preparedCollection ;
            _releaseX() ;
            goto begin ;
         }

         // It's possible that another thread has already moved up the
         // _curCollection or is preparing a new CL, we should just go
         // back and retry
         if ( ( _currentCollection != tempCurCL ) || _prepInProgress )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, 
                     "CurrentCollection(%d) changed from %d or "
                     "prepInProgress(%d). retry.",
                     _currentCollection, tempCurCL,
                     _prepInProgress, rc ) ;
#endif
            if ( _currentCollection == tempCurCL )
            {
               _releaseX() ;
               // sleep for short period time for in progress prepare to finish
               ossSleep(DMS_RBS_CREATECL_SMALL_INTERVAL) ;
            }
            else
            {
               _releaseX() ;
            }
            goto begin ;
         }

         // prepare/allocate the CL. Note that latch is released on return
         rc = _prepareRBSCL( eduCB, dpsCB, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get curCL mbLock, rc: %d", rc ) ;
            goto error ;
         }

         _numSyncAddCL.inc() ;

         // trigger GC event,
         if ( NULL != _rbsMgr && _rbsMgr->allowGC() )
         {
            rc = dmsStartAsyncRBSGC() ;
            if ( rc )
            {
               // log error message and reset to OK
               PD_LOG ( PDWARNING, "Failed to trigger GC, rc=%d ", rc ) ; 
               rc = SDB_OK ;
            }
         }

         // we have prepared new collection, just go back and retry
         goto begin ;
      } // end of !spaceEnough

      // remember _currentCollection
      rbsclID  = tempCurCL ;
   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD, rc );
      return rc ;

   error:
      if ( mbLocked )
      {
         _su->data()->releaseMBContext( clContext ) ;
      }
      goto done ;
   }

   // prepare or allocate the RBSCL. 
   // CALLER MUST HOLD _dmsRBSSUMgr::_latch in X
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__PREPARERBSCL, "_dmsRBSSUMgr::_prepareRBSCL" )
   SINT32 _dmsRBSSUMgr::_prepareRBSCL( pmdEDUCB   * eduCB,
                                       SDB_DPSCB  * dpsCB,
                                       BOOLEAN      updateCurCL )
   {
      SINT32       rc           = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__PREPARERBSCL);
      // set prepCL to next one
      UINT16       prepCL       = _preparedCollection + 1 ;
      CHAR         clName[30]   = {0} ;

      // setup prepare in progress 
      _prepInProgress = TRUE ;

      // if reached max, wrap to first one.
      if ( prepCL >= DMS_MAX_RBS_CL )
      {
         prepCL = DMS_FIRST_RBS_CL ;
      }

      // new curCL should not be overlap with last free.
      // A special case is lastFree never changed after system start, but the
      // prepare is trying to wrap around
      if ( ( prepCL == _lastFreeCollection ) || 
           ( DMS_MAX_RBS_CL == _lastFreeCollection &&
                DMS_FIRST_RBS_CL == prepCL ) )
      {
         rc = SDB_DMS_NOSPC ;
         PD_LOG ( PDWARNING,
                  "Run out of space in RBS collection lastFreeCL=%d,"
                  "curCL=%d, prepCL(full)=%d, rc=%d",
                  _lastFreeCollection, _currentCollection, 
                  _preparedCollection, rc ) ;
         _releaseX() ;   
         goto error ;
      }
      // now we can release the latch and do the real create. Releasing the
      // latch give better concurrency for sync thread to continue working
      // on curCL if it has space.
      _releaseX() ;

      try
      {
         // current CL does NOT have enough space, create the new CL
         BSONObjBuilder builder ;
         BSONObj        extOptions ;
         UINT32         logicalID    = DMS_INVALID_CLID ;
         UINT16         collectionID = DMS_INVALID_MBID ;

         builder.append( FIELD_NAME_SIZE, _maxCollectionSize ) ;
         builder.append( FIELD_NAME_MAX, 0 ) ;
         builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
         extOptions = builder.done() ;

         // add the collection for RBS
         DMS_BUILD_RBS_CL_NAME( clName, prepCL ) ;

         rc = _su->data()->addCollection ( clName, &collectionID,
                                           UTIL_UNIQUEID_NULL,
                                           DMS_MB_ATTR_CAPPED |
                                           DMS_MB_ATTR_NOIDINDEX,
                                           eduCB, dpsCB, 0, TRUE,
                                           UTIL_COMPRESSOR_INVALID,
                                           &logicalID,
                                           &extOptions ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to add RBS collection %s, rc: %d",
                     clName, rc ) ;

         PD_TRACE2 ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD,
                     PD_PACK_STRING(clName),
                     PD_PACK_UINT(logicalID) );
         PD_LOG ( PDDEBUG,
                  "Successfully created RBS collection %s, logicalID=%d, "
                  "mbID=%d, updateCurCL=%d",
                  clName, logicalID, collectionID, updateCurCL ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception when adding RBSCL : %s",
                 e.what() ) ;
         goto error ;
      }

      _latchX() ;
      // update the preparedCollection and unset the progress flag
      _preparedCollection = prepCL ;
      // only update currentCollection if caller want to
      if( updateCurCL )
      {
         _currentCollection = prepCL ;
      }
      _prepInProgress = FALSE ;      
      _releaseX() ;
   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__PREPARERBSCL, rc );
      return rc ;

   error:
      // unset prepare in progress on error. We may not neccessarily need
      // the latch because everyone else is only checking at this moment.
      // But to make sure PD_LOG dump correct value in other places, we
      // still take the latch. BTW, this is error code path. 
      _latchX() ;
      _prepInProgress = FALSE ;      
      _releaseX() ;
      
      goto done ;
   }

   BOOLEAN _dmsRBSSUMgr::_needPrepareRBSCL( BOOLEAN latched ) 
   {
      BOOLEAN need = FALSE ;
      if ( !latched )
      {
         _latchS() ;
      }

      if ( !_prepInProgress && ( _preparedCollection == _currentCollection ) )
      {
         // need prepare if there is no prepare in progress and we are using
         // the last prepared collection
         need = TRUE ;
      }

      if ( !latched )
      {
         _releaseS() ;
      }
      return need ; 
   }

   // allocate space for RBS record and return the beginning offset
   // TODO: current hold metaCL mblatch in X, consider X next
   // Dependency: caller must hold metaCL mblatch S so we are save use curCL
   // Will take curCL mblatch to see its logical ID, find if there is
   // enough space. Return the logical ID if there is and increase
   //    logical ID by size. Otherwise move to next CL.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__ALLOCRBSRECORDSPACE, "_dmsRBSSUMgr::_allocRBSRecordSpace" )
   SINT32 _dmsRBSSUMgr::_allocRBSRecordSpace( UINT32        size,
                                              dmsRBSOffset &newOffset,
                                              pmdEDUCB     *eduCB,
                                              SDB_DPSCB    *dpsCB,
                                              dmsMBContext *metaContext,
                                              dmsMBContext *&clContext )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__ALLOCRBSRECORDSPACE );
      SINT32        rc         = SDB_OK ;
      CHAR          clName[30] = {0} ;
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data();
      dmsRecordID   foundRID ;
      BOOLEAN       mbLocked   = FALSE ;

      // candidate collection to save RBS record
      UINT16        curClID = DMS_MAX_RBS_CL ;

      // mblatch will be held after this call
      rc = _prepareRBSCLForRecord( size, eduCB, dpsCB, clContext, curClID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to prepare space (size=%d), rc: %d, curCL:%d",
                  size, rc, _currentCollection ) ;
         goto error ;
      }
      mbLocked = TRUE ;

      // now current CL has enough space. reserve the space
      _su->data()->_allocRecordSpace( clContext, size, foundRID, eduCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR,
                  "Failed to allocate space in %s (size=%d), rc: %d",
                  clName, size, rc ) ;
         goto error ;
      }

      sd->_extLidAndOffset2RecLid( foundRID._extent, foundRID._offset,
                                   newOffset._logicalID );
      newOffset._clID = _currentCollection ;

      PD_LOG ( PDDEBUG,
               "Found space in %s (size=%d) for new record at %lld",
               clName, size, newOffset._logicalID ) ;

   done:
      if ( mbLocked )
      {
         // only unlatch, do not release the context
         clContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__ALLOCRBSRECORDSPACE, rc );
      return rc ;

   error:
      goto done ;
   }

   // Input Parm:
   //    csid, clid, rid: key to identify a record.
   //    clLID: decide the life of a CL. It is changed after drop/truncate
   //    recordTransID:  Record version, (last creation/update trans ID)
   //    ownerTransID: transaction to put the record to in memory old version
   //                  container and now to RBS
   //    data:  the data object of this version
   // Output Parm:
   //    rc: return code, SDB_OK or error code
   // Note that we currently use RID for hashing because we will fail already
   // started transaction thus each primary node use it's own method for 
   // hashing at run time. If we ever support newly voted primary to continue
   // servicing running transaction, we need something unique (liek lsn) across
   // node.
   // Append a record to the end of the RBS using insertRecord interface:
   // 1. based on CSID+RID, hash and find the proper bucket, lock the bucket
   // 2. build proper record
   // 3. invoke insertRecord to append the record to the destinated collection
   // 4. update bucket with the returned offset, unlock bucket
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_RBSAPPENDRECORD, "_dmsRBSSUMgr::rbsAppendRecord" )
   SINT32 _dmsRBSSUMgr::rbsAppendRecord ( dmsStorageUnitID   csid,
                                          UINT16                clid,
                                          UINT32                clLID,
                                          const dmsRecordID    &rid,
                                          UINT32               bucketID,
                                          DPS_TRANS_ID         &recordTransID,
                                          DPS_TRANS_ID         &ownerTransID,
                                          const BSONObj        &data,
                                          dmsTransLockCallback * callback )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_RBSAPPENDRECORD );
      SINT32        rc          = SDB_OK ;
      dmsRBSOffset  newOffset ;
      //dmsRBSRecord *rbsRecord ;
      dmsMBContext *clContext    = NULL ;
      CHAR          clName[30]   = {0} ;
      pmdEDUCB     *eduCB        = pmdGetThreadEDUCB() ;
      // Note: we may want to setup dpsCB so that the replica can replay the
      // addCollection and insert log record. This is currently disabled as 
      // we decided to fail the transaction after failover to new primary node.
      // If we decide to life this restriction, we will setup the proper dpsCB
      //SDB_DPSCB    *dpsCB = pmdGetKRCB()->getDPSCB() ;
      SDB_DPSCB    *dpsCB = NULL ;
      UINT32        bkt          = bucketID ;
      BSONObjBuilder builder ;
      utilInsertResult insertResult ;
      BSONObj       record ;
      BOOLEAN       bktLatched   = FALSE ;
      UINT32        recSize ;
      dmsRBSOffset  location ;
      // type conversion for following use
      SINT32        cl           = clid ;
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data();
      // candidate collection to save RBS record
      UINT16        curClID = DMS_MAX_RBS_CL ;

      PD_TRACE4( SDB__DMSRBSSUMGR_RBSAPPENDRECORD,
                 PD_PACK_UINT(csid),
                 PD_PACK_UINT(clid),
                 PD_PACK_UINT(rid._extent),
                 PD_PACK_UINT(rid._offset) ) ;

      // build BSON record to include following:
      // recordKey, transID, preOffset and original recordData
      try
      {
         // use array of 4 int to store on disk
         builder.append( FIELD_NAME_RBS_RECORD_KEY,
                         BSON_ARRAY( (SINT32)csid <<
                                     (SINT32)cl   <<
                                     rid._extent  <<
                                     rid._offset ) ) ;

         // has to cast to INT64
         //builder.append( FIELD_NAME_RBS_RECORD_LSN_OFFSET,
         //                (INT64)lsn ) ;
         builder.append( FIELD_NAME_RBS_RECORD_CLLID,
                         clLID ) ;

         // append transaction ID as BSON sub-object
         rc = dpsTransIDToBSON( recordTransID, builder,
                                FIELD_NAME_RBS_RECORD_TRANSID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build transaction ID into BSON "
                      "format, rc: %d", rc ) ;

         // the above can be done outside of latch, anything action related
         // to existing bucket value must be done under bkt latch protection
         _rbsRecordBkt.lock( bkt );
         bktLatched = TRUE ;

         builder.append( FIELD_NAME_RBS_PRERECORD_CL,
                         _rbsRecordBkt.getOffset( bkt )._clID ) ;
         builder.append( FIELD_NAME_RBS_PRERECORD_OFFSET,
                         _rbsRecordBkt.getOffset( bkt )._logicalID ) ;
         builder.append( FIELD_NAME_RBS_RECORD_DATA, data ) ;
         record = builder.done() ;
         recSize = record.objsize() + DMS_RECORD_CAP_METADATA_SZ ;
         recSize = ossAlignX( recSize, 4 ) ;

      retry:
         // move to proper RBS CL which has enough space
         // We do not hold the cl lock on return as insertRecord would
         // take the lock
         rc = _prepareRBSCLForRecord( recSize, eduCB, dpsCB, clContext, curClID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to prepare RBSCL for record, rc: %d",
                     rc ) ;
            goto error ;
         }

         // upon _prepareRBSCLForRecord successfully return,
         // the curClID shall be set
         SDB_ASSERT( ( curClID != DMS_MAX_RBS_CL ), "Invalid clID !" ) ;

         // insert the record to RBS
         rc = _su->insertRecord ( clName, record, eduCB, dpsCB,
                                  TRUE, TRUE, clContext, -1, &insertResult ) ;
         if ( rc )
         {
            // there could be timing hole that the cl we want to use was
            if ( SDB_OSS_UP_TO_LIMIT == rc )
            {
               PD_LOG ( PDEVENT,
                        "Failed to insert into RBS(%d), going to retry. rc: %d",
                        clContext->mbID(), rc ) ;
               _su->data()->releaseMBContext( clContext ) ;
               goto retry ;
            }

            PD_LOG ( PDERROR, "Failed to insert into RBS, rc: %d",
                     rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception when insert record to RBS: %s",
                 e.what() ) ;
         goto error ;
      }
      // update the _maxGlobTransID as needed
      clContext->mbStat()->updateGlobTransIDWithComp( ownerTransID ) ;

      _su->data()->releaseMBContext( clContext ) ;

      // Update bucket to point to the new record location
      {
         SINT32       ext, offset ;
         insertResult.getInsertLoc( ext, offset ) ;
         sd->_extLidAndOffset2RecLid( ext, offset, location._logicalID ) ;
         location._clID = curClID ;
         _rbsRecordBkt.setOffset( location, bkt ) ;
      }
      // unlock the bucket
      _rbsRecordBkt.release( bkt ) ;
      bktLatched  = FALSE ;

      if ( callback )
      {
         callback->setRBSRecordOffset( location ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_RBSAPPENDRECORD, rc );
      return  rc ;
   error:
      if ( bktLatched )
      {
         _rbsRecordBkt.release( bkt ) ;
      }
      if ( NULL != clContext )
      {
         _su->data()->releaseMBContext( clContext ) ;
      }
      goto done ;
   }

   // Given a transactionID and beginning of a record chain, find a visiable record
   // This method uses fetch method from cappedCL.
   // User can provide start and end position of the RBSRecord, index scan 
   // normally does this as it need make sure the index value matches the 
   // old version of the record.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_RBSGETRECORD, "_dmsRBSSUMgr::rbsGetRecord" )
   SINT32 _dmsRBSSUMgr::rbsGetRecord ( dmsStorageUnitID  csid,
                                       UINT16            clid,
                                       UINT32            clLID,
                                       dmsRecordID      &rid,
                                       UINT32             bucketID,
                                       DPS_TRANS_ID     &transid,
                                       BOOLEAN          &found,
                                       dmsRecordData    &recordData,
                                       dmsRBSOffset     &startPos,
                                       dmsRBSOffset     &endPos,
                                       const DPS_TRANS_ID &diskRecordTransID )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_RBSGETRECORD );
      SINT32        rc         = SDB_OK ;
      UINT32        bkt        = bucketID ;
      dmsMBContext *context    = NULL ;
      pmdEDUCB     *eduCB      = pmdGetThreadEDUCB() ;
      dmsRecordRW   recordRW ;
      dmsRBSOffset  position ;
      CHAR          clName[30] = {0} ;
      dmsStorageDataCapped *sd = (dmsStorageDataCapped*)_su->data();
      dmsExtentID   extID      = DMS_INVALID_EXTENT ;
      dmsOffset     offset     = DMS_INVALID_OFFSET;
      // decide if we were provided with start and finish position in RBS.
      // index scan does this type of search. Either of this can be valid.
      BOOLEAN       useRange   = ( startPos.isValid() || endPos.isValid() );
      DPS_TRANS_ID  ownerTransID, lastRecTransID ;

      lastRecTransID = diskRecordTransID ;

#ifdef _DEBUG
      PD_LOG ( PDDEBUG,
               "Transaction (%s) tries to find a proper version from RBS, "
               "csid(%d), clid(%d), record rid(%d, %d), cllid(%d), "
               "disk transid (%s)",
               dpsTransIDToString( transid ).c_str(),
               csid, clid, rid._extent, rid._offset, clLID,
               dpsTransIDToString( lastRecTransID ).c_str() ) ;
#endif
      SDB_ASSERT( pmdGetOptionCB()->globTransOn() && 
                  pmdGetOptionCB()->mvccOn() , 
                  "global transaction should be on" ) ;

      found = FALSE ;
      // 1. From the hash table, find the position
      // lock the bucket
      if ( useRange && startPos.isValid() )
      {
         position = startPos ;
      }
      else
      {
         // range search without valid start position means start from
         // the offset in hash bucket
         _rbsRecordBkt.lock( bkt );
         position = _rbsRecordBkt.getOffset( bkt );

         // When is it safe to release the latch? do we allow anybody else to
         // insert/free the position while  we got a position and are still
         // using it.
         // I "think" it should be ok as long as insert guy holds recordLock
         // in X and reader already went through the lock request but failed
         // thus decided to use a version of old record, AND the version is
         // already stored in RBS AND hasn't been recycled yet.
         _rbsRecordBkt.release( bkt ) ;
      }

      do
      {
         // finish if the hasbucket entry is invalid Or hit the end of range
         // Or the position is pointing to a version no longer exist
         // (CL has been recycled)
         if ( !position.isValid() ||
              ( useRange && ( endPos == position ) ) ||
              _rbsPositionExpired( position ) )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG,
                     "no more older version found, useRange(%d)"
                     "position(%d, %llu), endPos(%d, %llu)"
                     "curCL(%d), lastFreeCL(%d)",
                     useRange,  position._clID, position._logicalID,
                     endPos._clID, endPos._logicalID,
                     _currentCollection, _lastFreeCollection ) ;
#endif
            goto done ;
         }

         // calculate extID and offset from logicalID
         sd->_recLid2ExtLidAndOffset( position._logicalID, extID, offset ) ;

         try
         {
            // 2. read record from the position
            dmsRecordID   recordID( extID, offset ) ;
            BSONObj       cappedRecord ;
            DPS_TRANS_ID  recordTransID ;
            BSONElement   eleTransID;
            BSONElement   eleCLLID;
            BSONElement   eleLsnOffset;
            BSONElement   eleKey;

            DMS_BUILD_RBS_CL_NAME( clName, position._clID ) ;
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, "Try %s at location(%d,%d) for older version",
                     clName, extID, offset  ) ;
#endif
            rc = _su->data()->getMBContext( &context, clName, SHARED ) ;
            if ( rc )
            {
               // if the error is due to expired position(CL has been recycled)
               // means no more old version record found, may simply exit.
               if ( _rbsPositionExpired( position ) ||
                    ( SDB_DMS_NOTEXIST == rc ) )
               {
                  rc = SDB_OK ;
                  goto done ;
               }
               PD_LOG ( PDERROR, "Failed to get mbLatch for %s, rc=%d",
                        clName, rc ) ;
               goto error ;
            }

            // get the record's onwer transid as well.
            // Caller current will skip the record if the ownerTransID
            // is visiable(see afterLockAquired)
            rc = sd->fetch( context, recordID, cappedRecord,
                            eduCB, FALSE, &ownerTransID ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to fetch rbsrecord, rid(%d, %d), rc=%d, "
                        "useRange(%d), position(%d, %llu), "
                        "endPos(%d, %llu), startPos(%d, %llu), "
                        "curCL(%d), lastFreeCL(%d), bucketID(%d); "
                        "csid(%d), clid(%d), record rid(%d, %d), cllid(%d)" ,
                        extID, offset, rc,
                        useRange, position._clID, position._logicalID,
                        endPos._clID, endPos._logicalID,
                        startPos._clID, startPos._logicalID,
                        _currentCollection, _lastFreeCollection, bkt,
                        csid, clid, rid._extent, rid._offset, clLID ) ;
               goto error ;
            }

            // 3. parse the dataRecord to figure out record key and visiability
            eleTransID = cappedRecord.getField(FIELD_NAME_RBS_RECORD_TRANSID) ;
            eleCLLID = cappedRecord.getField(FIELD_NAME_RBS_RECORD_CLLID) ;

            eleKey = cappedRecord.getField( FIELD_NAME_RBS_RECORD_KEY ) ;
            vector< BSONElement > vecKey = eleKey.Array() ;

            // parse transaction ID
            PD_CHECK( Object == eleTransID.type(), SDB_SYS, error, PDERROR,
                      "Failed to parse transaction ID from record, field [%s] "
                      "should be an object", FIELD_NAME_RBS_RECORD_TRANSID ) ;
            rc = dpsTransIDFromBSON( eleTransID.embeddedObject(),
                                     recordTransID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse transaction ID, rc: %d",
                         rc ) ;

            // 4. setup return data for qualified version
            // use clLID to determin the life of the record
            if ( ( vecKey[0].numberInt() == csid ) &&
                 ( vecKey[1].numberInt() == clid ) &&
                 ( vecKey[2].numberInt() == rid._extent ) &&
                 ( vecKey[3].numberInt() == rid._offset ) )
            {
               UINT32 recCLLID = eleCLLID.numberInt() ;
               if ( recCLLID != clLID )
               {
                  // logical ID of collection is different
                  // which means the collection had been truncated or
                  // recreated, earlier records should not been seen
                  // by current transaction
#ifdef _DEBUG
                  PD_LOG ( PDDEBUG, "collection's logical ID is different, "
                           "current [%u], record [%u], "
                           "no more older version found", clLID, recCLLID ) ;
#endif
                  _su->data()->releaseMBContext( context ) ;
                  context = NULL ;
                  goto done ;
               }
               else
               {
                  BOOLEAN isVisible = FALSE ;

                  // check visibility for current transaction against
                  // owner transaction
                  //
                  // 3 cases here
                  //
                  // - normal case: all records were created by committed
                  //   transactions
                  //
                  //   disk ( rec: T3 )
                  //   RBS  ( owner: T3, rec: T2 ) ->
                  //        ( owner: T2, rec: T1 ) ->
                  //        ( owner: T1, rec: T0 )
                  //
                  //   in this case, all owner transaction IDs equal to
                  //   record transaction ID of previous searched item
                  //
                  // - rollback case: a rollback happened among committed
                  //   transactions
                  //
                  //   disk ( rec: T3 )
                  //   RBS  ( owner: T3, rec: T2 ) ->
                  //        ( owner: T33, rec: T2 ) ->
                  //        ( owner: T2, rec: T1 ) ->
                  //        ( owner: T1, rec: T0 )
                  //
                  //   in this case, a record transaction ID equals to record
                  //   transaction ID of previous searched item
                  //   ( e.g. T33 is a rollbacked transaction )
                  //
                  // - non-transaction case: a non-transaction operators
                  //   happened among committed transactions
                  //
                  //   in this case, a owner transaction ID is different from
                  //   record transaction ID of previous searched item
                  //   and record transaction IDs are different
                  //
                  //   - DELETE case:
                  //
                  //   disk ( rec: T3 )
                  //   RBS  ( owner: T3, rec: T2 ) ->
                  //        ( owner: T22, rec: T1 ) ->
                  //        ( owner: T1, rec: T0 )
                  //
                  //   there is a non-transaction DELETE after T22,
                  //   and then T2 INSERT back with the same RID )
                  //
                  //   - UPDATE case:
                  //
                  //   disk ( rec: T3 )
                  //   RBS  ( owner: T3, rec: invalid ) ->
                  //        ( owner: T22, rec: T1 ) ->
                  //        ( owner: T1, rec: T0 )
                  //
                  //   there is a non-transaction UPDATE after T22,
                  //   so the record transaction ID become invalid,
                  //   so everyone could access this record with invalid
                  //   record transaction ID

                  if ( ownerTransID != lastRecTransID )
                  {
                     if ( lastRecTransID.isInvalid() )
                     {
                        SDB_ASSERT( diskRecordTransID.isInvalid(),
                                    "disk record transaction ID should "
                                    "be invalid also" ) ;
                        // if last record transaction ID is invalid, means
                        // it comes from mem-index tree searching
                        // in this case, we need to reverify if we can access
                        // the first item of RBS chain
                        rc = sdbGetTransCB()->isVersionVisible(
                                                           eduCB,
                                                           ownerTransID,
                                                           transid,
                                                           eduCB->getTransBeginTime(),
                                                           TRANS_ISOLATION_RR,
                                                           FALSE,
                                                           isVisible ) ;
                        PD_RC_CHECK( rc, PDERROR,
                                     "Failed to check version visibility for "
                                     "current transaction [%s] against owner "
                                     "transaction [%s], rc: %d",
                                     dpsTransIDToString( transid ).c_str(),
                                     dpsTransIDToString( ownerTransID ).c_str(), rc ) ;

                        if ( isVisible )
                        {
#if defined (_DEBUG)
                           PD_LOG ( PDDEBUG,
                                    "Hit owner trans version(%s) at position(%d, %lld)",
                                    dpsTransIDToString( ownerTransID ).c_str(),
                                    position._clID,
                                    position._logicalID ) ;
#endif
                           _su->data()->releaseMBContext( context ) ;
                           context = NULL ;
                           break ;
                        }
                     }
                     else if ( recordTransID == lastRecTransID )
                     {
                        // rollback case, move to next record
#if defined (_DEBUG)
                        PD_LOG( PDDEBUG, "Got rollback transaction [%s], "
                                "record transaction [%s], move to next",
                                dpsTransIDToString( ownerTransID ).c_str(),
                                dpsTransIDToString( recordTransID ).c_str() ) ;
#endif
                        // setup next position, release mblatch and continue
                        position._clID =
                          cappedRecord.getField(FIELD_NAME_RBS_PRERECORD_CL).numberInt();
                        position._logicalID =
                          cappedRecord.getField(FIELD_NAME_RBS_PRERECORD_OFFSET).numberLong();
                        // release mblatch before move to next position
                        _su->data()->releaseMBContext( context ) ;
                        context = NULL ;
                        continue ;
                     }
                     else
                     {
                        // non-transaction case, could not access this record,
                        // since it had been changed by non-transaction
                        // operators
#if defined (_DEBUG)
                        PD_LOG( PDDEBUG, "Got non transaction operators, "
                                "transaction ID of previous [%s], "
                                "owner transaction [%s], "
                                "current record transaction [%s], "
                                "could not access",
                                dpsTransIDToString( lastRecTransID ).c_str(),
                                dpsTransIDToString( ownerTransID ).c_str(),
                                dpsTransIDToString( recordTransID ).c_str() ) ;
#endif
                        _su->data()->releaseMBContext( context ) ;
                        context = NULL ;
                        break ;
                     }
                  }
                  lastRecTransID = recordTransID ;

                  // check version visible for current transaction against record
                  // transaction
                  rc = sdbGetTransCB()->isVersionVisible(
                                                  eduCB,
                                                  recordTransID,
                                                  transid,
                                                  eduCB->getTransBeginTime(),
                                                  TRANS_ISOLATION_RR,
                                                  FALSE,
                                                  isVisible ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Failed to check version visibility for "
                               "current transaction [%s] against record "
                               "transaction [%s], rc: %d",
                               dpsTransIDToString( transid ).c_str(),
                               dpsTransIDToString( recordTransID ).c_str(),
                               rc ) ;

                  if ( isVisible )
                  {
                     // it is visible, get data to output
                     BSONElement ele =
                           cappedRecord.getField(FIELD_NAME_RBS_RECORD_DATA) ;
                     found = TRUE ;
                     recordData.setData( ele.value(), ele.valuesize() )  ;
#ifdef _DEBUG
                     PD_LOG ( PDDEBUG,
                              "Found version(%s) at position(%d, %ld)",
                              dpsTransIDToString( recordTransID ).c_str(),
                              position._clID,
                              position._logicalID ) ;
#endif
                     _su->data()->releaseMBContext( context ) ;
                     context = NULL ;
                     break ;
                  }
               }
               // version is not matched, go on to earlier record
            }


            // setup next position, release mblatch and continue
            position._clID =
              cappedRecord.getField(FIELD_NAME_RBS_PRERECORD_CL).numberInt();
            position._logicalID =
              cappedRecord.getField(FIELD_NAME_RBS_PRERECORD_OFFSET).numberLong();

         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Occur exception when getting a RBS record: %s",
                    e.what() ) ;
            goto error ;
         }
         // release mblatch before move to next position
         _su->data()->releaseMBContext( context ) ;
         context = NULL ;

#ifdef _DEBUG
         PD_LOG ( PDDEBUG, "Moving to next position(%d, %ld)",
                  position._clID,
                  position._logicalID ) ;
#endif

      } while ( true );


      // 3. FIXME: update monitor counter for RBS
      // DMS_MON_OP_COUNT_INC( pMonAppCB, MON_RBS_DATA_READ, 1 ) ;

      if ( rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_RBSGETRECORD, rc );
      return  rc ;
   error:
      _su->data()->releaseMBContext( context ) ;
      goto done ;
   }

   // Given start position(_lastFreedCL), try to run RBS garbage collection
   // to recycle space(RBSCL and idxTree nodes).
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__GCRBS, "_dmsRBSSUMgr::_gcRBS" )
   SINT32 _dmsRBSSUMgr::_gcRBS ( UINT16 position, SDB_DPSCB *dpsCB )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR__GCRBS );
      SINT32      rc         = SDB_OK ;
      pmdEDUCB   *eduCB      = pmdGetThreadEDUCB() ;
      CHAR        clName[30] = {0} ;
      DPS_TRANS_ID maxGlobTransID ;
      SINT32      curPos     = position + 1 ;
#ifdef _DEBUG
      SINT32      beginPos   = curPos ;
#endif
      dmsMBContext *pContext = NULL ;

      // finish if transCB or oldVersionCB was not setup. This can happen
      // during start time
      if ( !sdbGetTransCB() || !sdbGetTransCB()->getOldVCB() )
      {
         goto done ;
      }
      // From start position, go through each CL, compare its maxGlobTransID
      // against current lowtran. If the the maxGlobTransID is older, we can
      // recycle the CL by dropping it.
      while ( TRUE )
      {
         // handle the logic to flip to 1
         if ( curPos >= DMS_MAX_RBS_CL )
         {
            curPos = DMS_FIRST_RBS_CL ;
         }

         // Stop if we are about to reach _currentCollection
         _latchS() ;
         if ( curPos == _currentCollection )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG,
                     "Finished all RBS except the currently using one,"
                     " start(%d), end(%d)",
                     beginPos, curPos) ;
#endif
            _releaseS() ;
            break ;
         }
         _releaseS() ;

         // got curPos, can directly use the cached curPos to access the CL
         DMS_BUILD_RBS_CL_NAME( clName, curPos ) ;

         // acquire mbLock before work on this CL, since we will try
         // to drop it, let's take X directly. It's possible that multiple
         // GC can get to here, but there will be only one get in first with
         // mbLock held in X. Others will break out. There is no point to
         // work on next CL either as the one had mbLock will work on them
         // anyway. 
         // Early break out if the CL no long exist or it's held by others.
         if ( SDB_OK != _su->data()->getMBContext( &pContext, clName, -1 ) ||
              SDB_OK != pContext->mbTryLock( EXCLUSIVE ) )
         {
            // early break if someone is still using this cl
            break ;
         }

         // retrieve maxGlobTransID of current CL after we hold the mbLock.
         // NOTE: set with global transaction tag
         maxGlobTransID.setSN( pContext->mbStat()->getMaxGlobTransID() ) ;

#ifdef _DEBUG
         PD_LOG ( PDDEBUG,
                  "Got maxGlobTransID [%s], lowTran [%s], expireTran [%s], "
                  "curPos=%d",
                  dpsTransIDToString( maxGlobTransID ).c_str(),
                  dpsTransIDToString(
                        sdbGetTransCB()->getGlobLowTran() ).c_str(),
                  dpsTransIDToString(
                        sdbGetTransCB()->getGlobExpireTran() ).c_str(),
                  curPos ) ;
#endif
         // Do GC when the cl max transID is expired
         if ( sdbGetTransCB()->isVersionExpired( maxGlobTransID ) )
         {
            rc = _su->data()->dropCollection( clName, eduCB, dpsCB,
                                             TRUE, pContext ) ;
            _su->data()->releaseMBContext( pContext ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to drop RBSCL (%s), rc: %d",
                        clName, rc ) ;
               goto error ;
            }

            // if the lastFree was not changed, update it under protection. 
            // modify position as it's a cached version of lastFreeCollection
            _latchX() ;
            if ( position == _lastFreeCollection )
            {
               _lastFreeCollection = curPos ;
               position = curPos ;
            }
            _releaseX() ;

            PD_LOG ( PDDEBUG, "Successfully recycled %s. ",
                     clName ) ;
         }
         else
         {
            // break out on the first one failed with the condition
            _su->data()->releaseMBContext( pContext ) ;
            break ;
         }
         curPos++ ;
      } // end of while

      _latchX() ;
      if ( _needPrepareRBSCL( TRUE ) )
      {
#ifdef _DEBUG
         PD_LOG( PDDEBUG,
                 "GC prepare CL: "
                 "preparedCl(%d), currentCl(%d), lastFreeCl(%d).",
                 _preparedCollection, _currentCollection, _lastFreeCollection) ;
#endif
         // don't update curCL
         rc = _prepareRBSCL( eduCB, dpsCB, FALSE ) ;
      }
      else
      {
         _releaseX() ;
      }

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "RBS GC on index trees." ) ;
#endif

   done:

      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__GCRBS, rc );
      return  rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsRBSSUMgr::_rbsCLExpired( UINT16 cl )
   {
      BOOLEAN rv = TRUE; 
      //   lastFreeCL           currentCL
      // 0-----|=====================|-----4096
      // OR
      //   currentCL            lastFreeCL
      // 0=====|---------------------|=====4096
      // --- expired (rv=TRUE)
      // === inUse   (rv=FALSE)
      if ( _lastFreeCollection < _currentCollection )
      {
         if ( ( cl > _lastFreeCollection ) && 
              ( cl <= _currentCollection ) )
         {
            rv = FALSE; 
         }
      }
      else
      {
         if ( ( cl > _lastFreeCollection ) ||
              ( cl <= _currentCollection ) )
         {
            rv = FALSE; 
         }
      }
      return rv ;
   }

   BOOLEAN _dmsRBSSUMgr::_rbsPositionExpired( dmsRBSOffset & pos ) 
   {
      return _rbsCLExpired( pos._clID ) ;
   }

   // This is the main interface to run garbage collection on RBS with best
   // effort. All protection is self contained.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_GCRBS, "_dmsRBSSUMgr::gcRBS" )
   void _dmsRBSSUMgr::gcRBS ( )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_GCRBS );
      SINT32      rc         = SDB_OK ;
      // we explicitly set dpsCB to NULL as we decided to fail the transaction
      // after failover to new primary node. If we decide to life this
      // restriction, we will setup the proper dpsCB
      //SDB_DPSCB  *dpsCB      = pmdGetKRCB()->getDPSCB() ;
      SDB_DPSCB    *dpsCB    = NULL ;

      // make sure that the RBSCS(su) is not changed while we are doing gc.
      DMSSYSSUMGR_SLOCK() ;

      // try the best to gc as much as possible
      rc = _gcRBS( _lastFreeCollection, dpsCB ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to run GC, meta=(%d, %d) rc: %d",
                  _currentCollection, _lastFreeCollection, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXIT ( SDB__DMSRBSSUMGR_GCRBS );
      // this is best effort
      return  ;
   error:
      goto done ;
   }

}
