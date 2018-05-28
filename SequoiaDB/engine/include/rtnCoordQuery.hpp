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
                                          rtnQueryConf *pQueryConf = NULL,
                                          rtnContextBuf *buf = NULL ) ;

      INT32                queryOrDoOnCL( MsgHeader *pMsg,
                                          netMultiRouteAgent *pRouteAgent,
                                          pmdEDUCB *cb,
                                          rtnContextCoord **pContext,
                                          rtnSendOptions &sendOpt,
                                          CoordGroupList &sucGrpLst,
                                          rtnQueryConf *pQueryConf = NULL,
                                          rtnContextBuf *buf = NULL ) ;

   protected:
      INT32                _queryOrDoOnCL( MsgHeader *pMsg,
                                           netMultiRouteAgent *pRouteAgent,
                                           pmdEDUCB *cb,
                                           rtnContextCoord **pContext,
                                           rtnSendOptions &sendOpt,
                                           CoordGroupList *pSucGrpLst = NULL,
                                           rtnQueryConf *pQueryConf = NULL,
                                           rtnContextBuf *buf = NULL ) ;

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

