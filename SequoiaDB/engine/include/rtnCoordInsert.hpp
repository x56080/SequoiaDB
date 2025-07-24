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

   Source File Name = rtnCoordInsert.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNCOORDINSERT_HPP__
#define RTNCOORDINSERT_HPP__

#include "rtnCoordOperator.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class rtnCoordInsert : public rtnCoordTransOperator
   {
   typedef map< string, netIOVec >        SubCLObjsMap ;
   typedef map< UINT32, SubCLObjsMap >    GroupSubCLMap ;

   struct rtnCoordInsertPvtData
   {
      UINT64         _insertMixNum ;   /// InsertedNum(Hi) + IgnoredNum(Lo)
      GroupSubCLMap  _grpSubCLDatas ;

      rtnCoordInsertPvtData()
      {
         _insertMixNum = 0 ;
      }
   } ;

   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      INT32 shardDataByGroup( CoordCataInfoPtr &cataInfo,
                              INT32 count,
                              CHAR *pInsertor,
                              const netIOV &fixed,
                              GROUP_2_IOVEC &datas ) ;

      INT32 shardAnObj( CHAR *pInsertor,
                        CoordCataInfoPtr &cataInfo,
                        const netIOV &fixed,
                        GROUP_2_IOVEC &datas ) ;

      INT32 reshardData( CoordCataInfoPtr &cataInfo,
                         const netIOV &fixed,
                         GROUP_2_IOVEC &datas ) ;

      /// main collection relation
      INT32 shardAnObj( CHAR *pInsertor,
                        CoordCataInfoPtr &cataInfo,
                        pmdEDUCB * cb,
                        GroupSubCLMap &groupSubCLMap ) ;

      INT32 shardDataByGroup( CoordCataInfoPtr &cataInfo,
                              INT32 count,
                              CHAR *pInsertor,
                              pmdEDUCB *cb,
                              GroupSubCLMap &groupSubCLMap ) ;

      INT32 reshardData( CoordCataInfoPtr &cataInfo,
                         pmdEDUCB *cb,
                         GroupSubCLMap &groupSubCLMap ) ;

      INT32 buildInsertMsg( const netIOV &fixed,
                            GroupSubCLMap &groupSubCLMap,
                            vector< BSONObj > &subClInfoLst,
                            GROUP_2_IOVEC &datas ) ;

   protected:

      virtual INT32              _prepareCLOp( CoordCataInfoPtr &cataInfo,
                                               rtnSendMsgIn &inMsg,
                                               rtnSendOptions &options,
                                               netMultiRouteAgent *pRouteAgent,
                                               pmdEDUCB *cb,
                                               rtnProcessResult &result,
                                               ossValuePtr &outPtr ) ;

      virtual void               _doneCLOp( ossValuePtr itPtr,
                                            CoordCataInfoPtr &cataInfo,
                                            rtnSendMsgIn &inMsg,
                                            rtnSendOptions &options,
                                            netMultiRouteAgent *pRouteAgent,
                                            pmdEDUCB *cb,
                                            rtnProcessResult &result ) ;

      virtual INT32              _prepareMainCLOp( CoordCataInfoPtr &cataInfo,
                                                   CoordGroupSubCLMap &grpSubCl,
                                                   rtnSendMsgIn &inMsg,
                                                   rtnSendOptions &options,
                                                   netMultiRouteAgent *pRouteAgent,
                                                   pmdEDUCB *cb,
                                                   rtnProcessResult &result,
                                                   ossValuePtr &outPtr ) ;

      virtual void               _doneMainCLOp( ossValuePtr itPtr,
                                                CoordCataInfoPtr &cataInfo,
                                                CoordGroupSubCLMap &grpSubCl,
                                                rtnSendMsgIn &inMsg,
                                                rtnSendOptions &options,
                                                netMultiRouteAgent *pRouteAgent,
                                                pmdEDUCB *cb,
                                                rtnProcessResult &result ) ;

      virtual void               _prepareForTrans( pmdEDUCB *cb,
                                                   MsgHeader *pMsg ) ;

      virtual void               _onNodeReply( INT32 processType,
                                               MsgOpReply *pReply,
                                               pmdEDUCB *cb,
                                               rtnSendMsgIn &inMsg ) ;

   } ;
}

#endif

