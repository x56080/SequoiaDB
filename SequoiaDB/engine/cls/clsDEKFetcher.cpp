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

   Source File Name = clsDEKFetcher.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/30/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsDEKFetcher.hpp"
#include "utilSecurityKeys.hpp"

namespace engine
{

   _clsDEKFetcher::_clsDEKFetcher( _clsShardMgr &shardMgr ) : _shardMgr( shardMgr ) {}

   _clsDEKFetcher::~_clsDEKFetcher() {}

   INT32 _clsDEKFetcher::fetch( ossSM4Key dek )
   {
      INT32 rc = SDB_OK ;
      MsgHeader msg ;
      MsgHeader *pRecvMsg = NULL ;
      MsgOpReply *reply = NULL ;
      BOOLEAN isValid = FALSE ;
      msg.messageLength = sizeof( MsgHeader ) ;
      msg.TID = 0 ;
      msg.routeID.value = 0 ;
      msg.opCode = MSG_CAT_FETCH_DEK_REQ ;
      rc = _shardMgr.syncSend( &msg, CATALOG_GROUPID, TRUE, &pRecvMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to to receive msg, rc: %d", rc ) ;
      PD_LOG( PDDEBUG, "Send request to fetch DEK, rc: %d", rc ) ;

      reply = (MsgOpReply *)pRecvMsg ;
      if ( SDB_OK != reply->flags )
      {
         rc = reply->flags;
         PD_LOG( PDERROR, "Failed to fetch DEK, rc: %d", rc );
         goto error;
      }
      if ( reply->dataLen > 0 && reply->numReturned == 1 )
      {
         try
         {
            BSONObj dekInfo( (CHAR *)reply + sizeof( MsgOpReply ) ) ;
            rc = utilSecExtractDEKFromDEKInfoBSONObj( dekInfo, dek, isValid ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to extract DEK, rc: %d", rc ) ;
            if ( !isValid )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "The DEK and DEK signature do not match, rc: %d", rc );
               goto error ;
            }
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Impossible path" );
         goto error ;
      }

   done:
      SAFE_OSS_FREE( pRecvMsg ) ;
      return rc ;
   error:
      goto done ;
   }
} // namespace engine