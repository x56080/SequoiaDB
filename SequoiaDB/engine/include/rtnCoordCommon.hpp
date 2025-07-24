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

   Source File Name = rtnCoordCommon.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef RTNCOORDCOMMON_HPP__
#define RTNCOORDCOMMON_HPP__

#include "coordCB.hpp"
#include "rtnContext.hpp"
#include "../bson/bson.h"
#include <vector>
#include "utilMap.hpp"
#include <queue>
#include <string>
#include <set>

using namespace bson ;

namespace engine
{
   typedef std::queue<CHAR *>                         REPLY_QUE ;
   typedef _utilMap< UINT64, INT32, 20 >              ROUTE_RC_MAP ;
   typedef _utilMap< UINT64, MsgHeader*, 20 >         ROUTE_REPLY_MAP ;
   typedef _utilMap< UINT32, netIOVec, 20 >           GROUP_2_IOVEC ;
   typedef std::set< INT32 >                          SET_RC ;

   INT32 rtnCoordGetReply ( pmdEDUCB *cb, REQUESTID_MAP &requestIdMap,
                            REPLY_QUE &replyQue, const SINT32 opCode,
                            BOOLEAN isWaitAll = TRUE,
                            BOOLEAN clearReplyIfFailed = TRUE ) ;

   INT32 rtnCoordCataQuery ( const CHAR *pCollectionName,
                             const bson::BSONObj &selector,
                             const bson::BSONObj &matcher,
                             const bson::BSONObj &orderBy,
                             const bson::BSONObj &hint,
                             INT32 flag,
                             pmdEDUCB *cb,
                             SINT64 numToSkip,
                             SINT64 numToReturn,
                             SINT64 &contextID );

   INT32 getServiceName ( bson::BSONElement &beService,
                          INT32 serviceType,
                          std::string &strServiceName );

   INT32 rtnCoordNodeQuery ( const CHAR *pCollectionName,
                             const bson::BSONObj &condition,
                             const bson::BSONObj &selector,
                             const bson::BSONObj &orderBy,
                             const bson::BSONObj &hint,
                             INT64 numToSkip, INT64 numToReturn,
                             CoordGroupList &groupLst,
                             pmdEDUCB *cb,
                             rtnContext **ppContext,
                             const CHAR *realCLName = NULL,
                             INT32 flag = 0 ) ;

   INT32 rtnCoordGetCataInfo( pmdEDUCB *cb,
                              const CHAR *pCollectionName,
                              BOOLEAN isNeedRefreshCata,
                              CoordCataInfoPtr &cataInfo,
                              BOOLEAN *pHasUpdate = NULL );

   INT32 rtnCoordGetGroupInfo ( pmdEDUCB *cb,
                                UINT32 groupID,
                                BOOLEAN isNeedRefresh,
                                CoordGroupInfoPtr &groupInfo,
                                BOOLEAN *pHasUpdate = NULL ) ;

   INT32 rtnCoordGetGroupInfo ( pmdEDUCB *cb,
                                const CHAR *groupName,
                                BOOLEAN isNeedRefresh,
                                CoordGroupInfoPtr &groupInfo,
                                BOOLEAN *pHasUpdate = NULL ) ;


   INT32 rtnCoordGetCatGroupInfo ( pmdEDUCB *cb,
                                   BOOLEAN isNeedRefresh,
                                   CoordGroupInfoPtr &groupInfo,
                                   BOOLEAN *pHasUpdate = NULL ) ;

   /*
      Retry when error occurrs and hasUpdate = FALSE
   */
   INT32 rtnCoordGetGroupsByCataInfo( CoordCataInfoPtr &cataInfo,
                                      CoordGroupList &sendGroupLst,
                                      CoordGroupList &groupLst,
                                      pmdEDUCB *cb,
                                      BOOLEAN *pHasUpdate = NULL,
                                      const BSONObj *pQuery = NULL ) ;

   void  rtnCoordRemoveGroup( UINT32 group ) ;

   /*
      Send to nodes, when failed, will push the node to failedNodes,
      otherwise push to sendNodes
   */
   INT32 rtnCoordSendRequestToNodes( void *pBuffer,
                                     ROUTE_SET &nodes,
                                     netMultiRouteAgent *pRouteAgent,
                                     pmdEDUCB *cb,
                                     REQUESTID_MAP &sendNodes,
                                     ROUTE_RC_MAP &failedNodes ) ;

   /*
      Not retry when send failed
   */
   INT32 rtnCoordSendRequestToNode( void *pBuffer,
                                    MsgRouteID routeID,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb,
                                    REQUESTID_MAP &sendNodes );

