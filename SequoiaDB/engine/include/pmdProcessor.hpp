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

   Source File Name = pmdProcessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/12/2014  Lin Youbin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_PROCESSOR_HPP_
#define PMD_PROCESSOR_HPP_

#include "pmd.hpp"
#include "rtnCB.hpp"
#include "dmsCB.hpp"
#include "dpsLogWrapper.hpp"
#include "pmdSession.hpp"

namespace engine
{
   /*
      _pmdDataProcessor define
   */
   class _pmdDataProcessor : public _pmdProcessorBase
   {
      public:
         _pmdDataProcessor() ;
         virtual ~_pmdDataProcessor() ;

      public:
         virtual INT32           processMsg( MsgHeader *msg,
                                             rtnContextBuf &contextBuff,
                                             INT64 &contextID,
                                             BOOLEAN &needReply,
                                             BOOLEAN &needRollback,
                                             BSONObjBuilder &builder ) ;

         virtual INT32           doRollback() ;
         virtual INT32           doCommit() ;

         virtual const CHAR*           processorName() const ;
         virtual SDB_PROCESSOR_TYPE    processorType() const ;

      protected:
         virtual void                  _onAttach () ;
         virtual void                  _onDetach () ;

      protected:
         INT32                   _onMsgReqMsg( MsgHeader * msg ) ;
         INT32                   _onUpdateReqMsg( MsgHeader * msg,
                                                  SDB_DPSCB *dpsCB,
                                                  utilUpdateResult &upResult ) ;
         INT32                   _onInsertReqMsg( MsgHeader * msg,
                                                  rtnContextBuf &buff,
                                                  utilInsertResult &inResult ) ;
         INT32                   _onQueryReqMsg( MsgHeader * msg,
                                                 SDB_DPSCB *dpsCB,
                                                 _rtnContextBuf &buffObj,
                                                 INT64 &contextID,
                                                 BOOLEAN &needRollback,
                                                 BSONObjBuilder &builder ) ;
         INT32                   _onDelReqMsg( MsgHeader * msg,
                                               SDB_DPSCB *dpsCB,
                                               utilDeleteResult &delResult ) ;
         INT32                   _onGetMoreReqMsg( MsgHeader * msg,
                                                   rtnContextBuf &buffObj,
                                                   INT64 &contextID,
                                                   BOOLEAN &needRollback ) ;
         INT32                   _onKillContextsReqMsg( MsgHeader *msg ) ;
         INT32                   _onSQLMsg( MsgHeader *msg,
                                            INT64 &contextID,
                                            SDB_DPSCB *dpsCB,
                                            BOOLEAN &needRollback,
                                            BSONObjBuilder &builder ) ;
         INT32                   _onTransBeginMsg () ;
         INT32                   _onTransCommitMsg ( SDB_DPSCB *dpsCB ) ;
         INT32                   _onTransRollbackMsg ( SDB_DPSCB *dpsCB ) ;
         INT32                   _onAggrReqMsg( MsgHeader *msg,
                                                INT64 &contextID ) ;
         INT32                   _onOpenLobMsg( MsgHeader *msg,
                                                SDB_DPSCB *dpsCB,
                                                SINT64 &contextID,
                                                rtnContextBuf &buffObj ) ;
         INT32                   _onWriteLobMsg( MsgHeader *msg ) ;
         INT32                   _onReadLobMsg( MsgHeader *msg,
                                                rtnContextBuf &buffObj ) ;
         INT32                   _onLockLobMsg( MsgHeader *msg ) ;
         INT32                   _onCloseLobMsg( MsgHeader *msg,
                                                 rtnContextBuf &buffObj ) ;
         INT32                   _onRemoveLobMsg( MsgHeader *msg,
                                                  SDB_DPSCB *dpsCB ) ;
         INT32                   _onTruncateLobMsg( MsgHeader *msg,
                                                    SDB_DPSCB *dpsCB ) ;
         INT32                   _onGetLobRTDetailMsg( MsgHeader *msg,
                                                     rtnContextBuf &buffObj ) ;
         INT32                   _onCreateLobIDMsg( MsgHeader *msg,
                                                    rtnContextBuf &buffObj ) ;
         INT32                   _onInterruptMsg( MsgHeader *msg,
                                                  SDB_DPSCB *dpsCB ) ;
         INT32                   _onInterruptSelfMsg() ;
         INT32                   _onDisconnectMsg() ;

         INT32                   _updateVCS( const CHAR *fullName,
                                             const BSONObj &updator ) ;

         INT32                   _insertVCS( const CHAR *fullName,
                                             const BSONObj &insertor ) ;

         INT32                   _deleteVCS( const CHAR *fullName,
                                             const BSONObj &deletor ) ;

      protected:
         INT32                   _checkTransOperator( BOOLEAN checkDps = TRUE,
                                                      BOOLEAN autoCommit = FALSE ) ;
         INT32                   _checkTransAutoCommit( const MsgHeader *msg,
                                                        BOOLEAN checkDps = TRUE ) ;

      protected:
         virtual INT32           _beginTrans( BOOLEAN isAutoCommit ) ;

      protected:
         _SDB_KRCB *             _pKrcb ;
         _SDB_DMSCB *            _pDMSCB ;
         _SDB_RTNCB *            _pRTNCB ;
   } ;
   typedef _pmdDataProcessor pmdDataProcessor ;

   /*
      _pmdCoordProcessor define
   */
   class _pmdCoordProcessor : public _pmdDataProcessor
   {
      public:
         _pmdCoordProcessor() ;
         virtual ~_pmdCoordProcessor() ;

      public:
         virtual INT32           processMsg( MsgHeader *msg,
                                             rtnContextBuf &contextBuff,
                                             INT64 &contextID,
                                             BOOLEAN &needReply,
                                             BOOLEAN &needRollback,
                                             BSONObjBuilder &builder ) ;

         virtual INT32           doRollback() ;
         virtual INT32           doCommit() ;

         virtual const CHAR*           processorName() const ;
         virtual SDB_PROCESSOR_TYPE    processorType() const ;

      protected:
         virtual INT32           _beginTrans( BOOLEAN isAutoCommit ) ;

      protected:
         virtual void                  _onAttach () ;
         virtual void                  _onDetach () ;

         INT32                   _onQueryReqMsg( MsgHeader *msg,
                                                 _rtnContextBuf &buffObj,
                                                 INT64 &contextID,
                                                 BOOLEAN &needRollback ) ;

      private:
         INT32                   _processCoordMsg( MsgHeader *msg,
                                                   INT64 &contextID,
                                                   rtnContextBuf &contextBuff,
                                                   BOOLEAN &needReply,
                                                   BOOLEAN &needRollback ) ;
   } ;

   typedef _pmdCoordProcessor pmdCoordProcessor ;

   /*
    * Check if the database is in restore pending state, and
    * whether a msg is allowed to be processed. Only used in
    * the coord right now.
    */
   class pmdRestorePendingChecker
   {
      public:
         pmdRestorePendingChecker( MsgHeader *msg ) : _msg(msg) {}
         ~pmdRestorePendingChecker() {}

         // check if the operation is allowed to be processed
         BOOLEAN isOpAllowed() ;
      private:
         BOOLEAN _isOpAllowed() ;
         MsgHeader *_msg ;
   } ;
}

#endif  /*PMD_PROCESSOR_HPP_*/

