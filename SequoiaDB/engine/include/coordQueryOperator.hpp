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

   Source File Name = coordQueryOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/17/2017  XJH  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_QUERY_OPERATOR_HPP__
#define COORD_QUERY_OPERATOR_HPP__

#include "coordTransOperator.hpp"
#include "coordContext.hpp"

using namespace bson ;

namespace engine
{

   /*
      coordQueryConf define
   */
   struct coordQueryConf
   {
      string      _realCLName ;        // for command as 'drop cl' and so on
      BOOLEAN     _updateAndGetCata ;  // update catalog before first get version

      BOOLEAN     _openEmptyContext ;  // open context without sel & orderby ...
      BOOLEAN     _allCataGroups ;     // send to all catalog info groups,
                                       // don't use query to filter
      BOOLEAN     _preRead ;           // enable pre-read for coord context

      coordQueryConf()
      {
         // don't change the default value
         _updateAndGetCata = FALSE ;
         _openEmptyContext = FALSE ;
         _allCataGroups    = FALSE ;
         _preRead          = TRUE ;
      }
   } ;

   /*
      _coordQueryOperator define
   */
   class _coordQueryOperator : public _coordTransOperator
   {
      public:
         _coordQueryOperator( BOOLEAN readOnly = TRUE ) ;
         virtual ~_coordQueryOperator() ;

         virtual const CHAR* getName() const ;

         virtual BOOLEAN needRollback() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      public:

         INT32                queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord::sharePtr *pContext,
                                             coordSendOptions &sendOpt,
                                             coordQueryConf *pQueryConf = NULL,
                                             rtnContextBuf *buf = NULL ) ;

         INT32                queryOrDoOnCL( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextCoord::sharePtr *pContext,
                                             coordSendOptions &sendOpt,
                                             CoordGroupList &sucGrpLst,
                                             coordQueryConf *pQueryConf = NULL,
                                             rtnContextBuf *buf = NULL ) ;

         BOOLEAN              canPrepareTrans( pmdEDUCB *cb,
                                               const MsgHeader *pMsg ) const ;

      protected:

         virtual void         _prepareForTrans( pmdEDUCB *cb, MsgHeader *pMsg ) ;

         virtual BOOLEAN      _canPrepareTrans( pmdEDUCB *cb,
                                                const MsgHeader *pMsg ) const ;

         virtual BOOLEAN      _isTrans( pmdEDUCB *cb, MsgHeader *pMsg ) ;

         virtual void         _onNodeReply( INT32 processType,
                                                  MsgOpReply *pReply,
                                                  pmdEDUCB *cb,
                                                  coordSendMsgIn &inMsg ) ;

         virtual BOOLEAN      _canPushDownAutoCommit( coordSendMsgIn &inMsg,
                                                      coordSendOptions &options,
                                                      pmdEDUCB *cb ) const ;


         INT32                _queryOrDoOnCL( MsgHeader *pMsg,
                                              pmdEDUCB *cb,
                                              rtnContextCoord::sharePtr *pContext,
                                              coordSendOptions &sendOpt,
                                              CoordGroupList *pSucGrpLst = NULL,
                                              coordQueryConf *pQueryConf = NULL,
                                              rtnContextBuf *buf = NULL ) ;

      private:

         INT32    _buildNewMsg( const CHAR *msg,
                                const BSONObj *newSelector,
                                const BSONObj *newHint,
                                CHAR *&newMsg,
                                INT32 &buffSize,
                                IExecutor *cb ) ;

         BSONObj  _buildNewQuery( const BSONObj &query,
                                  const CoordSubCLlist &subCLList ) ;

         INT32    _checkQueryModify( coordSendMsgIn &inMsg,
                                     coordSendOptions &options,
                                     CoordGroupSubCLMap *grpSubCl ) ;

         void     _optimize( coordSendMsgIn &inMsg,
                             coordSendOptions &options,
                             coordProcessResult &result ) ;

         BOOLEAN  _isUpdate( const BSONObj &hint, INT32 flags ) ;

         INT32    _generateNewHint( const CoordCataInfoPtr &cataInfo,
                                    const BSONObj &matcher,
                                    const BSONObj &hint,
                                    BSONObj &newHint,
                                    BOOLEAN &isChanged,
                                    BOOLEAN &isEmpty,
                                    pmdEDUCB *cb,
                                    BOOLEAN keepShardingKey ) ;

         void     _clearBlock( pmdEDUCB *cb ) ;

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

      private:
         rtnContextCoord::sharePtr  _pContext ;
         INT32                      _processRet ;
         vector<CHAR*>              _vecBlock ;

         BOOLEAN                    _needRollback ;

   } ;
   typedef _coordQueryOperator coordQueryOperator ;

}

#endif // COORD_QUERY_OPERATOR_HPP__