   INT32 rtnCoordSendRequestToNode( void *pBuffer,
                                    MsgRouteID routeID,
                                    netMultiRouteAgent *pRouteAgent,
                                    pmdEDUCB *cb,
                                    const netIOVec &iov,
                                    REQUESTID_MAP &sendNodes );

   /*
      Not retry when send failed
   */
   INT32 rtnCoordSendRequestToNodeWithoutReply( void *pBuffer,
                                                MsgRouteID &routeID,
                                                netMultiRouteAgent *pRouteAgent );

   void  rtnCoordSendRequestToNodesWithOutReply( void *pBuffer,
                                                 ROUTE_SET &nodes,
                                                 netMultiRouteAgent *pRouteAgent ) ;

   INT32 rtnCoordSendRequestToNodeWithoutCheck( void *pBuffer,
                                                const MsgRouteID &routeID,
                                                netMultiRouteAgent *pRouteAgent,
                                                pmdEDUCB *cb,
                                                REQUESTID_MAP &sendNodes );

   /*
      Send to node group, retry when send failed
   */
   INT32 rtnCoordSendRequestToNodeGroup( MsgHeader *pBuffer,
                                         CoordGroupInfoPtr &groupInfo,
                                         BOOLEAN isSendPrimary,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         const netIOVec &iov,
                                         REQUESTID_MAP &sendNodes,
                                         BOOLEAN isResend,
                                         MSG_ROUTE_SERVICE_TYPE type =
                                         MSG_ROUTE_SHARD_SERVCIE ) ;

   INT32 rtnCoordSendRequestToNodeGroup( CHAR *pBuffer,
                                         CoordGroupInfoPtr &groupInfo,
                                         BOOLEAN isSendPrimary,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         REQUESTID_MAP &sendNodes,
                                         BOOLEAN isResend,
                                         MSG_ROUTE_SERVICE_TYPE type ) ;

   /*
      Send to node groups, break when send to a group failed
   */
   INT32 rtnCoordSendRequestToNodeGroups( MsgHeader *pBuffer,
                                          CoordGroupList &groupLst,
                                          CoordGroupMap &mapGroupInfo,
                                          BOOLEAN isSendPrimary,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          const netIOVec &iov,
                                          REQUESTID_MAP &sendNodes,
                                          BOOLEAN isResend,
                                          MSG_ROUTE_SERVICE_TYPE type =
                                          MSG_ROUTE_SHARD_SERVCIE ) ;

   INT32 rtnCoordSendRequestToNodeGroups( MsgHeader *pBuffer,
                                          CoordGroupList &groupLst,
                                          CoordGroupMap &mapGroupInfo,
                                          BOOLEAN isSendPrimary,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          GROUP_2_IOVEC &iov,
                                          REQUESTID_MAP &sendNodes,
                                          BOOLEAN isResend,
                                          MSG_ROUTE_SERVICE_TYPE type =
                                          MSG_ROUTE_SHARD_SERVCIE ) ;

   INT32 rtnCoordSendRequestToNodeGroups( CHAR *pBuffer,
                                          CoordGroupList &groupLst,
                                          CoordGroupMap &mapGroupInfo,
                                          BOOLEAN isSendPrimary,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          REQUESTID_MAP &sendNodes,
                                          BOOLEAN isResend,
                                          MSG_ROUTE_SERVICE_TYPE type =
                                          MSG_ROUTE_SHARD_SERVCIE,
                                          BOOLEAN ignoreError = FALSE ) ;

   /*
      Send to primary node group
      When transaction node invalid, will send to transaction node
      Update and retry when send to primary failed
   */
   INT32 rtnCoordSendRequestToPrimary( CHAR *pBuffer,
                                       CoordGroupInfoPtr &groupInfo,
                                       REQUESTID_MAP &sendNodes,
                                       netMultiRouteAgent *pRouteAgent,
                                       MSG_ROUTE_SERVICE_TYPE type,
                                       pmdEDUCB *cb ) ;

   INT32 rtnCoordSendRequestToPrimary( MsgHeader *pBuffer,
                                       CoordGroupInfoPtr &groupInfo,
                                       REQUESTID_MAP &sendNodes,
                                       netMultiRouteAgent *pRouteAgent,
                                       const netIOVec &iov,
                                       MSG_ROUTE_SERVICE_TYPE type,
                                       pmdEDUCB *cb ) ;

   /*
      Send to one node of group, select by prefer instance
      Retry every node of group until send succeed
      Update group info and retry when send all failed
   */
   INT32 rtnCoordSendRequestToOne( CHAR *pBuffer,
                                   CoordGroupInfoPtr &groupInfo,
                                   REQUESTID_MAP &sendNodes,
                                   netMultiRouteAgent *pRouteAgent,
                                   MSG_ROUTE_SERVICE_TYPE type,
                                   pmdEDUCB *cb,
                                   BOOLEAN isResend ) ;

