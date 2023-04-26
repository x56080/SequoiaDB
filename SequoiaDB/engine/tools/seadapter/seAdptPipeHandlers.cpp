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
