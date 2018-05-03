/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

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
      cmd = cmdMgr->findCommand( "insert" ) ;
   }
   else if ( dbDelete == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "delete" ) ;
   }
   else if ( dbUpdate == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "update" ) ;
   }
   else if ( dbQuery == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "query" ) ;
   }
   else if ( dbGetMore == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "getMore" ) ;
   }
   else if ( dbKillCursors == _parser.dataPacket().opCode )
   {
      cmd = cmdMgr->findCommand( "killCursors" ) ;
   }

   if ( NULL == cmd )
   {
      rc = SDB_OPTION_NOT_SUPPORT ;
      goto error ;
   }

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

done:
   return rc ;
error:
   goto done ;
}

INT32 mongoConverter::reConvert( msgBuffer &out, MsgOpReply *reply )
{
   INT32 rc = SDB_OK ;
   INT32 numToReturn = -1 ;
   baseCommand *&cmd = _parser.command() ;
   commandMgr *cmdMgr = commandMgr::instance() ;
   if ( NULL == cmdMgr )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( OP_CMD_GET_INDEX == _parser.currentOption() ||
        OP_CMD_GET_CLS == _parser.currentOption() )
   {
      if ( SDB_OK != reply->flags )
      {
         rc = reply->flags ;
         goto done ;
      }
      else
      {
         MsgOpQuery* query = (MsgOpQuery *)out.data() ;
         numToReturn = query->numToReturn ;

         if ( 0 == reply->numReturned && 0 != reply->contextID )
         {
            out.zero() ;
            fap::mongo::buildGetMoreMsg( out ) ;
            MsgOpGetMore *msg = ( MsgOpGetMore *)out.data() ;
            msg->header.requestID = reply->header.requestID ;
            msg->contextID = reply->contextID ;
            msg->numToReturn = numToReturn ;
            goto done ;
         }
      }
   }

   out.zero() ;

   if ( OP_CMD_COUNT == _parser.currentOption() )
   {
      if ( SDB_OK != reply->flags )
      {
         rc = reply->flags ;
         goto done ;
      }

      numToReturn = 1 ;
      _parser.setCurrentOp( OP_CMD_COUNT_MORE );

      fap::mongo::buildGetMoreMsg( out ) ;
      MsgOpGetMore *msg = ( MsgOpGetMore* )out.data() ;
      msg->header.requestID = reply->header.requestID ;
      msg->contextID = reply->contextID ;
      msg->numToReturn = numToReturn ;
      goto done ;
   }

   // create collection failed
   if ( OP_CMD_CREATE == _parser.currentOption() )
   {
      // here mean mongo msg was converted to multi sdb msg
      // like create collection command msg
      // those msg convert to more than one sdb msg
      // that time cs may be not existed, should skip the error
      // and create collection space first
      if ( SDB_OK != reply->flags && SDB_DMS_CS_NOTEXIST == reply->flags )
      {
         _parser.reparse() ;
         cmd = cmdMgr->findCommand( "createCS" ) ;
         if ( NULL != cmd )
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
            goto done ;
         }
      }
      else
      {
         rc = reply->flags ;
         goto error ;
      }
   }

   // if is create collection space msg
   if ( OP_CMD_CREATE_CS == _parser.currentOption() )
   {
      if ( SDB_OK != reply->flags && SDB_DMS_CS_EXIST != reply->flags )
      {
         rc = reply->flags ;
         goto error ;
      }

      // then, try to create collection again
      _parser.reparse() ;
      cmd = cmdMgr->findCommand( "create" ) ;
      if ( NULL != cmd )
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
         goto done ;
      }
   }

   // when not handled above, assigned the reply flags to rc for return
   rc = reply->flags ;

done:
   return rc ;
error:
   goto done ;
}
