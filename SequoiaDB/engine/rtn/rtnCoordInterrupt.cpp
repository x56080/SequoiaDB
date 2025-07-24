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

   Source File Name = rtnCoordInterrupt.cpp

   Descriptive Name = Runtime Coord Interrupt

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   interrupt processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "rtnCoordInterrupt.hpp"
#include "coordSession.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOINTERRUPT_EXECUTE, "rtnCoordInterrupt::execute" )
   INT32 rtnCoordInterrupt::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOINTERRUPT_EXECUTE ) ;
      pmdKRCB *pKrcb = pmdGetKRCB();
      SDB_RTNCB *pRtncb = pKrcb->getRTNCB();
      contextID = -1 ;

      // send interrut message to all sub-session
      CoordSession *pSession = cb->getCoordSession();
      if ( pSession )
      {
         ROUTE_SET routeSet;
         pSession->getAllSessionRoute( routeSet );
         SendInterrupt( cb, routeSet );
      }

      // delete all opened contexts when received the interrupt message
      {
         SINT64 contextID = -1 ;
         while ( -1 != (contextID = cb->contextPeek() ))
            pRtncb->contextDelete( contextID, NULL ) ;
      }

      PD_TRACE_EXITRC ( SDB_RTNCOINTERRUPT_EXECUTE, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOINTERRUPT_SENDINTER, "rtnCoordInterrupt::SendInterrupt" )
   void rtnCoordInterrupt::SendInterrupt( pmdEDUCB *cb, ROUTE_SET &routeSet )
   {
      PD_TRACE_ENTRY ( SDB_RTNCOINTERRUPT_SENDINTER ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      netMultiRouteAgent *pRouteAgent = krcb->getCoordCB()->getRouteAgent();

      MsgHeader interruptMsg;
      interruptMsg.messageLength = sizeof( MsgHeader );
      interruptMsg.opCode = MSG_BS_INTERRUPTE;
      interruptMsg.TID = cb->getTID();
      interruptMsg.routeID.value = 0;

      rtnCoordSendRequestToNodesWithOutReply((void *)(&interruptMsg), routeSet,
                                             pRouteAgent );
      PD_TRACE_EXIT ( SDB_RTNCOINTERRUPT_SENDINTER ) ;
   }
}
