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

   Source File Name = clsMsgHandler.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_MSG_HANDLER_HPP_
#define CLS_MSG_HANDLER_HPP_

#include "clsBase.hpp"
#include "pmdAsyncHandler.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"
#include "ossMemPool.hpp"
#include <vector>

namespace engine
{
   class _pmdAsycSessionMgr ;

   /*
      _shdMsgHandler define
   */
   class _shdMsgHandler : public _pmdAsyncMsgHandler
   {
      typedef ossPoolSet< ossEvent* >              SET_EVENTS ;
      typedef ossPoolMap< NET_HANDLE, SET_EVENTS > MAP_NET_2_EVENTS ;
      typedef MAP_NET_2_EVENTS::iterator           MAP_NET_2_EVENTS_IT ;

      public:
         _shdMsgHandler( _pmdAsycSessionMgr *pSessionMgr,
                         _schedTaskAdapterBase *pTaskAdapter = NULL,
                         _pmdRemoteSessionMgr *pRemoteSessionMgr = NULL ) ;
         virtual ~_shdMsgHandler();

         OSS_INLINE void attachShardCB( pmdEDUCB *cb ) { _pShardCB = cb ; }
         OSS_INLINE void detachShardCB() { _pShardCB = NULL ; }

         virtual void  handleClose( const NET_HANDLE &handle,
                                    _MsgRouteID id ) ;

      protected:
         virtual void _postMainMsg( const NET_HANDLE &handle,
                                    MsgHeader *pNewMsg,
                                    pmdEDUMemTypes memType ) ;

      protected:
         pmdEDUCB             *_pShardCB ;

   } ;
   typedef _shdMsgHandler shdMsgHandler ;

   /*
      _replMsgHandler define
   */
   class _replMsgHandler : public _pmdAsyncMsgHandler
   {
      public:
         _replMsgHandler ( _pmdAsycSessionMgr *pSessionMgr ) ;
         virtual ~_replMsgHandler () ;

         INT32 type () const ;

      protected:
         virtual void _postMainMsg( const NET_HANDLE &handle,
                                    MsgHeader *pNewMsg,
                                    pmdEDUMemTypes memType ) ;

   } ;
   typedef _replMsgHandler replMsgHandler ;

}

#endif //CLS_MSG_HANDLER_HPP_

