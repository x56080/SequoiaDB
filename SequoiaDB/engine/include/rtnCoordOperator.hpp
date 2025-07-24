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

   Source File Name = rtnCoordOperator.hpp

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
#ifndef RTNCOORDOPERATOR_HPP__
#define RTNCOORDOPERATOR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "msg.hpp"
#include "coordCB.hpp"
#include "rtnCoordCommon.hpp"
#include "msgCatalog.hpp"
#include "../bson/bson.h"
#include <set>

using namespace bson ;
using namespace std ;

namespace engine
{

   enum RTN_MSG_IN_PRIVATE_DATA_TYPE
   {
      PRIVATE_DATA_NULL             = 0,
      PRIVATE_DATA_BSON             = 1,
      PRIVATE_DATA_NUMBERLONG       = 2,
      PRIVATE_DATA_COORDCONTEXT     = 3,

      PRIVATE_DATA_USER,
      PRIVATE_DATA_MAX
   } ;

   enum RTN_REPLY_PROCESS_TYPE
   {
      RTN_PROCESS_OK                = 0,
      RTN_PROCESS_IGNORE,
      RTN_PROCESS_NOK
   } ;

   struct rtnSendMsgIn
   {
      MsgHeader                  *_pMsg ;
      GROUP_2_IOVEC              _datas ;

      // private data
      INT32                      _pvtType ;
      CHAR                       *_pvtData ;

      rtnSendMsgIn( MsgHeader *msg )
      {
         _pMsg    = msg ;
         _pvtType = PRIVATE_DATA_NULL ;
         _pvtData = NULL ;
      }

      MsgHeader *msg()
      {
         return _pMsg ;
      }
      GROUP_2_IOVEC *data()
      {
         return &_datas ;
      }
      BOOLEAN hasData() const
      {
         if ( _datas.size() > 0 )
         {
            return TRUE ;
         }
         return FALSE ;
      }
      void resetHeader( pmdEDUCB *cb )
      {
         _pMsg->routeID.value = MSG_INVALID_ROUTEID ;
         _pMsg->TID = cb->getTID() ;
      }
      INT32 opCode()
      {
         return _pMsg->opCode ;
      }
   } ;

   struct rtnSendOptions
   {
      BOOLEAN                    _primary ;
      MSG_ROUTE_SERVICE_TYPE     _svcType ;
      UINT32                     _retryTimes ;

      CoordGroupList             _groupLst ;
      BOOLEAN                    _useSpecialGrp ;  // use the specila group of _groupLst
      CoordGroupMap              _mapGroupInfo ;
      SET_RC                     *_pIgnoreRC ;

      rtnSendOptions( BOOLEAN primary = FALSE,
                      MSG_ROUTE_SERVICE_TYPE type = MSG_ROUTE_SHARD_SERVCIE )
      {
         _primary    = primary ;
         _svcType    = type ;
         _retryTimes = 0 ;
         _pIgnoreRC  = NULL ;
         _useSpecialGrp = FALSE ;
      }

      BOOLEAN isIgnored( INT32 rc )
      {
         if ( _pIgnoreRC && _pIgnoreRC->find( rc ) != _pIgnoreRC->end() )
         {
            return TRUE ;
         }
         return FALSE ;
      }
   } ;

   struct rtnProcessResult
   {
      CoordGroupList             _sucGroupLst ;
      ROUTE_RC_MAP               *_pNokRC ;
      ROUTE_RC_MAP               *_pIgnoreRC ;
      ROUTE_RC_MAP               *_pOkRC ;

      ROUTE_REPLY_MAP            *_pNokReply ;
      ROUTE_REPLY_MAP            *_pIgnoreReply ;
      ROUTE_REPLY_MAP            *_pOkReply ;

      rtnProcessResult()
      {
         _pNokRC           = NULL ;
         _pIgnoreRC        = NULL ;
         _pOkRC            = NULL ;

         _pNokReply        = NULL ;
         _pIgnoreReply     = NULL ;
         _pOkReply         = NULL ;
      }

