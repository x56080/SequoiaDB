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

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace bson ;
namespace fs = boost::filesystem ;

namespace engine
{

   _dmsRBSSUMgr::_dmsRBSSUMgr ( SDB_DMSCB *dmsCB )
      : _dmsSysSUMgr( dmsCB ), _numActiveGC( 0 )
   {
      // By default, start with second collection as the first one stores meta
      _currentCollection  = DMS_FIRST_RBS_CL ;
      _lastFreeCollection = DMS_MAX_RBS_CL ;
      // use default size for now, we may want to add config parm later on
      _maxCollectionSize  = DMS_DFT_RBSCL_SIZE ;
   }

   // Initialization of RBS during node start up
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_INIT, "_dmsRBSSUMgr::init" )
   SINT32 _dmsRBSSUMgr::init ()
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

      // exclusive lock temp cb. this function should be called during process
      // initialization, so it shouldn't be called in parallel by agents
      DMSSYSSUMGR_XLOCK() ;

      // first to load collection space
      rc = rtnLoadCollectionSpace( SDB_DMSRBS_NAME,
                                   pmdGetOptionCB()->getDbPath(),
                                   pmdGetOptionCB()->getIndexPath(),
                                   pmdGetOptionCB()->getLobPath(),
                                   pmdGetOptionCB()->getLobMetaPath(),
                                   NULL, _dmsCB, FALSE ) ;
      // FIXME: remove
      PD_LOG ( PDDEBUG, "load RBS cs %s with rc:%d", SDB_DMSRBS_NAME, rc ) ;

      if ( SDB_OK == rc )
      {
         // Drop existing RBSCS
         rc = rtnDelCollectionSpaceCommand( SDB_DMSRBS_NAME, NULL, _dmsCB,
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
         PD_LOG ( PDDEBUG, "Creating RBS cs %s.", SDB_DMSRBS_NAME ) ;

         rc = rtnCreateCollectionSpaceCommand( SDB_DMSRBS_NAME, NULL, _dmsCB,
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

         rc = rtnCollectionSpaceLock ( SDB_DMSRBS_NAME, _dmsCB, TRUE,
                                       &_su, suID ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get collection space and lock for %s, "
                     "rc: %d", SDB_DMSTEMP_NAME, rc ) ;
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

         PD_LOG ( PDDEBUG, "Created RBS collection %s(%d) successfully.",
                  clName, logicalID );
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
   // On normal return, the context for current RBSCL will be latched in X
   // and returned.
   // On error, nothing is held.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR__PREPARERBSCLFORRECORD, "_dmsRBSSUMgr::_prepareRBSCLForRecord" )
   SINT32 _dmsRBSSUMgr::_prepareRBSCLForRecord( UINT32          recordSize,
                                                pmdEDUCB     *  eduCB,
                                                SDB_DPSCB    *  dpsCB,
                                                dmsMBContext *& clContext )
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
      DMS_BUILD_RBS_CL_NAME( clName, _currentCollection ) ;
      _releaseS() ;

      rc = _su->data()->getMBContext( &clContext, clName, EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get curCL(%s) mbLock, rc: %d",
                  clName, rc ) ;
         goto error ;
      }
      mbLocked = TRUE ;

      // Has to get clID from context as the _curCL could change outside
      // of latch protection
      tempCurCL = _currentCollection ;
      //if( !sd->clDataSpaceEnough( clContext, recordSize ) )
      if( !sd->spaceEnough( clContext, recordSize ) )
      {
         // current CL does NOT have enough space, create the new CL
         BSONObjBuilder builder ;
         BSONObj        extOptions ;
         UINT32         logicalID    = DMS_INVALID_CLID ;
         UINT16         collectionID = DMS_INVALID_MBID ;

#ifdef _DEBUG
         {
            const dmsMBStatInfo *mbStatInfo =
                                   sd->getMBStatInfo( clContext->mbID() ) ;
            PD_LOG ( PDINFO,
                  "Out of space in %s, allocating next RBSCL. recordsize(%d),"
                  "clfreespace(%d), clTotalPages(%d), cltotalrecord(%d),"
                  "cl max(%lld), squareroot(%d)",
                  clName, recordSize,
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
         // It's possible that another thread has already moved up the
         // _curCollection, we should just go back and retry
         if ( _currentCollection != tempCurCL )
         {
            PD_LOG ( PDDEBUG, 
                     "CurrentCollection(%d) changed from %d, retry.",
                     _currentCollection, tempCurCL, rc ) ;
            _releaseX() ;
            goto begin ;
         }

         tempCurCL++ ;
         // if reached max, wrap to first one.
         if ( tempCurCL >= DMS_MAX_RBS_CL )
         {
            tempCurCL = DMS_FIRST_RBS_CL ;
         }

         // create next CL
         try
         {
            builder.append( FIELD_NAME_SIZE, _maxCollectionSize ) ;
            builder.append( FIELD_NAME_MAX, 0 ) ;
            builder.appendBool( FIELD_NAME_OVERWRITE, FALSE ) ;
            extOptions = builder.done() ;

            // add the collection for RBS
            DMS_BUILD_RBS_CL_NAME( clName, tempCurCL ) ;

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
               _releaseX() ;
               goto error ;
            }
            PD_LOG ( PDDEBUG, "Successfully created RBS collection %s, logicalID= %d",
                     clName, logicalID ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Occur exception when adding RBSCL : %s",
                    e.what() ) ;
            _releaseX() ;
            goto error ;
         }

         // update curCL under the latch, but after everything succeeded
         _currentCollection = tempCurCL ;

         _releaseX() ;

         dmsStartAsyncRBSGC() ;

         // get curCL context and take mbLock here
         rc = _su->data()->getMBContext( &clContext, clName, EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get curCL mbLock, rc: %d", rc ) ;
            goto error ;
         }
         mbLocked = TRUE ;
      } // end of spaceEnough

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

      // mblatch will be held after this call
      rc = _prepareRBSCLForRecord( size, eduCB, dpsCB, clContext ) ;
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
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_APPENDRECORD1, "_dmsRBSSUMgr::appendRecord" )
   SINT32 _dmsRBSSUMgr::appendRecord ( dmsStorageUnitID      csid,
                                       UINT16                clid,
                                       //DPS_LSN_OFFSET        lsn,
                                       UINT32                clLID,
                                       const dmsRecordID    &rid,
                                       DPS_TRANS_ID         &recordTransID,
                                       DPS_TRANS_ID         &ownerTransID,
                                       const BSONObj        &data )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_APPENDRECORD1 );
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
      UINT32        bkt          = _hash( csid, clid, rid );
      BSONObjBuilder builder ;
      utilInsertResult insertResult ;
      BSONObj       record ;
      BOOLEAN       bktLatched   = FALSE ;
      BOOLEAN       clLocked     = FALSE ;
      UINT32        recSize ;
      // type conversion for following use
      SINT32        cl           = clid ;
      _dmsStorageDataCapped *sd = (_dmsStorageDataCapped*)_su->data();

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
         // move to proper RBS CL which has enough space, on OK return,
         // the cl is locked in X
         rc = _prepareRBSCLForRecord( recSize, eduCB, dpsCB, clContext ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to prepare RBSCL for record, rc: %d",
                     rc ) ;
            goto error ;
         }
         clLocked = TRUE ;


         // insert the record to RBS
         rc = _su->insertRecord ( clName, record, eduCB, dpsCB,
                                  TRUE, TRUE, clContext, -1, &insertResult ) ;
         if ( rc )
         {
            // there could be timing hole that the cl we want to use was
            if ( SDB_OSS_UP_TO_LIMIT == rc )
            {
               PD_LOG ( PDDEBUG,
                        "Failed to insert into RBS(%d), going to retry. rc: %d",
                        clContext->mbID(), rc ) ;
               _su->data()->releaseMBContext( clContext ) ;
               clLocked = FALSE ;
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
      clLocked = FALSE ;

      // Update bucket to point to the new record location
      {
         dmsRBSOffset location ;
         SINT32       ext, offset ;
         insertResult.getInsertLoc( ext, offset ) ;
         // FIXME:  is this ext correct? should it be extlid from extent
         sd->_extLidAndOffset2RecLid( ext, offset, location._logicalID ) ;
         location._clID = _currentCollection ;
         _rbsRecordBkt.setOffset( location, bkt ) ;
      }
      // unlock the bucket
      _rbsRecordBkt.release( bkt ) ;
      bktLatched  = FALSE ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_APPENDRECORD1, rc );
      return  rc ;
   error:
      if ( bktLatched )
      {
         _rbsRecordBkt.release( bkt ) ;
      }
      if ( clLocked )
      {
         _su->data()->releaseMBContext( clContext ) ;
      }
      goto done ;
   }

   // Given a transactionID and beginning of a record chain, find a visiable record
   // This method uses fetch method from cappedCL
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_GETRECORD, "_dmsRBSSUMgr::getRecord" )
   SINT32 _dmsRBSSUMgr::getRecord ( dmsStorageUnitID  csid,
                                    UINT16            clid,
                                    //DPS_LSN_OFFSET    &lsn,
                                    UINT32            clLID,
                                    dmsRecordID      &rid,
                                    DPS_TRANS_ID     &transid,
                                    BOOLEAN          &found,
                                    dmsRecordData    &recordData )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_GETRECORD );
      SINT32        rc         = SDB_OK ;
      UINT32        bkt        = _hash( csid, clid, rid );
      dmsMBContext *context    = NULL ;
      pmdEDUCB     *eduCB      = pmdGetThreadEDUCB() ;
      dmsRecordRW   recordRW ;
      dmsRBSOffset  position ;
      CHAR          clName[30] = {0} ;
      dmsStorageDataCapped *sd = (dmsStorageDataCapped*)_su->data();
      dmsExtentID   extID      = DMS_INVALID_EXTENT ;
      dmsOffset     offset     = DMS_INVALID_OFFSET;

