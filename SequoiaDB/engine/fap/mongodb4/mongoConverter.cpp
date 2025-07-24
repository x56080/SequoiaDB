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

   Source File Name = mongoConverter.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongoConverter.hpp"
#include "mongodef.hpp"
#include "commands.hpp"
#include "mongoReplyHelper.hpp"

INT32 mongoConverter::convert( msgBuffer &out )
{
   INT32 rc = SDB_OK ;
   baseCommand *&cmd = _parser.command() ;
   _parser.extractMsg( _msgdata, _msglen ) ;

   // convert mongodb msg to sequoiadb msg
   // for all kinds of requests available
   commandMgr *cmdMgr = commandMgr::instance() ;
   if ( NULL == cmdMgr )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( dbInsert == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_INSERT ) ;
   }
   else if ( dbDelete == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_DELETE ) ;
   }
   else if ( dbUpdate == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_UPDATE ) ;
   }
   else if ( dbQuery == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_QUERY ) ;
   }
   else if ( dbGetMore == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_GETMORE ) ;
   }
   else if ( dbKillCursors == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_KILLCURSORS ) ;
   }
   else if ( dbMsg == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( FAP_CMD_NAME_MSG ) ;
   }

   if ( NULL == cmd )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      goto error ;
   }

   try
   {
      rc = cmd->convert( _parser ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = cmd->buildMsg( _parser, out ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   catch ( std::exception &e )
   {
      PD_RC_CHECK( SDB_SYS, PDERROR,
                   "Failed to process command: %s, exception occurred: %s",
                   cmd->name(), e.what() ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