      BOOLEAN pushNokRC( UINT64 id, const MsgOpReply *pReply )
      {
         if ( _pNokRC )
         {
            (*_pNokRC)[ id ] = coordErrorInfo( pReply ) ;
            return TRUE ;
         }
         return FALSE ;
      }
      BOOLEAN pushOkRC( UINT64 id, INT32 rc )
      {
         if ( _pOkRC )
         {
            (*_pOkRC)[ id ] = rc ;
            return TRUE ;
         }
         return FALSE ;
      }
      BOOLEAN pushIgnoreRC( UINT64 id, INT32 rc )
      {
         if ( _pIgnoreRC )
         {
            (*_pIgnoreRC)[ id ] = rc ;
            return TRUE ;
         }
         return FALSE ;
      }
      BOOLEAN pushNokReply( UINT64 id, MsgHeader *pMsg )
      {
         if ( _pNokReply )
         {
            return _pNokReply->insert( std::make_pair( id, pMsg ) ).second ;
         }
         return FALSE ;
      }
      BOOLEAN pushIgnoreReply( UINT64 id, MsgHeader *pMsg )
      {
         if ( _pIgnoreReply )
         {
            return _pIgnoreReply->insert( std::make_pair( id, pMsg ) ).second ;
         }
         return FALSE ;
      }
      BOOLEAN pushOkReply( UINT64 id, MsgHeader *pMsg )
      {
         if ( _pOkReply )
         {
            return _pOkReply->insert( std::make_pair( id, pMsg ) ).second ;
         }
         return FALSE ;
      }
      BOOLEAN pushReply( MsgHeader *pMsg, INT32 processType )
      {
         if ( RTN_PROCESS_OK == processType )
         {
            return pushOkReply( pMsg->routeID.value, pMsg ) ;
         }
         else if ( RTN_PROCESS_IGNORE == processType )
         {
            return pushIgnoreReply( pMsg->routeID.value, pMsg ) ;
         }
         else if ( RTN_PROCESS_NOK == processType )
         {
            return pushNokReply( pMsg->routeID.value, pMsg ) ;
         }
         return FALSE ;
      }
      UINT32 ignoreSize() const
      {
         return _pIgnoreRC ? (UINT32)_pIgnoreRC->size() : 0 ;
      }
      UINT32 nokSize() const
      {
         return _pNokRC ? (UINT32)_pNokRC->size() : 0 ;
      }
      UINT32 okSize() const
      {
         return _pOkRC ? (UINT32)_pOkRC->size() : 0 ;
      }
      BOOLEAN isAllOK() const
      {
         if ( 0 != nokSize() || 0 != ignoreSize() )
         {
            return FALSE ;
         }
         return TRUE ;
      }
      void clearError() ;
      void clear() ;

   } ;

   void rtnClearReplyQue( REPLY_QUE *pReply ) ;

   class rtnCoordOperator : public SDBObject
   {
      friend class rtnCoordProcesserFactory ;
   public:
      rtnCoordOperator() { _isReadonly = FALSE ; }
      virtual ~rtnCoordOperator(){}

      BOOLEAN  isReadonly() const { return _isReadonly ; }

   public:
      virtual INT32        execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf ) = 0 ;

   public:
      /*
         Do on groups, the reply msg can't include data, only the flag
      */
      virtual INT32        doOnGroups( rtnSendMsgIn &inMsg,
                                       rtnSendOptions &options,
                                       netMultiRouteAgent *pAgent,
                                       pmdEDUCB *cb,
                                       rtnProcessResult &result ) ;

      virtual INT32        doOpOnMainCL( CoordCataInfoPtr &cataInfo,
                                         const BSONObj &objMatch,
                                         rtnSendMsgIn &inMsg,
                                         rtnSendOptions &options,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB *cb,
                                         rtnProcessResult &result ) ;

      virtual INT32        doOpOnCL( CoordCataInfoPtr &cataInfo,
                                     const BSONObj &objMatch,
                                     rtnSendMsgIn &inMsg,
                                     rtnSendOptions &options,
                                     netMultiRouteAgent *pRouteAgent,
                                     pmdEDUCB *cb,
                                     rtnProcessResult &result ) ;

