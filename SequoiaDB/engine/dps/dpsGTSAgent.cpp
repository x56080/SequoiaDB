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

   Source File Name = dpsGTSAgent.cpp

   Descriptive Name = Runtime Global Transaction Agent

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains background job to update
   global lowTran.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsGTSAgent.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "dpsTrace.hpp"
#include "pmdOptions.hpp"
#include "rtnCB.hpp"
#include "dpsUtil.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   // update lowTran for each 10 seconds
   // NOTE: so global lowTran from different nodes should be updated in at
   //       lease 2 rounds
   #define DPS_LOWTRAN_WAIT_SEC ( 10 )

   /*
      _dpsGTSAgent implement
    */
   _dpsGTSAgent::_dpsGTSAgent()
   : _transCB( sdbGetTransCB() ),
     _maxNodeTimeError( DPS_DEF_GLOBTRANS_MAXTIMEERROR )
   {
      SDB_ASSERT( NULL != _transCB, "transCB is invalid" ) ;
      _localRID.value = MSG_INVALID_ROUTEID ;
   }

   _dpsGTSAgent::~_dpsGTSAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSAGENT__GETLOCALLOWTRAN, "_dpsGTSAgent::_getLocalLowTran" )
   INT32 _dpsGTSAgent::_getLocalLowTran( DPS_TRANSID_SN &localLowTran,
                                         DPS_TRANSID_SN &localExpireTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSGTSAGENT__GETLOCALLOWTRAN ) ;

      DPS_TRANSID_SN lowTran = DPS_MAX_TRANSID_SN ;
      DPS_TRANSID_SN expireTran = DPS_MAX_TRANSID_SN ;

      // only get lowTran when global transaction is enabled
      if ( _transCB->isTransOn() && _transCB->isGlobTransOn() )
      {
         DPS_TRANS_ID lowTranID ;
         DPS_TRANS_ID expireTranID ;

         lowTranID = _transCB->getLocalLowTran() ;
         if ( lowTranID.isValid() )
         {
            lowTran = lowTranID.getGlobSN() ;
         }

         if ( SDB_ROLE_DATA == pmdGetDBRole() )
         {
            expireTranID = _transCB->getLocalExpireTran() ;
            if ( expireTranID.isValid() )
            {
               expireTran = expireTranID.getGlobSN() ;
            }
         }
      }

      // copy to output
      // if no global transaction in local, we report max value of
      // transaction SN to caller
      localLowTran = lowTran ;
      localExpireTran = expireTran ;

      PD_LOG( PDDEBUG, "Got local lowTran [%s], "
              "local expireTran [%s]", dpsTransSNToString( lowTran ).c_str(),
              dpsTransSNToString( expireTran ).c_str() ) ;

      PD_TRACE_EXITRC( SDB__DPSGTSAGENT__GETLOCALLOWTRAN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSAGENT__SETNODELOWTRANBSON, "_dpsGTSAgent::_setGlobLowTran" )
   INT32 _dpsGTSAgent::_setGlobLowTran( const DPS_TRANSID_SN &globLowTran,
                                        const DPS_TRANSID_SN &globExpireTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSGTSAGENT__SETNODELOWTRANBSON ) ;

      if ( _transCB->isTransOn() && _transCB->isGlobTransOn() )
      {
         _transCB->setGlobLowTran( globLowTran ) ;
         _transCB->setGlobExpireTran( globExpireTran ) ;
      }

      PD_TRACE_EXITRC( SDB__DPSGTSAGENT__SETNODELOWTRANBSON, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSAGENT__FILLLOWTRANREQ, "_dpsGTSAgent::_fillLowTranReq" )
   INT32 _dpsGTSAgent::_fillLowTranReq( MsgGTSLowTranReq *request,
                                        const DPS_TRANSID_SN &localLowTran,
                                        const DPS_TRANSID_SN &localExpireTran,
                                        BSONObj &requestObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSGTSAGENT__FILLLOWTRANREQ ) ;

      stpAgent agent ;

      BOOLEAN transOn = pmdGetOptionCB()->transactionOn() ;
      BOOLEAN globTransOn = pmdGetOptionCB()->globTransOn() ;
      BOOLEAN mvccOn = pmdGetOptionCB()->mvccOn() ;
      BOOLEAN stpAvailable = agent.isAvailable() ;

      // build request object
      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_TRANS_LOWTRAN, (INT64)localLowTran ) ;
         builder.append( FIELD_NAME_TRANS_EXPTRAN, (INT64)localExpireTran ) ;
         builder.appendBool( PMD_OPTION_TRANSACTIONON, transOn ) ;
         builder.appendBool( PMD_OPTION_GLOBTRANSON, globTransOn ) ;
         builder.appendBool( PMD_OPTION_MVCCON, mvccOn ) ;
         builder.appendBool( FIELD_NAME_STP_AVAILABLE, stpAvailable ) ;
         requestObject = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build request object, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // fill message
      request->messageLength = sizeof( MsgGTSLowTranReq ) +
                               requestObject.objsize() ;
      request->opCode = MSG_GTS_LOWTRAN_REQ ;
      request->TID = 0 ;
      // set route ID of this node
      request->routeID.value = _localRID.value ;
      request->requestID = 0LL ;

   done:
      PD_TRACE_EXITRC( SDB__DPSGTSAGENT__FILLLOWTRANREQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSAGENT__PARSELOWTRANRSP, "_dpsGTSAgent::_parseLowTranRsp" )
   INT32 _dpsGTSAgent::_parseLowTranRsp( MsgGTSLowTranRsp *response,
                                         DPS_TRANSID_SN &globLowTran,
                                         DPS_TRANSID_SN &globExpireTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSGTSAGENT__PARSELOWTRANRSP ) ;

      BSONObj responseObject ;

      // check message length
      PD_CHECK( response->header.messageLength >
                        (INT32)( sizeof( MsgGTSLowTranRsp ) ) +
                        responseObject.objsize(),
                SDB_SYS, error, PDERROR,
                "Failed to extract lowTran response, "
                "message length [%d] is unexpected",
                response->header.messageLength ) ;

      try
      {
         BSONElement element ;

         responseObject = BSONObj( (CHAR *)response +
                                   sizeof( MsgGTSLowTranRsp ) ) ;

         // parse global lowTran
         element = responseObject.getField( FIELD_NAME_TRANS_GLOBLOWTRAN ) ;
         PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it should not be empty",
                   FIELD_NAME_TRANS_GLOBLOWTRAN ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it should be long type",
                   FIELD_NAME_TRANS_GLOBLOWTRAN ) ;
         globLowTran = (DPS_TRANSID_SN)( element.numberLong() ) ;

         element = responseObject.getField( FIELD_NAME_TRANS_GLOBEXPTRAN ) ;
         PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it should not be empty",
                   FIELD_NAME_TRANS_GLOBEXPTRAN ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it should be long type",
                   FIELD_NAME_TRANS_GLOBEXPTRAN ) ;
         globExpireTran = (DPS_TRANSID_SN)( element.numberLong() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build response object, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSGTSAGENT__PARSELOWTRANRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSAGENT__FILLGTSARBITREQ, "_dpsGTSAgent::_fillGTSArbitReq" )
   INT32 _dpsGTSAgent::_fillGTSArbitReq( MsgClsGTSArbitReq *request,
                                         const DPS_TRANS_ID &readTransID,
                                         const DPS_TRANS_ID &writeTransID,
                                         DPS_TRANS_STATUS writeTransStatus )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSGTSAGENT__FILLGTSARBITREQ ) ;

      SDB_ASSERT( NULL != request, "request is invalid" ) ;

      // fill message ( header is filled by constructor of message )
      request->readTransNodeID = (UINT16)( readTransID.getNodeID() ) ;
      request->readTransID = (UINT64)( readTransID.getGlobSN() ) ;
      request->writeTransNodeID = (UINT16)( writeTransID.getNodeID() ) ;
      request->writeTransID = (UINT64)( writeTransID.getGlobSN() ) ;
      request->writeTransStatus = (UINT16)writeTransStatus ;

      PD_TRACE_EXITRC( SDB__DPSGTSAGENT__FILLGTSARBITREQ, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSAGENT__PARSEGTSARBITRSP, "_dpsGTSAgent::_parseGTSArbitRsp" )
   INT32 _dpsGTSAgent::_parseGTSArbitRsp( const MsgClsGTSArbitRsp *response,
                                          BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSGTSAGENT__PARSEGTSARBITRSP ) ;

      SDB_ASSERT( NULL != response, "response is invalid" ) ;

      visible = ( 0 != response->visible ) ? TRUE : FALSE ;

      PD_TRACE_EXITRC( SDB__DPSGTSAGENT__PARSEGTSARBITRSP, rc ) ;

      return rc ;
   }

   /*
       _dpsGTSLowTranJob implement
    */
   _dpsGTSLowTranJob::_dpsGTSLowTranJob( dpsGTSAgent *gtsAgent )
   : _gtsAgent( gtsAgent )
   {
      SDB_ASSERT( NULL != gtsAgent, "GTS agent is invalid" ) ;
   }

   _dpsGTSLowTranJob::~_dpsGTSLowTranJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSLOWTRANJOB_DOIT, "_dpsGTSLowTranJob::doit" )
   INT32 _dpsGTSLowTranJob::doit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSGTSLOWTRANJOB_DOIT ) ;

      pmdEDUCB *cb = eduCB() ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      UINT32 timeout = 0 ;
      ossEvent *updateEvent = sdbGetTransCB()->getUpdateLowTranEvent() ;
      ossEvent *waitEvent = sdbGetTransCB()->getWaitLowTranEvent() ;

      SDB_ASSERT( NULL != updateEvent, "update event is invalid" ) ;
      SDB_ASSERT( NULL != waitEvent, "wait event is invalid" ) ;

      while ( !PMD_IS_DB_DOWN() &&
              !cb->isForced() )
      {
         if ( 0 == timeout % DPS_LOWTRAN_WAIT_SEC )
         {
            PD_LOG( PDDEBUG, "%s: start to update global lowTran", name() ) ;
            rc = _gtsAgent->updateGlobLowTran() ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "%s: Failed to update global lowTran, rc: %d",
                       name(), rc ) ;
            }
            // wake up waiters even if failed to update
            // NOTE: old value of global lowTran is OK for most cases
            waitEvent->signalAll( rc ) ;
         }

         // when waiting for update events, the status of this thread is wait
         // once found update events, it will be changed to running
         eduMgr->waitEDU( cb->getID() ) ;
         rc = updateEvent->wait( OSS_ONE_SEC ) ;
         eduMgr->activateEDU( cb ) ;

         // the database is shutting down
         if ( PMD_IS_DB_DOWN() ||
              cb->isForced() )
         {
            break ;
         }

         // if timeout, means no one signals, wait until timeout
         // otherwise, someone signal the update, do it immediately
         if ( SDB_TIMEOUT == rc )
         {
            ++ timeout ;
         }
         else
         {
            timeout = 0 ;
         }

         // clear for too frequent signals
         updateEvent->reset() ;
      } // end while

      // signal all waiters to stop waiting
      if ( NULL != waitEvent )
      {
         waitEvent->signalAll( SDB_APP_INTERRUPT ) ;
      }

      PD_LOG( PDDEBUG, "%s: end job", name() ) ;

      rc = SDB_OK ;

      PD_TRACE_EXITRC( SDB__DPSGTSLOWTRANJOB_DOIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSLOWTRANJOB__ONATTACH, "_dpsGTSLowTranJob::_onAttach" )
   void _dpsGTSLowTranJob::_onAttach()
   {
      PD_TRACE_ENTRY( SDB__DPSGTSLOWTRANJOB__ONATTACH ) ;

      // call on attach event of GTS agent
      if ( NULL != _gtsAgent )
      {
         _gtsAgent->setLocalRID( pmdGetNodeID() ) ;
         _gtsAgent->onAttach( eduCB() ) ;
      }

      PD_TRACE_EXIT( SDB__DPSGTSLOWTRANJOB__ONATTACH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSGTSLOWTRANJOB__ONDETACH, "_dpsGTSLowTranJob::_onDetach" )
   void _dpsGTSLowTranJob::_onDetach()
   {
      PD_TRACE_ENTRY( SDB__DPSGTSLOWTRANJOB__ONDETACH ) ;

      // call on detach event of GTS agent
      if ( NULL != _gtsAgent )
      {
         _gtsAgent->onDetach( eduCB() ) ;
      }

      PD_TRACE_EXIT( SDB__DPSGTSLOWTRANJOB__ONDETACH ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSSTARTGTSLOWTRANJOB, "dpsStartGTSLowTranJob" )
   INT32 dpsStartGTSLowTranJob( dpsGTSAgent *gtsAgent,
                                EDUID *eduID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSSTARTGTSLOWTRANJOB ) ;

      dpsGTSLowTranJob *job = NULL ;

      PD_CHECK( NULL != gtsAgent, SDB_SYS, error, PDERROR,
                "Failed to start GTSLowTranJob, GTS agent is invalid" ) ;

      job = SDB_OSS_NEW dpsGTSLowTranJob( gtsAgent ) ;
      PD_CHECK( NULL != job, SDB_OOM, error, PDERROR,
                "Failed to allocate job" ) ;
      rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_STOP_RET, eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start GTSLowTranJob, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DPSSTARTGTSLOWTRANJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
