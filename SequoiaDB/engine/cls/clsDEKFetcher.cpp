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