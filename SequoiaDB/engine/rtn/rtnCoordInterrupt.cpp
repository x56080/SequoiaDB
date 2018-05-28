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

      /// set cb interrupted
      cb->interrupt() ;

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
