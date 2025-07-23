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

   Source File Name = pmdAsyncHandler.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          1/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_ASYNC_HANDLER_HPP_
#define PMD_ASYNC_HANDLER_HPP_

#include "netTimer.hpp"
#include "netMsgHandler.hpp"
#include "pmdEDU.hpp"
#include "schedTaskAdapterBase.hpp"
#if defined ( SDB_ENGINE )
#include "pmdRemoteSession.hpp"
#endif

namespace engine
{
   class _pmdAsycSessionMgr ;

   /*
      _pmdAsyncTimerHandler define
   */
   class _pmdAsyncTimerHandler : public _netTimeoutHandler
   {
      public:
         _pmdAsyncTimerHandler ( _pmdAsycSessionMgr * pSessionMgr ) ;
         virtual ~_pmdAsyncTimerHandler () ;

         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) ;

      public:
         OSS_INLINE void attach ( pmdEDUCB *cb ) { _pMgrCB = cb ; }
         OSS_INLINE void detach () { _pMgrCB = NULL ; }

      protected:
         virtual UINT64  _makeTimerID( UINT32 timerID ) ;

      protected:
         pmdEDUCB             *_pMgrCB ;
         _pmdAsycSessionMgr   *_pSessionMgr ;

   } ;
   typedef _pmdAsyncTimerHandler pmdAsyncTimerHandler ;

   /*
      _pmdAsyncMsgHandler define
   */
   class _pmdAsyncMsgHandler : public _netMsgHandler
   {
      public:
   #if defined ( SDB_ENGINE )
         _pmdAsyncMsgHandler( _pmdAsycSessionMgr *pSessionMgr,
                              _schedTaskAdapterBase *pTaskAdapter = NULL,
                              _pmdRemoteSessionMgr *pRemoteSessionMgr = NULL ) ;
   #else
         _pmdAsyncMsgHandler( _pmdAsycSessionMgr *pSessionMgr,
                              _schedTaskAdapterBase *pTaskAdapter = NULL ) ;
   #endif
         virtual ~_pmdAsyncMsgHandler () ;

         OSS_INLINE void attach( pmdEDUCB *cb ) { _pMgrEDUCB = cb; }
         OSS_INLINE void detach() { _pMgrEDUCB = NULL; }

         virtual INT32 handleMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg );
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;

         virtual INT32 handleConnect( const NET_HANDLE &handle,
                                      _MsgRouteID id,
                                      BOOLEAN isPositive ) ;

         virtual void  onPrepareStop() ;

         virtual void  onStop() ;

      protected:
         void* _copyMsg ( const CHAR* msg, UINT32 length,
                          pmdEDUMemTypes &memType ) ;

         INT32 _handleSessionMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg );

         INT32 _handleAdapterMsg( const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg ) ;

         INT32 _handleMainMsg( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg ) ;

         INT32 _handleSysInfo( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg ) ;

      protected:
         virtual void _postMainMsg( const NET_HANDLE &handle,
                                    MsgHeader *pNewMsg,
                                    pmdEDUMemTypes memType ) ;

      protected:
         _pmdAsycSessionMgr      *_pSessionMgr ;
         _schedTaskAdapterBase   *_pTaskAdapter ;
         pmdEDUCB                *_pMgrEDUCB ;

   #if defined ( SDB_ENGINE )
         _pmdRemoteSessionMgr    *_pRemoteSessionMgr ;
   #endif

   } ;
   typedef _pmdAsyncMsgHandler pmdAsyncMsgHandler ;

}

#endif //PMD_ASYNC_HANDLER_HPP_

