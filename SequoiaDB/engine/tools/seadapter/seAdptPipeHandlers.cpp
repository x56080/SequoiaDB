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

   Source File Name = seAdptPipeHandlers.cpp

   Descriptive Name = seadapter pipe handlers

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for pipe
   manager.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          25/04/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "seAdptPipeHandlers.hpp"
#include "pmdDef.hpp"
#include "seAdptMgr.hpp"

namespace seadapter
{
   /*
      _seAdapterPipeHandler implement
    */
   _seAdapterPipeHandler::_seAdapterPipeHandler()
   {
   }

   _seAdapterPipeHandler::~_seAdapterPipeHandler()
   {
   }

   INT32 _seAdapterPipeHandler::processMessage( CHAR *message, utilNodePipe &nodePipe )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != message, "message is invalid" ) ;

      if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_STARTTIME,
                            sizeof( ENGINE_NPIPE_MSG_STARTTIME ) ) )
      {
         UINT64 startTime = sdbGetSeAdapterCB()->getStartTime() ;
         rc = nodePipe.writePipe( (const CHAR *)( &startTime ), sizeof( startTime ) ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_MODE,
                sizeof( ENGINE_NPIPE_MSG_MODE ) ) )
      {
         const CHAR* modeStr = sdbGetSeAdapterCB()->getModeStr() ;
         rc = nodePipe.writePipe( modeStr, ossStrlen( modeStr ) + 1 ) ;
      }
      else if ( 0 == ossStrncmp( message, ENGINE_NPIPE_MSG_DATASVCNAME,
                                 sizeof( ENGINE_NPIPE_MSG_DATASVCNAME ) ) )
      {
         const CHAR* dataSvcName = sdbGetSeAdptOptions()->getDBService() ;
         rc = nodePipe.writePipe( dataSvcName, ossStrlen( dataSvcName ) + 1 ) ;
      }
      else
      {
         rc = SDB_UNKNOWN_MESSAGE ;
      }

      return rc ;
   }

}