   INT32 rtnCoordSendRequestToOne( MsgHeader *pBuffer,
                                   CoordGroupInfoPtr &groupInfo,
                                   REQUESTID_MAP &sendNodes,
                                   netMultiRouteAgent *pRouteAgent,
                                   const netIOVec &iov,
                                   MSG_ROUTE_SERVICE_TYPE type,
                                   pmdEDUCB *cb,
                                   BOOLEAN isResend ) ;

   /*
      Get local group info
   */
   INT32 rtnCoordGetLocalGroupInfo ( UINT32 groupID,
                                     CoordGroupInfoPtr &groupInfo ) ;

   INT32 rtnCoordGetLocalGroupInfo ( const CHAR *groupName,
                                     CoordGroupInfoPtr &groupInfo ) ;

   INT32 rtnCoordGetRemoteCataGroupInfoByAddr ( pmdEDUCB *cb,
                                                CoordGroupInfoPtr &groupInfo ) ;

   /// if only want to get groupinfo by groupid,
   /// set groupName = NULL.
   INT32 rtnCoordGetRemoteGroupInfo ( pmdEDUCB *cb,
                                      UINT32 groupID,
                                      const CHAR *groupName,
                                      CoordGroupInfoPtr &groupInfo,
                                      BOOLEAN addToLocal = TRUE ) ;

   /*
      Get catalog group info
   */
   INT32 rtnCoordGetLocalCatGroupInfo ( CoordGroupInfoPtr &groupInfo );

   INT32 rtnCoordGetRemoteCatGroupInfo ( pmdEDUCB *cb,
                                         CoordGroupInfoPtr &groupInfo );

   /*
      Get catalog infomation
   */
   INT32 rtnCoordGetLocalCata( const CHAR *pCollectionName,
                               CoordCataInfoPtr &cataInfo ) ;

   INT32 rtnCoordGetRemoteCata( pmdEDUCB *cb,
                                const CHAR *pCollectionName,
                                CoordCataInfoPtr &cataInfo ) ;

   /*
      will update node stat by reply flag
   */
   INT32 rtnCoordProcessQueryCatReply ( MsgCatQueryCatRsp *pReply,
                                        CoordCataInfoPtr &cataInfo ) ;

   /*
      will update node stat by reply flag
   */
   INT32 rtnCoordProcessGetGroupReply ( MsgHeader *pReply,
                                        CoordGroupInfoPtr &groupInfo ) ;

   INT32 rtnCoordUpdateRoute ( CoordGroupInfoPtr &groupInfo,
                               netMultiRouteAgent *pRouteAgent,
                               MSG_ROUTE_SERVICE_TYPE type ) ;

   INT32 rtnCoordReadALine( const CHAR *&pInput, CHAR *pOutput );

   void rtnCoordClearRequest( pmdEDUCB *cb, REQUESTID_MAP &sendNodes );

   INT32 rtnCoordGetSubCLsByGroups( const CoordSubCLlist &subCLList,
                                    const CoordGroupList &sendGroupList,
                                    pmdEDUCB *cb,
                                    CoordGroupSubCLMap &groupSubCLMap,
                                    const BSONObj *query = NULL );

   INT32 rtnCoordParseGroupList( pmdEDUCB *cb, const BSONObj &obj,
                                 CoordGroupList &groupList,
                                 BSONObj *pNewObj = NULL ) ;

   enum FILTER_BSON_ID
   {
      FILTER_ID_MATCHER    = 1,
      FILTER_ID_SELECTOR,
      FILTER_ID_ORDERBY,
      FILTER_ID_HINT
   } ;

   BSONObj* rtnCoordGetFilterByID( FILTER_BSON_ID filterID,
                                   rtnQueryOptions &queryOption ) ;

   INT32 rtnCoordParseGroupList( pmdEDUCB *cb, MsgOpQuery *pMsg,
                                 FILTER_BSON_ID filterObjID,
                                 CoordGroupList &groupList ) ;

   INT32 rtnCoordGetAllGroupList( pmdEDUCB * cb, GROUP_VEC &groupLst,
                                  const BSONObj *query = NULL,
                                  BOOLEAN exceptCata = FALSE,
                                  BOOLEAN exceptCoord = TRUE,
                                  BOOLEAN useLocalWhenFailed = TRUE ) ;

   INT32 rtnCoordGetAllGroupList( pmdEDUCB * cb, CoordGroupList &groupList,
                                  const BSONObj *query = NULL,
                                  BOOLEAN exceptCata = FALSE,
                                  BOOLEAN exceptCoord = TRUE,
                                  BOOLEAN useLocalWhenFailed = TRUE ) ;

