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

   Source File Name = coordInterruptOperator.cpp

   Descriptive Name = Coord Interrupt

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
#include "coordInterruptOperator.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "msgMessageFormat.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

namespace engine
{

   /*
      _coordInterrupt implement
   */
   _coordInterrupt::_coordInterrupt()
   {
   }

   _coordInterrupt::~_coordInterrupt()
   {
   }

   const CHAR* _coordInterrupt::getName() const
   {
      return "Interrupt" ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_INTERRUPT_EXE, "_coordInterrupt::execute" )
   INT32 _coordInterrupt::execute( MsgHeader *pMsg,
                                   pmdEDUCB *cb,
                                   INT64 &contextID,
                                   rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_INTERRUPT_EXE ) ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;
      pmdRemoteSessionSite *pSite = NULL ;
      INT64 tmpContextID = -1 ;

      contextID = -1 ;

      /// send interrupt to all nodes associate with the session,
      /// and kill all context
      if ( cb->getRemoteSite() )
      {
         pSite = (pmdRemoteSessionSite*)cb->getRemoteSite() ;
         pSite->interruptAllSubSession() ;
      }

      /// set cb interrupted
      cb->interrupt() ;

      // delete all opened contexts when received the interrupt message
      while ( -1 != ( tmpContextID = cb->contextPeek() ) )
      {
         pRtncb->contextDelete( tmpContextID, NULL ) ;
      }

      PD_TRACE_EXITRC ( COORD_INTERRUPT_EXE, rc ) ;
      return rc ;
   }

}

