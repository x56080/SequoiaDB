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
#ifndef __SEQUOIAFSMCSSESSIONMGR_HPP__
#define __SEQUOIAFSMCSSESSIONMGR_HPP__

#include "pmdAsyncSession.hpp"

using namespace engine;

#define MAX_CATCH_SIZE          (1000)

namespace sequoiafs
{
   //class sequoiaFSMCS;
   class _mcsSessionMgr : public _pmdAsycSessionMgr
   {
      public:
         _mcsSessionMgr();
         virtual ~_mcsSessionMgr();

         virtual INT32        handleSessionTimeout(UINT32 timerID,
                                                   UINT32 interval);
         
         virtual UINT64       makeSessionID(const NET_HANDLE &handle,
                                            const MsgHeader *header);

         virtual INT32        onErrorHanding(INT32 rc,
                                             const MsgHeader *pReq,
                                             const NET_HANDLE &handle,
                                             UINT64 sessionID,
                                             pmdAsyncSession *pSession);
         
      protected:
         virtual SDB_SESSION_TYPE   _prepareCreate(UINT64 sessionID,
                                                   INT32 startType,
                                                   INT32 opCode );

         virtual BOOLEAN      _canReuse(SDB_SESSION_TYPE sessionType);
         virtual UINT32       _maxCacheSize() const ;
         
         virtual pmdAsyncSession*  _createSession( SDB_SESSION_TYPE sessionType,
                                                   INT32 startType,
                                                   UINT64 sessionID,
                                                   void *data = NULL);

      protected:
         //sequoiaFSMCS         *_mcsMgr;
         UINT32               _unShardSessionTimer ;
   } ;

}

#endif
