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

   Source File Name = stpSession.hpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef STP_SESSION_HPP__
#define STP_SESSION_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "pmdAsyncSession.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      _stpSession define
    */
   // _stpSession is an asynchronous session for STP service
   class _stpSession : public pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      // constructor and destructor
      _stpSession( UINT64 sessionID, STPCB *stpCB ) ;
      ~_stpSession() ;

   public:
      // override functions of async session
      OSS_INLINE virtual SDB_SESSION_TYPE sessionType() const
      {
         return SDB_SESSION_STP ;
      }

      OSS_INLINE virtual const CHAR *className() const
      {
         return "STPSession" ;
      }

      OSS_INLINE virtual EDU_TYPES eduType() const
      {
         return EDU_TYPE_STP_SESSION ;
      }

   protected:
      // default message handle function to process with unknown messages
      virtual INT32 _defaultMsgFunc( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      // handle authentication request
      INT32 _handleAuthReq( NET_HANDLE handle, MsgHeader *message ) ;
      // handle query request ( including commands )
      INT32 _handleQueryReq( NET_HANDLE handle, MsgHeader *message ) ;
      // send reply with results
      INT32 _sendReply( MsgOpReply *reply, const CHAR *body, UINT32 bodySize ) ;
      // send reply with return code
      INT32 _sendReply( MsgHeader *message, INT32 returnCode ) ;
      // send reply with return code and result in BSON format
      INT32 _sendReply( MsgHeader *message,
                        INT32 returnCode,
                        const bson::BSONObj &result ) ;

   protected:
      // pointer to STP control block
      STPCB * _stpCB ;
   } ;

   typedef class _stpSession stpSession ;


   /*
      _stpSessionManager define
    */
   // _stpSessionManager manages asynchronous sessions for STP service
   class _stpSessionManager : public pmdAsycSessionMgr
   {
   public:
      // constructor and destructor
      _stpSessionManager( STPCB *stpCB ) ;
      virtual ~_stpSessionManager() ;

   public:
      // override functions for async session manager
      // make session ID
      virtual UINT64 makeSessionID( const NET_HANDLE &handle,
                                    const MsgHeader *header ) ;
      // handle error
      virtual INT32 onErrorHanding( INT32 rc,
                                    const MsgHeader *request,
                                    const NET_HANDLE &handle,
                                    UINT64 sessionID,
                                    pmdAsyncSession *session ) ;

   protected:
      // override protected functions for async session manager

      // on event before create session
      virtual SDB_SESSION_TYPE _prepareCreate( UINT64 sessionID,
                                               INT32 startType,
                                               INT32 opCode ) ;

      // indicate if we could reuse the session
      virtual BOOLEAN _canReuse( SDB_SESSION_TYPE sessionType ) ;

      // max cache size for idle session
      virtual UINT32  _maxCacheSize() const ;

      // create session
      // NOTE: data is not used
      virtual pmdAsyncSession * _createSession( SDB_SESSION_TYPE sessionType,
                                                INT32 startType,
                                                UINT64 sessionID,
                                                void *data = NULL ) ;

   protected:
      // pointer to STP control block
      STPCB * _stpCB ;
   } ;

   typedef class _stpSessionManager stpSessionManager ;

}

#endif // STP_SESSION_HPP__