   INT32 rtnGroupList2GroupPtr( pmdEDUCB *cb, CoordGroupList &groupList,
                                CoordGroupMap &groupMap,
                                BOOLEAN reNew = FALSE ) ;
   INT32 rtnGroupList2GroupPtr( pmdEDUCB *cb, CoordGroupList &groupList,
                                GROUP_VEC &groupPtrs ) ;
   INT32 rtnGroupPtr2GroupList( pmdEDUCB *cb, GROUP_VEC &groupPtrs,
                                CoordGroupList &groupList ) ;

   enum NODE_SEL_STY
   {
      NODE_SEL_ALL         = 1,
      NODE_SEL_PRIMARY,
      NODE_SEL_SECONDARY,
      NODE_SEL_ANY
   } ;
   INT32 rtnCoordGetGroupNodes( pmdEDUCB *cb, const BSONObj &filterObj,
                                NODE_SEL_STY emptyFilterSel,
                                CoordGroupList &groupList, ROUTE_SET &nodes,
                                BSONObj *pNewObj = NULL ) ;

   void  rtnCoordGetNodePos( INT32 preferReplicaType,
                             clsGroupItem *groupItem,
                             UINT32 random,
                             UINT32 &pos ) ;

   void  rtnCoordGetNextNode( INT32 preferReplicaType,
                              clsGroupItem *groupItem,
                              UINT32 &selTimes,
                              UINT32 &curPos ) ;

   void rtnCoordUpdateNodeStatByRC( pmdEDUCB *cb,
                                    const MsgRouteID &routeID,
                                    CoordGroupInfoPtr &groupInfo,
                                    INT32 retCode ) ;

   void rtnCoordUpdateNodeStatByRC( pmdEDUCB *cb,
                                    const MsgRouteID &routeID,
                                    INT32 retCode ) ;

   /*
      return TRUE/FALSE, if TRUE: can retry, otherwise error stop
   */
   BOOLEAN rtnCoordGroupReplyCheck( pmdEDUCB *cb, INT32 flag,
                                    BOOLEAN canRetry,
                                    const MsgRouteID &nodeID,
                                    CoordGroupInfoPtr &groupInfo,
                                    BOOLEAN *pUpdate = NULL,
                                    BOOLEAN canUpdate = TRUE,
                                    UINT32 primaryID = 0,
                                    BOOLEAN isReadCmd = FALSE ) ;
   /*
      return TRUE/FALSE, if TRUE: can retry, otherwise error stop
   */
   BOOLEAN rtnCoordCataReplyCheck( pmdEDUCB *cb, INT32 flag,
                                   BOOLEAN canRetry,
                                   CoordCataInfoPtr &cataInfo,
                                   BOOLEAN *pUpdate = NULL,
                                   BOOLEAN canUpdate = TRUE ) ;

   INT32 rtnCataChangeNtyToAllNodes( pmdEDUCB *cb ) ;

   #define RTN_CTRL_MASK_GLOBAL           0x00000001
   #define RTN_CTRL_MASK_NODE_SELECT      0x00000002
   #define RTN_CTRL_MASK_ROLE             0x00000004
   #define RTN_CTRL_MASK_RAWDATA          0x00000008

   #define RTN_CTRL_MASK_ALL              0xFFFFFFFF

   struct _rtnCoordCtrlParam
   {
      BOOLEAN           _isGlobal ;             // RTN_CTRL_MASK_GLOBAL
      FILTER_BSON_ID    _filterID ;
      NODE_SEL_STY      _emptyFilterSel ;       // RTN_CTRL_MASK_NODE_SELECT
      INT32             _role[ SDB_ROLE_MAX ] ; // RTN_CTRL_MASK_ROLE
      BOOLEAN           _rawData ;              // RTN_CTRL_MASK_RAWDATA

      UINT32            _parseMask ;

      _rtnCoordCtrlParam()
      {
         _isGlobal = TRUE ;
         _filterID = FILTER_ID_MATCHER ;
         _emptyFilterSel = NODE_SEL_ALL ;
         ossMemset( &_role, 0, sizeof( _role ) ) ;
         _role[ SDB_ROLE_DATA ] = 1 ;
         _role[ SDB_ROLE_CATALOG ] = 1 ;
         _rawData = FALSE ;
         _parseMask = 0 ;
      }
   } ;
   typedef _rtnCoordCtrlParam rtnCoordCtrlParam ;

   INT32 rtnCoordParseControlParam( const BSONObj &obj,
                                    rtnCoordCtrlParam &param,
                                    UINT32 mask,
                                    BSONObj *pNewObj = NULL ) ;

   BOOLEAN rtnCoordCanRetry( UINT32 retryTimes ) ;

}

#endif //RTNCOORDCOMMON_HPP__

