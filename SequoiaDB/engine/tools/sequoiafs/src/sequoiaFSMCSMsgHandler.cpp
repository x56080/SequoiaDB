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

   Source File Name = sequoiaFSMCSMsgHandler.cpp

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
#include "sequoiaFSMCSMsgHandler.hpp"
#include "sequoiaFSMCS.hpp"

using namespace engine;

namespace sequoiafs
{
   mcsMsghandler::mcsMsghandler(_pmdAsycSessionMgr *pSessionMgr,
                                sequoiaFSMCS *mcs)
   :_pmdAsyncMsgHandler(pSessionMgr)
   {
      _mcs = mcs;
   }

   mcsMsghandler::~mcsMsghandler()
   {
   }

   INT32 mcsMsghandler::handleMsg(const NET_HANDLE &handle,
                                  const _MsgHeader *header,
                                  const CHAR *msg)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT (NULL != header, "message-header should not be NULL");

      if (FS_REGISTER_REQ == header->opCode)
      {
         rc = handleRegRequest(handle, (_mcsRegReq*)header);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to handleRegRequest, rc=%d", rc);
            goto error;
         }
      }
      else if(FS_RELEASELOCK == header->opCode)
      {
         rc = handleNotifyRequest(handle, (_mcsNotifyReq*)header);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to handleNotifyRequest, rc=%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error :
      goto done ;
   }

   void mcsMsghandler::handleClose(const NET_HANDLE &handle,
                                   _MsgRouteID id)
   {
      _mcs->delMountNode(handle);
   }

   INT32 mcsMsghandler::handleRegRequest(const NET_HANDLE &handle,
                                         const _mcsRegReq *request)
   {
      INT32 rc = SDB_OK;
      _mcsRegRsp regResponse;

      SDB_ASSERT(NULL != request, "request message is invalid");
      SDB_ASSERT(FS_REGISTER_REQ == request->header.opCode,
                 "opcode of message is invalid");

      if((INT32)ossStrlen(request->mountPath) != request->pathLen)
      {
         PD_LOG(PDERROR, "pathLen(%d) in msg != the length of mountPath(%s) in msg,",
                          request->pathLen, request->mountPath);
         rc = SDB_INVALIDARG;
         goto error;
      }

      if(_mcs->getRegService()->isRegistered())
      {
         //mcs addFsNode(mountId, fsnode)
         rc = _mcs->addMountNode(request->mountId,
                                (CHAR*)&(request->mountIp),
                                (CHAR*)&(request->mountPath),
                                handle);
         if(rc != SDB_OK)
         {
            PD_LOG(PDERROR, "Failed to add mountNode, rc=%d", rc);
            goto error;
         }

         //send response msg
         regResponse.reply.header.messageLength = sizeof(_mcsRegRsp);
         regResponse.reply.header.opCode = FS_REGISTER_RSP;
         regResponse.reply.header.TID = request->header.TID;
         regResponse.reply.header.requestID = request->header.requestID;
         regResponse.reply.header.routeID.value = 0;
         regResponse.reply.res = SDB_OK;

         _mcs->getAgent()->syncSend(handle, (MsgHeader *)&regResponse);
         if(SDB_OK != rc)
         {
            _mcs->delMountNode(handle);
            PD_LOG(PDERROR, "Failed to send rsp for register, rc=%d", rc);
            goto error;
         }
      }
      else
      {
         rc = SDBCM_SVC_STARTING;
         PD_LOG(PDERROR, "MCS is registing to SequoiaDB, rc=%d", rc);
      }

   done:
      return rc;

   error :
      _mcs->getAgent()->close(handle);
      goto done;
   }

   INT32 mcsMsghandler::handleNotifyRequest(const NET_HANDLE &handle,
                                            const _mcsNotifyReq *request)
   {
      INT32 rc = SDB_OK;
      _mcsNotifyReq* broadcast = NULL;
      mountNode* cur = NULL;

      SDB_ASSERT(NULL != request, "request message is invalid");
      SDB_ASSERT(FS_RELEASELOCK == request->header.opCode,
                 "opcode of message is invalid");

      mountNode* mountFS = _mcs->getFsList(request->mountId);  //TODO:使用list的时候也要在锁中

      //build broadcast
      broadcast = (_mcsNotifyReq* )SDB_THREAD_ALLOC(request->header.messageLength);
      if(NULL == broadcast)
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "Failed to SDB_THREAD_ALLOC, length:%d", request->header.messageLength);
         goto error;
      }
      ossMemcpy(broadcast, request, request->header.messageLength);
      broadcast->header.opCode = MS_RELEASELOCK;

      while(mountFS != NULL)
      {
         cur = mountFS;
         mountFS = mountFS->next;
         if(cur->_handle == handle)
         {
            continue;
         }

         NET_HANDLE curHandle = (NET_HANDLE)(cur->_handle);
         rc = _mcs->getAgent()->syncSend(curHandle, (MsgHeader *)broadcast);

         if(SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to syncSend broadcast, ip:%s, path:%s, rc=%d", cur->mountIp, cur->mountPath, rc);
         }
      }

   done:
      if(broadcast)
      {
         SDB_THREAD_FREE(broadcast);
      }
      return rc;
   error :
      goto done;
   }

   _mcsTimerHandler::_mcsTimerHandler (_pmdAsycSessionMgr * pSessionMgr)
      :_pmdAsyncTimerHandler (pSessionMgr)
   {
   }

   _mcsTimerHandler::~_mcsTimerHandler()
   {
   }

   UINT64 _mcsTimerHandler::_makeTimerID(UINT32 timerID)
   {
      return ossPack32To64( MCSTimer, timerID ) ;
   }
}