      /*
         Check retry for collection operation
      */
      virtual BOOLEAN      checkRetryForCLOpr( INT32 rc,
                                               ROUTE_RC_MAP *pRC,
                                               MsgHeader *pSrcMsg,
                                               UINT32 times,
                                               CoordCataInfoPtr &cataInfo,
                                               pmdEDUCB *cb,
                                               INT32 &errRC,
                                               MsgRouteID *pNodeID = NULL,
                                               BOOLEAN canUpdate = TRUE ) ;

      virtual BOOLEAN      needRollback() const ;

   protected:
      virtual BOOLEAN            _isResend( UINT32 times ) ;
      virtual BOOLEAN            _canRetry( UINT32 times ) ;
      virtual BOOLEAN            _isTrans( pmdEDUCB *cb, MsgHeader *pMsg ) ;

      virtual void               _onNodeReply( INT32 processType,
                                               MsgOpReply *pReply,
                                               pmdEDUCB *cb,
                                               rtnSendMsgIn &inMsg ) ;

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

   protected:
      /// only write in rtnCoordProcesserFactory
      BOOLEAN                    _isReadonly ;

   };

   class rtnCoordOperatorDefault : public rtnCoordOperator
   {
   public:
      virtual INT32        execute( MsgHeader *pMsg,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf ) ;
   };

   class rtnCoordTransOperator : public rtnCoordOperator
   {
   public:
      virtual INT32        doOnGroups( rtnSendMsgIn &inMsg,
                                       rtnSendOptions &options,
                                       netMultiRouteAgent *pAgent,
                                       pmdEDUCB *cb,
                                       rtnProcessResult &result ) ;

      virtual BOOLEAN      needRollback() const ;

      INT32                rollBack( pmdEDUCB *cb,
                                     netMultiRouteAgent *pAgent ) ;

   protected:
      /*
         Prepare for transaction msg, you can change the opCode for trans
      */
      virtual void               _prepareForTrans( pmdEDUCB *cb,
                                                   MsgHeader *pMsg ) = 0 ;

      virtual BOOLEAN            _isTrans( pmdEDUCB *cb, MsgHeader *pMsg ) ;

      virtual void               _onNodeReply( INT32 processType,
                                               MsgOpReply *pReply,
                                               pmdEDUCB *cb,
                                               rtnSendMsgIn &inMsg ) ;

   protected:
      INT32         buildTransSession( const CoordGroupList &groupLst,
                                       netMultiRouteAgent *pRouteAgent,
                                       pmdEDUCB *cb,
                                       ROUTE_RC_MAP &newNodeMap ) ;
 
      INT32         releaseTransSession( ROUTE_SET &nodes,
                                         netMultiRouteAgent *pRouteAgent,
                                         pmdEDUCB * cb  ) ;

   } ;

   class rtnCoordMsg : public rtnCoordOperator
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;


   class rtnCoordShardKicker : public SDBObject 
   {
      struct strContainner
      {
         const CHAR *_pStr ;
         strContainner( const CHAR *str )
         {
            _pStr = str ;
         }
         strContainner()
         {
            _pStr = NULL ;
         }
         bool operator<( const strContainner &right ) const
         {
            if ( !right._pStr )
            {
               return false ;
            }
            else if ( !_pStr )
            {
               return true ;
            }
            return ossStrcmp( _pStr, right._pStr ) < 0 ? true : false ;
         }
      } ;

      typedef set< strContainner >              SET_SHARDINGKEY ;

   public:
      rtnCoordShardKicker() ;
      ~rtnCoordShardKicker() ;

   public:
      INT32 kickShardingKey( const CoordCataInfoPtr &cataInfo,
                             const BSONObj &updator, BSONObj &newUpdator,
                             BOOLEAN &hasShardingKey ) ;
      INT32 kickShardingKeyForSubCL( const CoordSubCLlist &subCLList,
                                     const BSONObj &updator, 
                                     BSONObj &newUpdator,
                                     BOOLEAN &hasShardingKey, pmdEDUCB *cb ) ;

   protected:
      BOOLEAN     _isUpdateReplace( const BSONObj &updator ) ;
      UINT32      _addKeys( const BSONObj &objKey ) ;

   private:
      set< UINT32 >              _skSiteIDs ;
      SET_SHARDINGKEY            _setKeys ;

   } ;

}

#endif //RTNCOORDOPERATOR_HPP__