#ifdef _DEBUG
      PD_LOG ( PDDEBUG,
               "Transaction (%s) tries to find a proper version from RBS, "
               "csid(%d), clid(%d), record rid(%d, %d), cllid(%d)",
               dpsTransIDToString( transid ).c_str(),
               csid, clid, rid._extent, rid._offset, clLID ) ;
#endif
      found = FALSE ;
      // 1. From the hash table, find the position
      // lock the bucket
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

      do
      {
         // finish if the hasbucket entry is invalid
         // FIXME: is it possible that the position is pointing to a
         // version no longer exist (CL has been recycled)
         if ( !position.isValid() )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, "no more older version found" ) ;
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
            //DPS_LSN_OFFSET recordLSNOffset ;
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
            if ( SDB_DMS_NOTEXIST == rc )
            {
               // RBS collection is removed by GC, there no earlier records
               // to be found
               // which means the record is created after current transaction
               // and current transaction should not see the record
#ifdef _DEBUG
               PD_LOG ( PDDEBUG, "Collection %s is deleted, "
                        "no more older version found", clName ) ;
#endif
               goto done ;
            }
            else if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to get mbLatch for %s, rc=%d",
                        clName, rc ) ;
               goto error ;
            }

            //rc = sd->fetch( context, recordID, cappedRecordData, eduCB ) ;
            rc = sd->fetch( context, recordID, cappedRecord, eduCB, FALSE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, 
                        "Failed to fetch rbsrecord,rid(%d, %d), rc=%d",
                        extID, offset, rc ) ;
               goto error ;
            }

            // 3. parse the dataRecord to figure out record key and visiability
            //cappedRecord = BSONObj( cappedRecordData.data() ) ;
            eleTransID = cappedRecord.getField(FIELD_NAME_RBS_RECORD_TRANSID) ;
