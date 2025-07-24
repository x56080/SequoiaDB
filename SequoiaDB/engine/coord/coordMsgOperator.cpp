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

   Source File Name = coordMsgOperator.cpp

   Descriptive Name = Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/
#include "coordMsgOperator.hpp"
#include "rtn.hpp"
#include "coordUtil.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "pmdEnv.hpp"
#include "msg.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordMsgOperator implement
   */
   _coordMsgOperator::_coordMsgOperator()
   {
   }

   _coordMsgOperator::~_coordMsgOperator()
   {
   }

   const CHAR* _coordMsgOperator::getName() const
   {
      return "Msg" ;
   }

   INT32 _coordMsgOperator::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      UINT32 sucNum = 0 ;

      // fill default-reply
      contextID    = -1 ;

      CoordGroupList groupLst ;
      SET_ROUTEID sendNodes ;
      MsgRouteID routeID ;
      ROUTE_RC_MAP failedNodes ;

      pmdRemoteSession *pRemote     = _groupSession.getSession() ;
      pmdSubSession *pSub           = NULL ;
      SET_ROUTEID::iterator it ;

      // run msg on local node, in case that it isn't registered in the cluster.
      rtnMsg( (MsgOpMsg *)pMsg ) ;

      // list all groups
      rc = _pResource->updateGroupList( groupLst, cb, NULL,
                                        FALSE, FALSE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get all group list, rc: %d", rc ) ;

      // get nodes
      rc = coordGetGroupNodes( _pResource, cb, BSONObj(), NODE_SEL_ALL,
                               groupLst, sendNodes, NULL, FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc ) ;
      if ( sendNodes.size() == 0 )
      {
         PD_LOG( PDWARNING, "Not found any node" ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }

      // erase local node, because rtnMsg() has been executed on
      // local node already
      routeID = pmdGetNodeID() ;
      routeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
      sendNodes.erase( routeID.value ) ;

      /// clear
      _groupSession.clear() ;

      /// send msg
      it = sendNodes.begin() ;
      while( it != sendNodes.end() )
      {
         pSub = pRemote->addSubSession( *it ) ;
         pSub->setReqMsg( pMsg, PMD_EDU_MEM_NONE ) ;

         rc = pRemote->sendMsg( pSub ) ;
         if ( rc )
         {
            failedNodes[ *it ] = rc ;
            pRemote->delSubSession( *it ) ;
         }
         else
         {
            ++sucNum ;
         }
         ++it ;
      }

      /// recv reply
      rc = pRemote->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Wait reply failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( ( rc || failedNodes.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &failedNodes,
                                                    sucNum ) ) ;
      }
      _groupSession.clear() ;
      return rc ;
   error:
      goto done ;
   }

}

