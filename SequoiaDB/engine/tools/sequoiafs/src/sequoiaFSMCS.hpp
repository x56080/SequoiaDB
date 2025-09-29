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

   Source File Name = sequoiaFSMCS.hpp

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
#ifndef __SEQUOIAFSMCS_HPP__
#define __SEQUOIAFSMCS_HPP__

#include "sequoiaFSMCSOptionMgr.hpp"
//#include "sdbConnectionPoolcomm.hpp"
#include "sdbConnectionPool.hpp"

#include "netRouteAgent.hpp"

#include "sequoiaFSMCSMsgHandler.hpp"
#include "sequoiaFSMCSRegister.hpp"
#include "sequoiaFSMCSSessionMgr.hpp"
#include "sequoiaFSMCSOptionMgr.hpp"


using namespace engine;
using namespace bson; 
using namespace sdbclient;  

namespace sequoiafs
{
   struct mountNode
   {
      CHAR mountIp[OSS_MAX_IP_ADDR + 1];
      CHAR mountPath[OSS_MAX_PATHSIZE + 1];
      NET_HANDLE _handle;  
      mountNode* pre;
      mountNode* next;

      mountNode(CHAR *ip, CHAR* path, NET_HANDLE handle)
      {
         ossStrncpy(mountIp, ip, OSS_MAX_IP_ADDR);
         ossStrncpy(mountPath, path, OSS_MAX_PATHSIZE);
         _handle = handle;
         pre = NULL;
         next = NULL;
      }
   };
   
   class sequoiaFSMCS : public SDBObject
   {
      public:
         sequoiaFSMCS();
         ~sequoiaFSMCS();
         INT32 mcsThreadMain(INT32 argc, CHAR** argv);
         
         mountNode* getFsList(INT32 mountId);
         INT32 addMountNode(INT32 mountId, CHAR * mountIp, CHAR * mountPath, NET_HANDLE handle);
         void delMountNode(UINT32 handle);
         void cleanAllFS();
         
         netRouteAgent* getAgent(){return _agent;}
         mcsRegService* getRegService(){return &_regService;}

      private:
         INT32 _mcsInitDataSource(sequoiafsMcsOptionMgr *optionMgr);
         INT32 _activeEDU();
         INT32 _openPdLog();

      private:
         sdbConnectionPool   _ds;
         boost::thread*  _thRegisterDB;
         _netRouteAgent* _agent;
         
         ossPoolMap<UINT32, INT32> _handleMap;  
         ossPoolMap<INT32, mountNode*> _fsIdMap;
         ossSpinXLatch _mapMutex;

         CHAR _hostName[OSS_MAX_HOSTNAME+1];
         CHAR *_port;

         _mcsTimerHandler *_mcsTimer;
         _mcsSessionMgr    _mcsSesMgr;
         mcsMsghandler     _msgHandler;
         mcsRegService   _regService;  // reg in db and hold the lock
         sequoiafsMcsOptionMgr _optionMgr;
   };
}

#endif
