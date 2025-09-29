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

   Source File Name = sequoiaFSMCSRegister.hpp

   Descriptive Name = sequoiafs meta cache service.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date      Who Description
   ====== =========== === ==============================================
      01/07/2021  zyj  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSMCSREG_HPP__
#define __SEQUOIAFSMCSREG_HPP__

#include "sdbConnectionPool.hpp"
#include "sequoiaFSDao.hpp"
#include "sequoiaFSCommon.hpp"

using namespace bson; 
using namespace sdbclient;  

#define MCS_REGISTER_DB_SECOND 30

namespace sequoiafs
{
   class sequoiaFSMCS;
   class mcsRegService : public SDBObject
   {
      public:
         mcsRegService(sequoiaFSMCS *mcs)
         {
            _mcs = mcs;
            _running = FALSE;
            _isRegistered = FALSE;
            _isWorking = FALSE;
         }
         ~mcsRegService();
         INT32 init(sdbConnectionPool* ds, CHAR* hostname, CHAR* port);
         void hold();
         BOOLEAN isRegistered(){return _isRegistered;}
         BOOLEAN isWorking(){return _isWorking;}
         void stop(){_running = FALSE;}

      private:   
         INT32 _registerToDB(BOOLEAN isForce);
         INT32 _checkHostNamePort(sdbCursor* cursor);
         INT32 _insertMCSService(fsConnectionDao* db);
         INT32 _updateMCSService(fsConnectionDao* db);

      private:
         BOOLEAN          _running;
         BOOLEAN          _isRegistered;
         BOOLEAN          _isWorking;
         CHAR*            _hostname;
         CHAR*            _port;

         sdbConnectionPool*   _ds;
         sequoiaFSMCS*    _mcs;
   };
}

#endif

