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

   
*******************************************************************************/
#include "ossTypes.hpp"
#include <gtest/gtest.h>

#include "netMsgHandler.hpp"
#include "dpsTransCB.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "pmdEDUMgr.hpp"

using namespace std ;
using namespace engine ;

TEST( dpsGTSTest, allocTransID )
{
   // test case to allocate global transaction ID
   // NOTE: need STP support

   INT32 rc = SDB_OK ;

   MsgRouteID nodeID ;

   dpsTransCB transCB ;

   DPS_TRANS_ID transID ;
   stpLogicalTimeUS beginTime ;

   // prepare node ID, registered as COORD
   nodeID.columns.groupID = COORD_GROUPID ;
   nodeID.columns.nodeID = 1 ;
   nodeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

   transCB.onRegistered( nodeID ) ;

   // try allocate transaction ID
   rc = transCB.allocTransID( FALSE, TRUE, -1, transID, beginTime ) ;

   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( transID.isValid() ) ;
   ASSERT_TRUE( transID.isGlobTrans() ) ;
}

TEST( dpsGTSTest, transVisibility_1 )
{
   // test case to test visibility: read transaction started before write
   // transaction

   INT32 rc = SDB_OK ;

   MsgRouteID nodeID ;

   pmdEDUMgr mgr ;
   pmdEDUCB eduCB( &mgr, EDU_TYPE_SHARDAGENT ) ;

   dpsTransCB transCB ;

   DPS_TRANS_ID transID, recTransID ;
   stpLogicalTimeUS transBeginTime ;
   BOOLEAN visible = FALSE ;

   // prepare node ID, registered as DATA
   nodeID.columns.groupID = DATA_GROUP_ID_BEGIN ;
   nodeID.columns.nodeID = DATA_NODE_ID_BEGIN ;
   nodeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

   transCB.onRegistered( nodeID ) ;

   // read transaction started in 10000 from node 1
   transID.setSN( 10000 ) ;
   transID.setGlobTrans() ;
   transID.setNodeID( 1 ) ;

   transBeginTime.setTime( 10000 ) ;

   // write transaction started in 20000 from node 2
   recTransID.setSN( 20000 ) ;
   recTransID.setGlobTrans() ;
   recTransID.setNodeID( 2 ) ;

   // check visibility
   rc = transCB.isVersionVisible( &eduCB, recTransID, transID,
                                  transBeginTime, TRANS_ISOLATION_RR,
                                  FALSE, visible ) ;

   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( FALSE == visible ) ;
}

TEST( dpsGTSTest, arbitTrans_2 )
{
   // test case to test visibility: read transaction started after write
   // transaction committed

   INT32 rc = SDB_OK ;

   MsgRouteID nodeID ;

   pmdEDUMgr mgr ;
   pmdEDUCB eduCB( &mgr, EDU_TYPE_SHARDAGENT ) ;

   dpsTransCB transCB ;

   DPS_TRANS_ID transID, recTransID ;
   stpLogicalTimeUS transBeginTime ;
   dpsHisTransStatus recHistInfo ;

   BOOLEAN visible = FALSE ;

   // prepare node ID, registered as DATA
   nodeID.columns.groupID = DATA_GROUP_ID_BEGIN ;
   nodeID.columns.nodeID = DATA_NODE_ID_BEGIN ;
   nodeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

   transCB.onRegistered( nodeID ) ;

   // read transaction started in 20000 from node 1
   transID.setSN( 20000 ) ;
   transID.setGlobTrans() ;
   transID.setNodeID( 1 ) ;

   transBeginTime.setTime( 20000 ) ;

   // write transaction started in 10000 and committed in 15000 from node 2
   recTransID.setSN( 10000 ) ;
   recTransID.setGlobTrans() ;
   recTransID.setNodeID( 2 ) ;

   // add history info
   recHistInfo._status = DPS_TRANS_COMMIT ;
   recHistInfo._beginTime.setTime( 10000 ) ;
   recHistInfo._preCommitTime.setTime( 15000 ) ;

   transCB.addHisTrans( recTransID, recHistInfo, FALSE ) ;

   // check visibility
   rc = transCB.isVersionVisible( &eduCB, recTransID, transID,
                                  transBeginTime, TRANS_ISOLATION_RR,
                                  FALSE, visible ) ;

   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( TRUE == visible ) ;
}
