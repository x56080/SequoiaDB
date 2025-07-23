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

   Source File Name = pmdTcpListener.cpp

   Descriptive Name = Process MoDel TCP Listener

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main entry point for TCP
   Listener.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <stdio.h>
#include "pd.hpp"
#include "ossMem.hpp"
#include "pmd.hpp"
#include "pmdEDUMgr.hpp"
#include "ossSocket.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDTCPLSTNENTPNT, "pmdTcpListenerEntryPoint" )
   INT32 pmdTcpListenerEntryPoint ( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDTCPLSTNENTPNT ) ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      pmdOptionsCB *optionCB = krcb->getOptionCB() ;
      monDBCB *mondbcb = krcb->getMonDBCB () ;
      pmdEDUMgr * eduMgr = cb->getEDUMgr() ;
      EDUID agentEDU = PMD_INVALID_EDUID ;
      pmdAgentParam *pAgentParam = NULL ;
      ossSocket *pListerner = ( ossSocket* )pData ;

      // let's set the state of EDU to RUNNING
      if ( SDB_OK != ( rc = eduMgr->activateEDU ( cb ) ) )
      {
         goto error ;
      }

      // master loop for tcp listener
      while ( !cb->isDisconnected() && !pListerner->isClosed() )
      {
         SOCKET s ;

         if ( pAgentParam )
         {
            SDB_OSS_DEL pAgentParam ;
            pAgentParam = NULL ;
         }

         // timeout in 10ms, so we won't hold global bind latch for too long
         // and it's only held at first time into the loop
         rc = pListerner->accept ( &s, NULL, NULL ) ;
         // if we don't get anything for a period of time, let's loop
         if ( SDB_TIMEOUT == rc )
         {
            rc = SDB_OK ;
            continue ;
         }
         else if ( SDB_TOO_MANY_OPEN_FD == rc )
         {
            pListerner->close() ;
            PD_LOG( PDERROR, "Can not accept more connections because of "
                    "open files upto limits, restart listening" ) ;
            pmdIncErrNum( rc ) ;

            while( !cb->isDisconnected() )
            {
               pListerner->close() ;
               ossSleep( 2 * OSS_ONE_SEC ) ;
               rc = pListerner->initSocket() ;
               if ( rc )
               {
                  continue ;
               }
               rc = pListerner->bind_listen() ;
               if ( rc )
               {
                  continue ;
               }
               PD_LOG( PDEVENT, "Restart listening on port[%d] succeed",
                       pListerner->getLocalPort() ) ;
               break ;
            }
            continue ;
         }
         // if we receive error due to database down, we finish
         if ( rc && PMD_IS_DB_DOWN() )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( rc )
         {
            // if we fail due to error, let's restart socket
            PD_LOG ( PDERROR, "Failed to accept socket in TcpListener(rc=%d)",
                     rc ) ;
            if ( pListerner->isClosed() )
            {
               break ;
            }
            else
            {
               continue ;
            }
         }

         cb->incEventCount() ;

         pAgentParam = SDB_OSS_NEW pmdAgentParam() ;
         if ( !pAgentParam )
         {
            PD_LOG( PDERROR, "Allocate memory failed" ) ;
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            continue ;
         }

         // assign the socket to the arg
         *(( SOCKET *)&pAgentParam->pSocket) = s ;
         pAgentParam->startTime = ossGetCurrentMicroseconds() ;

         if ( !krcb->isActive() )
         {
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            continue ;
         }

         mondbcb->connInc() ;
         if ( mondbcb->isConnLimited( optionCB->getMaxConn() ) )
         {
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            mondbcb->connDec();
            continue ;
         }

         // now we have a tcp socket for a new connection, let's get an 
         // agent, Note the new new socket sent passing to startEDU
         rc = eduMgr->startEDU ( EDU_TYPE_AGENT, pAgentParam, &agentEDU ) ;
         if ( rc )
         {
            PD_LOG( ( rc == SDB_QUIESCED ? PDWARNING : PDERROR ),
                    "Failed to start edu, rc: %d", rc ) ;

            // close remote connection if we can't create new thread
            ossSocket newsock ( &s ) ;
            newsock.close () ;
            mondbcb->connDec();
            continue ;
         }
         pAgentParam = NULL ;
         // Now EDU is started and posted with the new socket, let's
         // get back to wait for another request
      } //while ( ! cb->isDisconnected() )

      if ( SDB_OK != ( rc = eduMgr->waitEDU ( cb ) ) )
      {
         goto error ;
      }

   done :
      if ( pAgentParam )
      {
         SDB_OSS_DEL pAgentParam ;
         pAgentParam = NULL ;
      }
      PD_TRACE_EXITRC ( SDB_PMDTCPLSTNENTPNT, rc );
      return rc;
   error :
      switch ( rc )
      {
      case SDB_SYS :
         PD_LOG ( PDSEVERE, "System error occured" ) ;
         break ;
      default :
         PD_LOG ( PDSEVERE, "Internal error" ) ;
         break ;
      }
      goto done ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_TCPLISTENER, TRUE,
                          pmdTcpListenerEntryPoint,
                          "TCPListener" ) ;

}

