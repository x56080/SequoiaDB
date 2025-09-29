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

   Source File Name = sequoiaFSMsgHandler.hpp

   Descriptive Name = sequoiafs meta cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSHANDLER_HPP__
#define __SEQUOIAFSHANDLER_HPP__

#include "pmdAsyncHandler.hpp"
#include "netMsgHandler.hpp"
#include "sequoiaFSCommon.hpp"

using namespace engine;

#define FSTimer 1

namespace sequoiafs
{
   class fsMetaCache;
   class fsMsghandler : public _netMsgHandler
   {
      public:
         fsMsghandler(fsMetaCache* metaCache);
         virtual ~fsMsghandler();
         virtual INT32 handleMsg( const NET_HANDLE &handle,
                       const MsgHeader *header,
                       const CHAR *msg );
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id );
         INT32 handleRegRsp(_mcsRegRsp* response);
         INT32 handleNotify(_mcsNotifyReq* request);

      private:
         fsMetaCache* _metaCache;
   };

   class _fsTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _fsTimerHandler(_pmdAsycSessionMgr * pSessionMgr);
         virtual ~_fsTimerHandler();

      protected:
         virtual UINT64  _makeTimerID(UINT32 timerID);
   };
}

#endif
