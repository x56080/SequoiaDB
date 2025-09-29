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

   Source File Name = sequoiaFSMCSMsgHandler.hpp

   Descriptive Name = sequoiafs meta cache service.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        01/07/2021  zyj  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSMCSHANDLER_HPP__
#define __SEQUOIAFSMCSHANDLER_HPP__

#include "pmdAsyncHandler.hpp"
#include "netMsgHandler.hpp"
#include "netDef.hpp"
#include "sequoiaFSCommon.hpp"

using namespace engine;

#define MCSTimer 1

namespace sequoiafs
{
   class sequoiaFSMCS;
   class mcsMsghandler : public _pmdAsyncMsgHandler
   {
      public:
         mcsMsghandler(_pmdAsycSessionMgr *pSessionMgr,
                       sequoiaFSMCS *mcs);
         virtual ~mcsMsghandler();
         
         virtual INT32 handleMsg(const NET_HANDLE &handle,
                       const _MsgHeader *header,
                       const CHAR *msg );
         virtual void  handleClose(const NET_HANDLE &handle, 
                            _MsgRouteID id );
         INT32 handleRegRequest(const NET_HANDLE &handle, 
                                const _mcsRegReq *request);
         INT32 handleNotifyRequest(const NET_HANDLE &handle, 
                                   const _mcsNotifyReq *request);

      private:
         sequoiaFSMCS *_mcs;
   };

   class _mcsTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _mcsTimerHandler (_pmdAsycSessionMgr * pSessionMgr);
         virtual ~_mcsTimerHandler();

      protected:
         virtual UINT64  _makeTimerID(UINT32 timerID);
   } ;
}

#endif