/*
            eleLsnOffset = 
                     cappedRecord.getField(FIELD_NAME_RBS_RECORD_LSN_OFFSET) ;
            recordLSNOffset = eleLsnOffset.numberLong();
*/
            eleCLLID = 
                     cappedRecord.getField(FIELD_NAME_RBS_RECORD_CLLID) ;

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
                 //( recordLSNOffset == lsn ) &&
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
                 else if ( sdbGetTransCB()->isVersionVisible(
                                               recordTransID,
                                               transid,
                                               eduCB->getTransBeginTime() ) )
                 {
                    // check version, and it is visible
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
      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR_GETRECORD, rc );
      return  rc ;
   error:
      _su->data()->releaseMBContext( context ) ;
      goto done ;
   }

   // Given start position, try to run RBS garbage collection to recycle space
   // once finished, the new position is returned.
   // Note that the caller should hold mbLock of SYSRBS000
   // position could be stale.
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
      BOOLEAN     latched    = FALSE ;

      // From start position, go through each CL, compare its maxGlobTransID
      // against current lowtran. If the the maxGlobTransID is older, we can
      // recycle the CL by dropping it.
      while ( TRUE )
      {
         _latchX() ;
         latched = TRUE ;

         // handle the logic to flip to 1
         if ( curPos >= DMS_MAX_RBS_CL )
         {
            curPos = DMS_FIRST_RBS_CL ;
         }

         if ( curPos == _currentCollection )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG,
                     "Finished all RBS except the currently using one,"
                     " start(%d), end(%d)",
                     beginPos, curPos) ;
