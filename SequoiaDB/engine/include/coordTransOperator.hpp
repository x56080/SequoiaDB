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

   Source File Name = coordTransOperator.hpp

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
#ifndef COORD_TRANS_OPERATOR_HPP__
#define COORD_TRANS_OPERATOR_HPP__

#include "coordOperator.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordTransOperator
   */
   class _coordTransOperator : public _coordOperator
   {
      public:
         _coordTransOperator() ;
         virtual ~_coordTransOperator() ;

         virtual BOOLEAN      needRollback() const ;

      public:
         virtual INT32        doOnGroups( coordSendMsgIn &inMsg,
                                          coordSendOptions &options,
                                          pmdEDUCB *cb,
                                          coordProcessResult &result ) ;

         virtual INT32        rollback( pmdEDUCB *cb ) ;

      protected:
         /*
            Prepare for transaction msg, you can change the opCode for trans
         */
         virtual void               _prepareForTrans( pmdEDUCB *cb,
                                                      MsgHeader *pMsg ) = 0 ;

         virtual BOOLEAN            _canPrepareTrans( pmdEDUCB *cb,
                                                      const MsgHeader *pMsg ) const ;

         virtual BOOLEAN            _isTrans( pmdEDUCB *cb, MsgHeader *pMsg ) ;

         virtual void               _onNodeReply( INT32 processType,
                                                  MsgOpReply *pReply,
                                                  pmdEDUCB *cb,
                                                  coordSendMsgIn &inMsg ) ;

         virtual BOOLEAN            _canPushDownAutoCommit( coordSendMsgIn &inMsg,
                                                            coordSendOptions &options,
                                                            pmdEDUCB *cb ) const ;

      protected:
         INT32          buildTransSession( const CoordGroupList &groupLst,
                                           pmdEDUCB *cb,
                                           ROUTE_RC_MAP &newNodeMap ) ;
    
         INT32          releaseTransSession( SET_NODEID &nodes,
                                             pmdEDUCB *cb  ) ;

      protected:
         UINT64         _recvNum ;

   } ;
   typedef _coordTransOperator coordTransOperator ;

   /*
      _coordTransBegin define
   */
   class _coordTransBegin : public _coordOperator
   {
      public:
         _coordTransBegin() ;
         virtual ~_coordTransBegin() ;

         virtual const CHAR* getName() const ;

         INT32         beginTrans( pmdEDUCB *cb,
                                   BOOLEAN isAutoCommit = FALSE ) ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordTransBegin coordTransBegin ;

   /*
      _coord2PhaseCommit define
   */
   class _coord2PhaseCommit : public _coordOperator
   {
      public:
         _coord2PhaseCommit() ;
         virtual ~_coord2PhaseCommit() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      private:
         virtual INT32 doPhase1( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;

         virtual INT32 doPhase2( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;

         virtual INT32 doCompactPhase( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

         virtual INT32 cancelOp( MsgHeader *pMsg,
                                 pmdEDUCB *cb,
                                 INT64 &contextID,
                                 rtnContextBuf *buf ) ;

         virtual INT32 buildPhase1Msg( const CHAR *pReceiveBuffer,
                                       CHAR **pMsg,
                                       INT32 *pMsgSize,
                                       pmdEDUCB *cb ) = 0 ;

         virtual INT32 buildPhase2Msg( const CHAR *pReceiveBuffer,
                                       CHAR **pMsg,
                                       INT32 *pMsgSize,
                                       pmdEDUCB *cb ) = 0 ;

         virtual void  releasePhase1Msg( CHAR *pMsg,
                                         INT32 msgSize,
                                         pmdEDUCB *cb ) = 0 ;

         virtual void  releasePhase2Msg( CHAR *pMsg,
                                         INT32 msgSize,
                                         pmdEDUCB *cb ) = 0 ;

         virtual INT32 executeOnDataGroup ( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf ) = 0 ;

         virtual BOOLEAN canCompactCommit() = 0 ;

         virtual INT32   buildCompactMsg( const CHAR *pReceiveBuffer,
                                          CHAR **pMsg,
                                          INT32 *pMsgSize,
                                          pmdEDUCB *cb ) = 0 ;

         virtual void    releaseCompactMsg( CHAR *pMsg,
                                            INT32 msgSize,
                                            pmdEDUCB *cb ) = 0 ;
   } ;
   typedef _coord2PhaseCommit coord2PhaseCommit ;

   /*
      _coordTransCommit define
   */
   class _coordTransCommit : public _coord2PhaseCommit
   {
      public:
         _coordTransCommit() ;
         virtual ~_coordTransCommit() ;

         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual BOOLEAN      needRollback() const ;

      private:
         virtual INT32 buildPhase1Msg( const CHAR *pReceiveBuffer,
                                       CHAR **pMsg,
                                       INT32 *pMsgSize,
                                       pmdEDUCB *cb ) ;

         virtual INT32 buildPhase2Msg( const CHAR *pReceiveBuffer,
                                       CHAR **pMsg,
                                       INT32 *pMsgSize,
                                       pmdEDUCB *cb ) ;

         virtual void  releasePhase1Msg( CHAR *pMsg,
                                         INT32 msgSize,
                                         pmdEDUCB *cb ) ;

         virtual void  releasePhase2Msg( CHAR *pMsg,
                                         INT32 msgSize,
                                         pmdEDUCB *cb ) ;

         virtual INT32 executeOnDataGroup ( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf ) ;

         virtual BOOLEAN canCompactCommit() ;

         virtual INT32   buildCompactMsg( const CHAR *pReceiveBuffer,
                                          CHAR **pMsg,
                                          INT32 *pMsgSize,
                                          pmdEDUCB *cb ) ;

         virtual void    releaseCompactMsg( CHAR *pMsg,
                                            INT32 msgSize,
                                            pmdEDUCB *cb ) ;

      private:
         MsgOpTransCommit                 _phase2Msg ;
         
   } ;
   typedef _coordTransCommit coordTransCommit ;

   /*
      _coordTransRollback define
   */
   class _coordTransRollback : public _coordTransOperator
   {
      public:
         _coordTransRollback() ;
         virtual ~_coordTransRollback() ;

         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual BOOLEAN      needRollback() const ;

      protected:

         virtual void         _prepareForTrans( pmdEDUCB *cb,
                                                MsgHeader *pMsg ) ;

   } ;
   typedef _coordTransRollback coordTransRollback ;

}

#endif //COORD_TRANS_OPERATOR_HPP__

