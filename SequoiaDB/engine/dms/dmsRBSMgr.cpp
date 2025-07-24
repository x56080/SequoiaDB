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

   Source File Name = dmsRBSMgr.cpp

   Descriptive Name = DMS Rollback Segment Management

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   rollback segment creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/24/2019  CYX Initial Draft
          01/06/2020  HGM Copy from dmsRBSSUMgr.hpp

   Last Changed =

*******************************************************************************/
#include "dmsStorageUnit.hpp"
#include "../bson/bson.h"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "dmsRBSMgr.hpp"
#include "dpsUtil.hpp"
#include "ossMem.hpp"
#include "dmsRBSGCJob.hpp"
#include "dpsTransCB.hpp"
#include "dpsTransVersionCtrl.hpp"

using namespace bson ;

namespace engine
{

   /*
      _dmsRBSMgr implement
    */
   _dmsRBSMgr::_dmsRBSMgr()
   : _numActiveGC( 0 ),
     _rbsNum( 0 ),
     _rbsModulo( 0 ),
     _rbsPowerIndex( 0 ),
     _rbsSUMgrs( NULL )
   {
   }

   // Initialization of RBS during node start up
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSMGR_INIT, "_dmsRBSMgr::init" )
   INT32 _dmsRBSMgr::init( UINT32 rbsNum )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSRBSMGR_INIT ) ;

      if ( 0 == _rbsNum )
      {
         // initialize RBS storage unit managers
         rc = _initRBSSUMgrs( rbsNum ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize RBS storage unit"
                      "managers with number [%u], rc: %d", rbsNum, rc ) ;
      }

      SDB_ASSERT( rbsNum == _rbsNum && 0 != _rbsNum,
                  "number of RBS is not matched" ) ;
      SDB_ASSERT( NULL != _rbsSUMgrs,
                  "RBS storage unit managers is invalid" ) ;

      // initialize each RBS storage unit
      for ( UINT32 i = 0 ; i < _rbsNum ; ++ i )
      {
         rc = _rbsSUMgrs[ i ].init( this, i ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize storage unit "
                      "manager [%u], rc: %d", i, rc ) ;
      }

      // trigger GC background job, we will use the light weight background
      // to run gc every minute
      rc = dmsStartAsyncRBSGC() ;
      if ( rc )
      {
         // log error message and reset to OK
         PD_LOG ( PDWARNING, "Failed to trigger GC during start, rc=%d ",
                  rc ) ;
         rc = SDB_OK ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSRBSMGR_INIT, rc );
      return rc ;

   error:
      goto done ;
   }

   // initialize number of RBS
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSMGR__INITRBSSUMGRS, "_dmsRBSMgr::_initRBSSUMgrs" )
   INT32 _dmsRBSMgr::_initRBSSUMgrs( UINT32 rbsNum )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSRBSMGR__INITRBSSUMGRS ) ;

      SDB_ASSERT( ossIsPowerOf2( rbsNum ),
                  "number of RBS should be power of 2" ) ;

      PD_CHECK( ossIsPowerOf2( rbsNum, &_rbsPowerIndex ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to initialize number of RBS with [%u], "
                "it is not power of 2", rbsNum ) ;

      // allocate RBS storage units
      _rbsSUMgrs = SDB_OSS_NEW dmsRBSSUMgr[ rbsNum ] ;
      PD_CHECK( NULL != _rbsSUMgrs, SDB_OOM, error, PDERROR,
                "Failed to allocate storage unit managers with number [%u]",
                rbsNum ) ;

      // once storage unit managers are build, initialize number of RBS
      _rbsNum = rbsNum ;
      _rbsModulo = rbsNum - 1 ;

   done:
      PD_TRACE_EXITRC( SDB__DMSRBSMGR__INITRBSSUMGRS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _dmsRBSMgr::fini()
   {
      INT32 rc = SDB_OK ;

      if ( NULL != _rbsSUMgrs )
      {
         for ( UINT32 i = 0 ; i < _rbsNum ; ++ i )
         {
            _rbsSUMgrs[ i ].fini() ;
         }
         SDB_OSS_DEL [] _rbsSUMgrs ;
         _rbsSUMgrs = NULL ;
         _rbsNum = 0 ;
      }

      return rc ;
   }

   // append a record to RBS
   // Input Parm:
   //    csid, clid, rid: key to identify a record.
   //    clLID: decide the life of a CL. It is changed after drop/truncate
   //    recordTransID:  Record version, (last creation/update trans ID)
   //    ownerTransID: transaction to put the record to in memory old version
   //                  container and now to RBS
   //    data:  the data object of this version
   //    callback: transaction lock callback
   // Output Parm:
   //    rc: return code, SDB_OK or error code
   // NOTE: will calculate a hash value with CSID, CLID and RID to locate a
   //       RBS storage unit for append
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSMGR_RBSAPPENDRECORD, "_dmsRBSMgr::rbsAppendRecord" )
   INT32 _dmsRBSMgr::rbsAppendRecord( dmsStorageUnitID      csid,
                                      UINT16                clid,
                                      UINT32                clLID,
                                      const dmsRecordID    &rid,
                                      DPS_TRANS_ID         &recordTransID,
                                      DPS_TRANS_ID         &ownerTransID,
                                      const BSONObj        &data,
                                      dmsTransLockCallback *callback )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSRBSMGR_RBSAPPENDRECORD ) ;

      // locate storage unit and bucket by hash CSID, CLID and RID
      UINT32 hashValue = _hash( csid, clid, rid ) ;
      UINT32 suIndex = hashValue & _rbsModulo ;
      UINT32 bucketID = hashValue >> _rbsPowerIndex ;

      rc = _rbsSUMgrs[ suIndex ].rbsAppendRecord( csid,
                                                  clid,
                                                  clLID,
                                                  rid,
                                                  bucketID,
                                                  recordTransID,
                                                  ownerTransID,
                                                  data,
                                                  callback ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to append record to "
                   "RBS storage unit [%u], rc: %d", suIndex, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSRBSMGR_RBSAPPENDRECORD, rc ) ;
      return  rc ;

   error:
      goto done ;
   }

   // Given a transactionID and beginning of a record chain, find a visible
   // record
   // NOTE: will calculate a hash value with CSID, CLID and RID to locate a
   //       RBS storage unit for append
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSMGR_RBSGETRECORD, "_dmsRBSMgr::rbsGetRecord" )
   INT32 _dmsRBSMgr::rbsGetRecord( dmsStorageUnitID  csid,
                                   UINT16            clid,
                                   UINT32            clLID,
                                   dmsRecordID      &rid,
                                   DPS_TRANS_ID     &transid,
                                   BOOLEAN          &found,
                                   dmsRecordData    &recordData,
                                   dmsRBSOffset     &startPos,
                                   dmsRBSOffset     &endPos,
                                   const DPS_TRANS_ID &diskRecordTransID )
   {
      SINT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSRBSMGR_RBSGETRECORD ) ;

      // locate storage unit and bucket by hash CSID, CLID and RID
      UINT32 hashValue = _hash( csid, clid, rid ) ;
      UINT32 suIndex = hashValue & _rbsModulo ;
      UINT32 bucketID = hashValue >> _rbsPowerIndex ;

      rc = _rbsSUMgrs[ suIndex ].rbsGetRecord( csid,
                                               clid,
                                               clLID,
                                               rid,
                                               bucketID,
                                               transid,
                                               found,
                                               recordData,
                                               startPos,
                                               endPos,
                                               diskRecordTransID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record from "
                   "RBS storage unit [%u], rc: %d", suIndex, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSRBSMGR_RBSGETRECORD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // This is the main interface to run garbage collection on RBS with best
   // effort. All protection is self contained.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSMGR_GCRBS, "_dmsRBSMgr::gcRBS" )
   void _dmsRBSMgr::gcRBS()
   {
      PD_TRACE_ENTRY( SDB__DMSRBSMGR_GCRBS ) ;

      // loop for all RBS storage units
      for ( UINT32 i = 0 ; i < _rbsNum ; ++ i )
      {
         _rbsSUMgrs[ i ].gcRBS() ;
      }

      // clean up in memory index tree nodes
      sdbGetTransCB()->getOldVCB()->gcIdxTrees( ) ;

      PD_TRACE_EXIT( SDB__DMSRBSMGR_GCRBS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSMGR_GETNUMSYNCADDCL, "_dmsRBSMgr::getNumSyncAddCL" )
   UINT32 _dmsRBSMgr::getNumSyncAddCL()
   {
      UINT32 numSyncAddCL = 0 ;

      PD_TRACE_ENTRY( SDB__DMSRBSMGR_GETNUMSYNCADDCL ) ;

      for ( UINT32 i = 0 ; i < _rbsNum ; ++ i )
      {
         numSyncAddCL += _rbsSUMgrs[ i ].getNumFreeCL() ;
      }

      PD_TRACE_EXIT( SDB__DMSRBSMGR_GETNUMSYNCADDCL ) ;

      return numSyncAddCL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSRBSMGR_GETNUMFREECL, "_dmsRBSMgr::getNumFreeCL" )
   UINT32 _dmsRBSMgr::getNumFreeCL()
   {
      UINT32 numFreeCL = 0 ;

      PD_TRACE_ENTRY( SDB__DMSRBSMGR_GETNUMFREECL ) ;

      for ( UINT32 i = 0 ; i < _rbsNum ; ++ i )
      {
         numFreeCL += _rbsSUMgrs[ i ].getNumFreeCL() ;
      }

      PD_TRACE_EXIT( SDB__DMSRBSMGR_GETNUMFREECL ) ;

      return numFreeCL ;
   }

}
