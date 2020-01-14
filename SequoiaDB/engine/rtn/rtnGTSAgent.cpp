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

   Source File Name = rtnGTSAgent.cpp

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

#include "rtnGTSAgent.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "rtnTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{

   // update lowTran for each 30 seconds
   // NOTE: so global lowTran from different nodes should be updated in at
   //       lease 60 seconds in 2 rounds
   #define RTN_LOWTRAN_WAIT_SEC ( 30 )

   /*
      _rtnGTSAgent implement
    */
   _rtnGTSAgent::_rtnGTSAgent()
   : _transCB( sdbGetTransCB() )
   {
      SDB_ASSERT( NULL != _transCB, "transCB is invalid" ) ;
   }

   _rtnGTSAgent::~_rtnGTSAgent()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGTSAGENT__GETLOCALLOWTRAN, "_rtnGTSAgent::_getLocalLowTran" )
   INT32 _rtnGTSAgent::_getLocalLowTran( DPS_TRANSID_SN &localLowTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNGTSAGENT__GETLOCALLOWTRAN ) ;

      DPS_TRANSID_SN lowTran = DPS_MAX_TRANSID_SN ;

      // only get lowTran when global transaction is enabled
      if ( _transCB->isTransOn() && _transCB->isGlobTransOn() )
      {
         DPS_TRANS_ID lowTranID = _transCB->getLocalLowTran() ;
         if ( lowTranID.isValid() )
         {
            lowTran = lowTranID.getGlobSN() ;
         }
      }

      // copy to output
      // if no global transaction in local, we report max value of
      // transaction SN to caller
      localLowTran = lowTran ;

      PD_LOG( PDDEBUG, "Got local lowTran [%llu(0x%llX)]",
              lowTran, lowTran ) ;

      PD_TRACE_EXITRC( SDB__RTNGTSAGENT__GETLOCALLOWTRAN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGTSAGENT__SETNODELOWTRANBSON, "_rtnGTSAgent::_setGlobLowTran" )
   INT32 _rtnGTSAgent::_setGlobLowTran( const DPS_TRANSID_SN &globLowTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNGTSAGENT__SETNODELOWTRANBSON ) ;

      if ( _transCB->isTransOn() && _transCB->isGlobTransOn() )
      {
         _transCB->setGlobLowTran( globLowTran ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNGTSAGENT__SETNODELOWTRANBSON, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGTSAGENT__FILLLOWTRANREQUEST, "_rtnGTSAgent::_fillLowTranRequest" )
   INT32 _rtnGTSAgent::_fillLowTranRequest( MsgGTSLowTranReq *request,
                                            const DPS_TRANSID_SN &nodeLowTran,
                                            BSONObj &requestObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNGTSAGENT__FILLLOWTRANREQUEST ) ;

      // build request object
      try
      {
         requestObject = BSON( FIELD_NAME_TRANS_LOWTRAN <<
                               (INT64)nodeLowTran ) ;
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
      request->routeID.value = pmdGetNodeID().value ;
      request->requestID = 0LL ;

   done:
      PD_TRACE_EXITRC( SDB__RTNGTSAGENT__FILLLOWTRANREQUEST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGTSAGENT__PARSELOWTRANRSP, "_rtnGTSAgent::_parseLowTranResponse" )
   INT32 _rtnGTSAgent::_parseLowTranResponse( MsgGTSLowTranRsp *response,
                                              DPS_TRANSID_SN &globLowTran )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNGTSAGENT__PARSELOWTRANRSP ) ;

      BSONObj responseObject ;

      // check message length
      PD_CHECK( response->header.messageLength >
                        (INT32)( sizeof( MsgGTSLowTranRsp ) ) +
                        responseObject.objsize(),
                SDB_SYS, error, PDERROR,
                "Failed to extract lowTran response, "
                "message length [%d] is unexpected",
                response->header.messageLength ) ;

      // parse global lowTran
      try
      {
         responseObject = BSONObj( (CHAR *)response +
                                   sizeof( MsgGTSLowTranRsp ) ) ;
         BSONElement element =
                     responseObject.getField( FIELD_NAME_TRANS_GLOBLOWTRAN ) ;
         PD_CHECK( EOO != element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it should not be empty",
                   FIELD_NAME_TRANS_GLOBLOWTRAN ) ;
         PD_CHECK( NumberLong == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it should be long type",
                   FIELD_NAME_TRANS_GLOBLOWTRAN ) ;
         globLowTran = (DPS_TRANSID_SN)( element.numberLong() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build response object, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNGTSAGENT__PARSELOWTRANRSP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
       _rtnGTSLowTranJob implement
    */
   _rtnGTSLowTranJob::_rtnGTSLowTranJob( rtnGTSAgent *gtsAgent )
   : _gtsAgent( gtsAgent )
   {
      SDB_ASSERT( NULL != gtsAgent, "GTS agent is invalid" ) ;
   }

   _rtnGTSLowTranJob::~_rtnGTSLowTranJob()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNGTSLOWTRANJOB_DOIT, "_rtnGTSLowTranJob::doit" )
   INT32 _rtnGTSLowTranJob::doit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNGTSLOWTRANJOB_DOIT ) ;

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
         if ( 0 == timeout % RTN_LOWTRAN_WAIT_SEC )
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

      PD_TRACE_EXITRC( SDB__RTNGTSLOWTRANJOB_DOIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNSTARTGTSLOWTRANJOB, "rtnStartGTSLowTranJob" )
   INT32 rtnStartGTSLowTranJob( rtnGTSAgent *gtsAgent,
                                EDUID *eduID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNSTARTGTSLOWTRANJOB ) ;

      rtnGTSLowTranJob *job = NULL ;

      PD_CHECK( NULL != gtsAgent, SDB_SYS, error, PDERROR,
                "Failed to start GTSLowTranJob, GTS agent is invalid" ) ;

      job = SDB_OSS_NEW rtnGTSLowTranJob( gtsAgent ) ;
      PD_CHECK( NULL != job, SDB_OOM, error, PDERROR,
                "Failed to allocate job" ) ;
      rc = rtnGetJobMgr()->startJob( job, RTN_JOB_MUTEX_STOP_RET, eduID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start GTSLowTranJob, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNSTARTGTSLOWTRANJOB, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