#endif
            break ;
         }

         DMS_BUILD_RBS_CL_NAME( clName, curPos ) ;

         // retrieve system lowtran

         // acquire mbLock before work on this CL, since we will try
         // to drop it, let's take X directly
         if ( SDB_OK != _su->data()->getMBContext( &pContext, clName, -1 ) ||
              SDB_OK != pContext->mbTryLock( EXCLUSIVE ) )
         {
            // early break if someone is still using this cl
            break ;
         }

         // retrieve maxGlobTransID of current CL
         // NOTE: set with global transaction tag
         maxGlobTransID.setSN( pContext->mbStat()->getMaxGlobTransID() ) ;

#ifdef _DEBUG
         PD_LOG ( PDDEBUG,
                  "Got maxGlobTransID and lowTran (%s, %s), curPos=%d",
                   dpsTransIDToString( maxGlobTransID ).c_str(),
                   dpsTransIDToString(
                         sdbGetTransCB()->getGlobLowTran() ).c_str(),
                   curPos ) ;
#endif
         // Do GC when the cl max transID is older than lowtran
         // TODO: we may want to do GC when lowTran is invalid, meaning no 
         // running transaction
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

            // assignement as atomic operation
            _lastFreeCollection = curPos ;

            PD_LOG ( PDDEBUG, "Successfully recycled %s. ",
                     clName ) ;
         }
         else
         {
            // break out on the first one failed with the condition
            _su->data()->releaseMBContext( pContext ) ;
            break ;
         }
         _releaseX() ;
         latched = FALSE ;
         curPos++ ;
      } // end of while

      if ( latched )
      {
         _releaseX() ;
         latched = FALSE ;
      }

#ifdef _DEBUG
      PD_LOG( PDDEBUG, "RBS GC on index trees." ) ;
#endif

      // clean up in memory index tree nodes
      sdbGetTransCB()->getOldVCB()->gcIdxTrees( ) ;

   done:
      if ( latched )
      {
         _releaseX() ;
      }

      PD_TRACE_EXITRC ( SDB__DMSRBSSUMGR__GCRBS, rc );
      return  rc ;
   error:
      goto done ;
   }

   // This is the main interface to run garbage collection on RBS with best
   // effort. All protection is self contained.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSSUMGR_GCRBS, "_dmsRBSSUMgr::gcRBS" )
   void _dmsRBSSUMgr::gcRBS ( )
   {
      PD_TRACE_ENTRY ( SDB__DMSRBSSUMGR_GCRBS );
      SINT32      rc         = SDB_OK ;
      SDB_DPSCB  *dpsCB      = pmdGetKRCB()->getDPSCB() ;

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

   _dmsRBSGCJob::_dmsRBSGCJob( _dmsRBSSUMgr  *rbsSUMgr )
   {
      _rbsSUMgr = rbsSUMgr ;
   }

   _dmsRBSGCJob::~_dmsRBSGCJob()
   {
      _rbsSUMgr->decActiveGC() ;
   }

   const CHAR* _dmsRBSGCJob::name() const
   {
      return "RBS GC" ;
   }

   INT32 _dmsRBSGCJob::doit( IExecutor *pExe,
                             UTIL_LJOB_DO_RESULT &result,
                             UINT64 &sleepTime )
   {
      _rbsSUMgr->incActiveGC() ;
      _rbsSUMgr->gcRBS() ;
      result = UTIL_LJOB_DO_FINISH ;
      return SDB_OK ;
   }

   // submit async job to do garbage collection on RBS including data and idx
   void  dmsStartAsyncRBSGC()
   {
      if ( pmdGetOptionCB()->mvccOn() )
      {
         dmsRBSGCJob * pJob = NULL ;
#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Creating dmsRBSGCJob " ) ;
#endif
         pJob = SDB_OSS_NEW dmsRBSGCJob( pmdGetKRCB()->
                                         getDMSCB()->getRBSSUMgr() ) ;

         if ( !pJob )
         {
            PD_LOG( PDWARNING, "Alloc dmsRBSGCJob failed" ) ;
         }
         else
         {
            INT32 rc = pJob->submit( TRUE ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Submit dmsRBSGCJob failed,rc:%d",
                       rc ) ;
            }
         }
      }
   }
}
