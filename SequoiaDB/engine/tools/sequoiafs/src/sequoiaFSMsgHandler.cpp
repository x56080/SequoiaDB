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

   Source File Name = sequoiaFSMsgHandler.cpp

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
#include "sequoiaFSMsgHandler.hpp"
#include "sequoiaFSRegister.hpp"
#include "sequoiaFSMetaCache.hpp"
#include "msg.h"

using namespace engine;
using namespace bson; 

namespace sequoiafs
{
   fsMsghandler::fsMsghandler(fsMetaCache* metaCache)
   :_metaCache(metaCache)
   {
   }

   fsMsghandler::~fsMsghandler()
   {
   }
   
   INT32 fsMsghandler::handleMsg(const NET_HANDLE &handle,
                                 const _MsgHeader *header,
                                 const CHAR *msg ) 
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT ( NULL != header, "message-header should not be NULL" ) ;

      switch (header->opCode)
      {
         case FS_REGISTER_RSP:
         {
            rc = handleRegRsp((_mcsRegRsp*)header); 
            if(rc != SDB_OK)
            {
               PD_LOG(PDERROR, "handleRegRsp failed. rc=[%d]", rc);
               goto error;
            }
            break;
         }
         case MS_RELEASELOCK:
         {
            //analyse release lock, get parentId, path
            rc = handleNotify((_mcsNotifyReq*)msg);
            if(rc != SDB_OK)
            {
               PD_LOG(PDERROR, "handleNotify failed. rc=%d", rc);
               goto error;
            }
            break;
         }
         default:
         {
            rc = SDB_UNKNOWN_MESSAGE;
            PD_LOG(PDERROR, "Unknown message [%d]", header->opCode);
            goto error;
            break;
         }
      }
      
   done:
      return rc;
   error :
      goto done ;
   }

   void fsMsghandler::handleClose(const NET_HANDLE &handle, _MsgRouteID id)
   {//TODO: 如果收到mcs的断开链接，立刻尝试一次重新注册
      _metaCache->getRegister()->setRegister(FALSE);
      _metaCache->getDirMetaCache()->releaseAllLocalCache(TRUE);
      PD_LOG(PDERROR, "The connection to MCS is broken.");
      _metaCache->getRegister()->registerToMCS();
   }

   INT32 fsMsghandler::handleRegRsp(_mcsRegRsp* response)
   {
      INT32 rc = SDB_OK;
      
      if(response->reply.res == SDB_OK)
      {
         _metaCache->getRegister()->setRegister(TRUE);
         PD_LOG(PDDEBUG, "register success.");
      }
      else
      {
         rc = SDB_INTERRUPT;
         PD_LOG(PDERROR, "Register to MCS failed [%d]", response->reply.res);
         goto error;
      }

   done:
      return rc;
   error :
      goto done ;
   }

   INT32 fsMsghandler::handleNotify(_mcsNotifyReq* request)
   {
      INT32 rc = SDB_OK;
      
      rc = _metaCache->getDirMetaCache()->releaseLocalDirCache(request->parentId, request->dirName);
      if(rc != SDB_OK)
      {
         PD_LOG(PDERROR, "releaseLocalDirCache failed. parentId:%d, dirName:%s. rc=%d", 
                          request->parentId, request->dirName, rc);
         goto error;
      }

   done:
      return rc;
   error :
      goto done ;   
   }

   _fsTimerHandler::_fsTimerHandler (_pmdAsycSessionMgr * pSessionMgr)
      :_pmdAsyncTimerHandler (pSessionMgr)
   {
   }

   _fsTimerHandler::~_fsTimerHandler()
   {
   }

   UINT64 _fsTimerHandler::_makeTimerID(UINT32 timerID)
   {
      return ossPack32To64( FSTimer, timerID ) ;
   }
}

