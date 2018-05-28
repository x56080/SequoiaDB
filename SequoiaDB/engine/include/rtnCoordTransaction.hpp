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
