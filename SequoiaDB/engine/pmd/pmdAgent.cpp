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

   Source File Name = pmdAgent.cpp

   Descriptive Name = Process MoDel Agent

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "pmdLocalSession.hpp"
#include "pmdProcessor.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDLOCALAGENTENTPNT, "pmdLocalAgentEntryPoint" )
   INT32 pmdLocalAgentEntryPoint( pmdEDUCB *cb, void *arg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_PMDLOCALAGENTENTPNT );

      SOCKET s = *(( SOCKET *) &arg ) ;
      pmdLocalSession localSession( s ) ;
      localSession.attach( cb ) ;

      if ( pmdGetDBRole() == SDB_ROLE_COORD )
      {
         pmdCoordProcessor coordProcessor ;
         localSession.attachProcessor( &coordProcessor ) ;
         rc = localSession.run() ;
         localSession.detachProcessor() ;
      }
      else
      {
         pmdDataProcessor dataProcessor ;
         localSession.attachProcessor( &dataProcessor ) ;
         rc = localSession.run() ;
         localSession.detachProcessor() ;
      }

      localSession.detach() ;

      pmdGetKRCB()->getMonDBCB ()->connDec();

      PD_TRACE_EXITRC ( SDB_PMDLOCALAGENTENTPNT, rc );
      return rc ;
   }

   /// Register
   PMD_DEFINE_ENTRYPOINT( EDU_TYPE_AGENT, FALSE,
                          pmdLocalAgentEntryPoint,
                          "Agent" ) ;

}

