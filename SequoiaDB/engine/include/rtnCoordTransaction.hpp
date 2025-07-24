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

   Source File Name = rtnCoordTransaction.hpp

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
#ifndef RTNCOORDTRANSACTION_HPP__
#define RTNCOORDTRANSACTION_HPP__

#include "rtnCoordOperator.hpp"

namespace engine
{
   class rtnCoordTransBegin : public rtnCoordOperator
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   };

   class rtnCoord2PhaseCommit : public rtnCoordOperator
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:
      virtual INT32 doPhase1( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              INT64 &contextID,
                              rtnContextBuf *buf );

      virtual INT32 doPhase2( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              INT64 &contextID,
                              rtnContextBuf *buf );

      virtual INT32 cancelOp( MsgHeader *pMsg,
                              pmdEDUCB *cb,
                              INT64 &contextID,
                              rtnContextBuf *buf );

      virtual INT32 buildPhase1Msg( CHAR * pReceiveBuffer, CHAR **pMsg ) = 0;

      virtual INT32 buildPhase2Msg( CHAR * pReceiveBuffer, CHAR **pMsg ) = 0;

      virtual INT32 executeOnDataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf ) = 0;
   };

   class rtnCoordTransCommit : public rtnCoord2PhaseCommit
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

      virtual BOOLEAN      needRollback() const ;

   private:
      virtual INT32 buildPhase1Msg( CHAR * pReceiveBuffer, CHAR **pMsg );

      virtual INT32 buildPhase2Msg( CHAR * pReceiveBuffer, CHAR **pMsg );

      virtual INT32 executeOnDataGroup ( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         INT64 &contextID,
                                         rtnContextBuf *buf ) ;
   };

   class rtnCoordTransRollback : public rtnCoordTransOperator
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      virtual BOOLEAN      needRollback() const ;

      virtual void         _prepareForTrans( pmdEDUCB *cb,
                                             MsgHeader *pMsg ) ;

   };
}

#endif
