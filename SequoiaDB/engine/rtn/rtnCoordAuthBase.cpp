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

   Source File Name = rtnCoordAuthBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnCoordAuthBase.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnCoordCommon.hpp"
#include "msgAuth.hpp"

using namespace bson ;

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCOAUTHBASE_FORWARD, "rtnCoordAuthBase::forward" )
   INT32 rtnCoordAuthBase::forward( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT32 msgType,
                                    BOOLEAN sWhenNoPrimary,
                                    INT64 &contextID,
                                    const CHAR **ppUserName,
                                    const CHAR **ppPass,
                                    BSONObj *pOptions )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_RTNCOAUTHBASE_FORWARD ) ;
      pmdKRCB *pKrcb                   = pmdGetKRCB();
      CoordCB *pCoordcb                = pKrcb->getCoordCB();
      netMultiRouteAgent *pRouteAgent  = pCoordcb->getRouteAgent();
      pMsg->routeID.value = 0 ;
      pMsg->TID = cb->getTID() ;
      CoordGroupInfoPtr cata ;
      REQUESTID_MAP nodes ;
      REPLY_QUE replyQue ;
      //NodeID curNodeID = pmdGetNodeID() ;
      UINT32 times = 0 ;
      UINT32 primaryID = 0 ;

      contextID = -1 ;

      BSONObj authObj ;
      BSONElement user, pass, eOptions ;
      rc = extractAuthMsg( pMsg, authObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extrace auth msg, "
                   "rc: %d", rc ) ;
      user = authObj.getField( SDB_AUTH_USER ) ;
      pass = authObj.getField( SDB_AUTH_PASSWD ) ;
      eOptions = authObj.getField( FIELD_NAME_OPTIONS ) ;

      rc = rtnCoordGetCatGroupInfo( cb, FALSE, cata ) ;
      PD_RC_CHECK ( rc, PDWARNING, "Failed to get catalog group info, "
                    "rc = %d", rc  ) ;

      if ( ppUserName )
      {
         if ( String == user.type() )
         {
            *ppUserName = user.valuestr() ;
         }
         else
         {
            *ppUserName = "" ;
         }
      }
      if ( ppPass )
      {
         if ( String == pass.type() )
         {
            *ppPass = pass.valuestr() ;
         }
         else
         {
            *ppPass = "" ;
         }
      }
      if ( pOptions )
      {
         if ( Object == eOptions.type() )
         {
            *pOptions = eOptions.embeddedObject() ;
         }
         else
         {
            *pOptions = BSONObj() ;
         }
      }

   retry:
      nodes.clear() ;
      // send message
      rc = rtnCoordSendRequestToPrimary( (CHAR*)pMsg,
                                         cata, nodes,
                                         pRouteAgent,
                                         MSG_ROUTE_CAT_SERVICE,
                                         cb ) ;
      if ( SDB_OK != rc )
      {
         if ( sWhenNoPrimary )
         {
            rc = rtnCoordSendRequestToOne( (CHAR*)pMsg, cata,
                                           nodes, pRouteAgent,
                                           MSG_ROUTE_CAT_SERVICE,
                                           cb, TRUE ) ;
            PD_RC_CHECK ( rc, PDERROR, "Can not find a available cata node, "
                          "rc = %d", rc ) ;
         }
         else
         {
            PD_RC_CHECK ( rc, PDERROR, "Can not find the priamry, rc = %d",
                          rc ) ;
         }
      }

      rc = rtnCoordGetReply( cb, nodes, replyQue, msgType ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get reply from catalog for auth, "
                    "rc = %d", rc ) ;

      if ( !replyQue.empty() )
      {
         MsgHeader *res = (MsgHeader*)( replyQue.front() ) ;
         primaryID = MSG_GET_INNER_REPLY_STARTFROM(res) ;
         rc = MSG_GET_INNER_REPLY_RC(res) ;

         if ( rc )
         {
            if ( rtnCoordGroupReplyCheck( cb, rc, _canRetry( times++ ),
                                          res->routeID, cata, NULL,
                                          TRUE, primaryID, TRUE ) )
            {
               rtnClearReplyQue( &replyQue ) ;
               goto retry ;
            }
         }
         else if ( msgIsInnerOpReply( res ) )
         {
            _onSucReply( (const MsgOpReply*)res ) ;
         }
      }
      else
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR, "Empty reply is received" ) ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }

    done:
      rtnClearReplyQue( &replyQue ) ;
      PD_TRACE_EXITRC ( SDB_RTNCOAUTHBASE_FORWARD, rc ) ;
      return rc ;
   error:
      rtnCoordClearRequest( cb, nodes );
      goto done ;
   }

   void rtnCoordAuthBase::_onSucReply( const MsgOpReply *pReply )
   {
   }

   void rtnCoordAuthBase::updateSessionByOptions( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      UINT32 mask = 0 ;
      UINT32 configMask = 0 ;

      try
      {
         BSONElement e = options.getField( FIELD_NAME_AUDIT_MASK ) ;
         if ( String == e.type() )
         {
            rc = pdString2AuditMask( e.valuestr(), mask, TRUE, &configMask ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "User's audit config[%s] is invalid, rc: %d",
                       e.valuestr(), rc ) ;
               /// ignore
            }
            else
            {
               pdUpdateCurAuditMask( AUDIT_LEVEL_USER, mask, configMask ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         /// ignore
      }
   }

}

