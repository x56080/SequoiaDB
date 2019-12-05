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

   Source File Name = tpSession.hpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef TP_SESSION_HPP__
#define TP_SESSION_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "pmdAsyncSession.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      _tpSession define
    */
   class _tpSession : public pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _tpSession( UINT64 sessionID, SDB_TPCB *tpCB ) ;
      ~_tpSession() ;

   public:
      OSS_INLINE virtual SDB_SESSION_TYPE sessionType() const
      {
         return SDB_SESSION_TP ;
      }

      OSS_INLINE virtual const CHAR *className() const
      {
         return "TPSSession" ;
      }

      OSS_INLINE virtual EDU_TYPES eduType() const
      {
         return EDU_TYPE_TP_SESSION ;
      }

   protected:
      virtual INT32 _defaultMsgFunc( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      INT32 _handleAuthReq( NET_HANDLE handle, MsgHeader *message ) ;
      INT32 _handleQueryReq( NET_HANDLE handle, MsgHeader *message ) ;
      INT32 _sendReply( MsgOpReply *reply, const CHAR *body, UINT32 bodySize ) ;
      INT32 _sendReply( MsgHeader *message, INT32 returnCode ) ;
      INT32 _sendReply( MsgHeader *message,
                        INT32 returnCode,
                        const bson::BSONObj &result ) ;

   protected:
      SDB_TPCB * _tpCB ;
   } ;

   typedef class _tpSession tpSession ;


   /*
      _tpSessionManager define
    */
   class _tpSessionManager : public pmdAsycSessionMgr
   {
   public:
      _tpSessionManager( SDB_TPCB *tpCB ) ;
      virtual ~_tpSessionManager() ;

   public:
      virtual UINT64 makeSessionID( const NET_HANDLE &handle,
                                    const MsgHeader *header ) ;
      virtual INT32 onErrorHanding( INT32 rc,
                                    const MsgHeader *request,
                                    const NET_HANDLE &handle,
                                    UINT64 sessionID,
                                    pmdAsyncSession *session ) ;

   protected:
      virtual SDB_SESSION_TYPE _prepareCreate( UINT64 sessionID,
                                               INT32 startType,
                                               INT32 opCode ) ;
      virtual BOOLEAN _canReuse( SDB_SESSION_TYPE sessionType ) ;
      virtual UINT32  _maxCacheSize() const ;
      virtual pmdAsyncSession * _createSession( SDB_SESSION_TYPE sessionType,
                                                INT32 startType,
                                                UINT64 sessionID,
                                                void *data = NULL ) ;

   protected:
      SDB_TPCB * _tpCB ;
   } ;

   typedef class _tpSessionManager tpSessionManager ;

}

#endif // TP_SESSION_HPP__
