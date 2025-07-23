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

   // invalid redirect ID
   // which means the session has no redirected message
   #define STP_INVALID_REDIRECT_ID ( 0 )

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

      // get thread ID
      OSS_INLINE UINT32 getTID() const
      {
         SDB_ASSERT( NULL != _pEDUCB, "edu CB is invalid" ) ;
         return NULL != _pEDUCB ? _pEDUCB->getTID() : 0L ;
      }

      // set redirect ID
      OSS_INLINE void setRedirectID( UINT64 redirectID )
      {
         _redirectID = redirectID ;
      }

      // get redirect ID
      OSS_INLINE UINT64 getRedirectID() const
      {
         return _redirectID ;
      }

      // post message to this session
      INT32 postMessage( MsgHeader *message ) ;

      // hold in session
      OSS_INLINE void holdIn()
      {
         _holdIn() ;
      }

      // hold out session
      OSS_INLINE void holdOut()
      {
         _holdOut() ;
      }

   protected:
      // default message handle function to process with unknown messages
      virtual INT32 _defaultMsgFunc( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      // handle authentication request
      INT32 _handleAuthReq( NET_HANDLE handle, MsgHeader *message ) ;
      // handle query request ( including commands )
      INT32 _handleQueryReq( NET_HANDLE handle, MsgHeader *message ) ;
      // handle query result
      INT32 _handleQueryRes( NET_HANDLE handle, MsgHeader *message ) ;
      // send reply with results
      INT32 _sendReply( MsgOpReply *reply, const CHAR *body, UINT32 bodySize ) ;
      // send reply with return code
      INT32 _sendReply( MsgHeader *message, INT32 returnCode ) ;
      // send reply with return code and result in BSON format
      INT32 _sendReply( MsgHeader *message,
                        INT32 returnCode,
                        const bson::BSONObj &result ) ;
      INT32 _sendReply( MsgHeader *message ) ;

      // redirect request to primary
      INT32 _redirectPrimary( MsgHeader *message ) ;

   protected:
      // pointer to STP control block
      STPCB *     _stpCB ;

      // save redirect ID which means this session has message redirected to
      // other nodes
      UINT64      _redirectID ;
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

      // handle result for redirected session
      INT32 handleRedirectRes( MsgHeader *message ) ;
      // register a session with redirected message
      INT32 regRedirectSess( stpSession *session,
                             MsgHeader *message,
                             UINT64 &redirectID ) ;
      // unregister session with redirected message
      // means the session has received result for redirected message
      void  unregRedirectSess( UINT64 redirectID ) ;
      // get session by redirect ID
      INT32 getRedirectSess( UINT64 redirectID, UINT64 &sessionID ) ;
      // compact thread ID and request ID to redirect ID
      UINT64 makeRedirectID( UINT32 threadID, UINT32 requestID ) ;

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

      virtual INT32 _getSession( UINT64 sessionID,
                                 stpSession **session ) ;

   protected:
      typedef ossPoolMap< UINT32, UINT64 > STP_SESSION_MAP ;

      // pointer to STP control block
      STPCB *           _stpCB ;
      // latch to redirected session map
      ossSpinSLatch     _redLatch ;
      // allocator to redirect request ID
      UINT64            _curRedReqID ;
      // map to save redirected sessions
      STP_SESSION_MAP   _redSessions ;
   } ;

   typedef class _stpSessionManager stpSessionManager ;

}

#endif // STP_SESSION_HPP__
