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

   Source File Name = coordInsertOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_INSERT_OPERATOR_HPP__
#define COORD_INSERT_OPERATOR_HPP__

#include "coordTransOperator.hpp"
#include "ossMemPool.hpp"
#include "utilInsertResult.hpp"
#include "coordKeyKicker.hpp"
#include "rtnInsertModifier.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordInsertOperator define
   */
   class _coordInsertOperator : public _coordTransOperator
   {
      typedef ossPoolMap< string, netIOVec >       SubCLObjsMap ;
      typedef ossPoolMap< UINT32, SubCLObjsMap >   GroupSubCLMap ;
      typedef vector< BSONObj >                    VEC_OBJECT ;

      class _SimpleBSONBuilder ;

      public:
         _coordInsertOperator() ;
         virtual ~_coordInsertOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32  execute( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;

         virtual BOOLEAN      isReadOnly() const ;

         UINT64         getInsertedNum() const ;
         UINT64         getDuplicatedNum() const ;
         BSONObj        getResultObj() const ;
         void           clearStat() ;

      private:
         INT32 shardDataByGroup( CoordCataInfoPtr &cataInfo,
                                 INT32 count,
                                 const CHAR *pInsertor,
                                 const netIOV &fixed,
                                 GROUP_2_IOVEC &datas ) ;

         INT32 shardAnObj( const CHAR *pInsertor,
                           CoordCataInfoPtr &cataInfo,
                           const netIOV &fixed,
                           GROUP_2_IOVEC &datas ) ;

         INT32 reshardData( CoordCataInfoPtr &cataInfo,
                            const netIOV &fixed,
                            GROUP_2_IOVEC &datas ) ;

         /// main collection relation
         INT32 shardAnObj( const CHAR *pInsertor,
                           CoordCataInfoPtr &cataInfo,
                           pmdEDUCB * cb,
                           GroupSubCLMap &groupSubCLMap ) ;

         INT32 shardDataByGroup( CoordCataInfoPtr &cataInfo,
                                 INT32 count,
                                 const CHAR *pInsertor,
                                 pmdEDUCB *cb,
                                 GroupSubCLMap &groupSubCLMap ) ;

         INT32 reshardData( CoordCataInfoPtr &cataInfo,
                            pmdEDUCB *cb,
                            GroupSubCLMap &groupSubCLMap ) ;

         INT32 buildInsertMsg( const netIOV &fixed,
                               GroupSubCLMap &groupSubCLMap,
                               vector< BSONObj > &subClInfoLst,
                               GROUP_2_IOVEC &datas ) ;

         /// AutoIncrement relation
         INT32 _addAutoIncToMsg( const clsAutoIncSet &autoIncSet,
                                 MsgOpInsert *pInsertMsg,
                                 CHAR const *pInsertor,
                                 const INT32 count,
                                 INT32 orgMsgLen,
                                 BOOLEAN needAppendID,
                                 pmdEDUCB *cb,
                                 CHAR **ppNewMsg,
                                 INT32 &newMsgSize,
                                 INT32 &newMsgLen,
                                 BOOLEAN &hasExplicitKey ) ;

         template <typename T>
         INT32 _addAutoIncToObj( const BSONObj &objIn,
                                 const T &set,
                                 pmdEDUCB *cb,
                                 _SimpleBSONBuilder &builder,
                                 BOOLEAN &hasExplicitKey,
                                 BOOLEAN needAppendID = FALSE ) ;

         INT32 _processUserInput( const clsAutoIncItem *pItem,
                                  BSONElement &ele,
                                  pmdEDUCB *cb,
                                  _SimpleBSONBuilder &builder,
                                  BOOLEAN &hasExplicitKey ) ;

         INT32 _appendAutoIncField( const clsAutoIncItem *pItem,
                                    pmdEDUCB *cb,
                                    _SimpleBSONBuilder &builder ) ;

         BOOLEAN _canRetry( INT32 count,
                            ROUTE_RC_MAP &failedNodes,
                            const clsAutoIncSet &set,
                            BOOLEAN hasExplicitKey ) ;

         void _removeLocalSeqCache( const clsAutoIncSet &set ) ;

         INT32 _checkIDField( const CHAR *pInsertor,
                              INT32 count,
                              BOOLEAN &needAppendID ) ;

         INT32 _addIDFieldToMsg( MsgOpInsert *pInsertMsg,
                                 const CHAR *pInsertor,
                                 INT32 count,
                                 INT32 orgMsgLen,
                                 pmdEDUCB *cb,
                                 CHAR **pNewMsg,
                                 INT32 &newMsgSize,
                                 INT32 &newMsgLen ) ;

      protected:

         virtual INT32              _prepareCLOp( coordCataSel &cataSel,
                                                  coordSendMsgIn &inMsg,
                                                  coordSendOptions &options,
                                                  pmdEDUCB *cb,
                                                  coordProcessResult &result ) ;

         virtual void               _doneCLOp( coordCataSel &cataSel,
                                               coordSendMsgIn &inMsg,
                                               coordSendOptions &options,
                                               pmdEDUCB *cb,
                                               coordProcessResult &result ) ;

         virtual INT32              _prepareMainCLOp( coordCataSel &cataSel,
                                                      coordSendMsgIn &inMsg,
                                                      coordSendOptions &options,
                                                      pmdEDUCB *cb,
                                                      coordProcessResult &result ) ;

         virtual void               _doneMainCLOp( coordCataSel &cataSel,
                                                   coordSendMsgIn &inMsg,
                                                   coordSendOptions &options,
                                                   pmdEDUCB *cb,
                                                   coordProcessResult &result ) ;

         virtual void               _prepareForTrans( pmdEDUCB *cb,
                                                      MsgHeader *pMsg ) ;

         virtual void               _onNodeReply( INT32 processType,
                                                  MsgOpReply *pReply,
                                                  pmdEDUCB *cb,
                                                  coordSendMsgIn &inMsg ) ;

         virtual INT32              _prepareExtraInfoForMsg( netIOVec &iov ) ;

      private:
         utilInsertResult  _inResult ;
         BOOLEAN           _hasRetry ;

         BOOLEAN           _hasGenerated ;
         INT64             _lastGenerateID ;

         /*
            For main collection
         */
         VEC_OBJECT        _vecObject ;
         GroupSubCLMap     _grpSubCLDatas ;

         rtnInsertModifier _modifier ;
         const CHAR *      _pHint ;
   } ;
   typedef _coordInsertOperator coordInsertOperator ;

}

#endif // COORD_INSERT_OPERATOR_HPP__

