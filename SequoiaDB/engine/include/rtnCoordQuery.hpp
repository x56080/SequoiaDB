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

   Source File Name = rtnCoordQuery.hpp

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
#ifndef RTNCOORDQUERY_HPP__
#define RTNCOORDQUERY_HPP__

#include "rtnCoordOperator.hpp"
#include "rtnContext.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   struct rtnQueryConf
   {
      string      _realCLName ;        // for command as 'drop cl' and so on
      BOOLEAN     _updateAndGetCata ;  // update catalog before first get version

      BOOLEAN     _openEmptyContext ;  // open context without sel & orderby ...
      BOOLEAN     _allCataGroups ;     // send to all catalog info groups,
                                       // don't use query to filter

      rtnQueryConf()
      {
         // don't change the default value
         _updateAndGetCata = FALSE ;
         _openEmptyContext = FALSE ;
         _allCataGroups    = FALSE ;
      }
   } ;

   class rtnCoordQuery : virtual public rtnCoordOperator
   {
   public:
      struct rtnQueryPvtData
      {
         INT32                   _ret ;
         rtnContextCoord         *_pContext ;

         rtnQueryPvtData()
         {
            _ret        = SDB_OK ;
            _pContext   = NULL ;
         }
      } ;
   public:
      rtnCoordQuery() { _isReadonly = TRUE ; }
      rtnCoordQuery( BOOLEAN isReadOnly ) { _isReadonly = isReadOnly ; }

      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

      INT32                queryOrDoOnCL( MsgHeader *pMsg,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          rtnContextCoord **pContext,
                                          rtnSendOptions &sendOpt,
                                          rtnQueryConf *pQueryConf = NULL ) ;

      INT32                queryOrDoOnCL( MsgHeader *pMsg,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          rtnContextCoord **pContext,
                                          rtnSendOptions &sendOpt,
                                          CoordGroupList &sucGrpLst,
                                          rtnQueryConf *pQueryConf = NULL ) ;

   protected:
      INT32                _queryOrDoOnCL( MsgHeader *pMsg,
                                           netMultiRouteAgent *pRouteAgent,
                                           pmdEDUCB *cb,
                                           rtnContextCoord **pContext,
                                           rtnSendOptions &sendOpt,
                                           CoordGroupList *pSucGrpLst = NULL,
                                           rtnQueryConf *pQueryConf = NULL ) ;

   private:

      INT32 _buildNewMsg( const CHAR *msg,
                          const BSONObj *newSelector,
                          const BSONObj *newHint,
                          CHAR *&newMsg,
                          INT32 &buffSize ) ;

      BSONObj _buildNewQuery( const BSONObj &query,
                              const CoordSubCLlist &subCLList ) ;

      INT32 _checkQueryModify( rtnSendMsgIn &inMsg,
                               rtnSendOptions &options,
                               CoordGroupSubCLMap *grpSubCl ) ;

      void  _optimize( rtnSendMsgIn &inMsg,
                       rtnSendOptions &options,
                       rtnProcessResult &result ) ;

      BOOLEAN _isUpdate( const BSONObj &hint, INT32 flags ) ;

      INT32 _generateNewHint( const CoordCataInfoPtr &cataInfo,
                              const BSONObj &selector,
                              const BSONObj &hint, BSONObj &newHint,
                              BOOLEAN &isChanged, BOOLEAN &isEmpty,
                              pmdEDUCB *cb ) ;

      INT32 _generateShardUpdator( const CoordCataInfoPtr &cataInfo,
                                   const BSONObj &selector,
                                   const BSONObj &updator, BSONObj &newUpdator,
                                   BOOLEAN &isChanged, pmdEDUCB *cb ) ;

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

   private:
      /// can't define members, because it's single instance

   };

}

#endif

