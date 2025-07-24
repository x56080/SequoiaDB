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

   Source File Name = sequoiaFSCacheQueue.hpp

   Descriptive Name = sequoiafs fuse file operation api.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        10/30/2020  zyj  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef __SEQUOIAFSCACHEQUEUE_HPP__
#define __SEQUOIAFSCACHEQUEUE_HPP__

#include "ossPriorityQueue.hpp"
#include "ossQueue.hpp"

#include "sequoiaFSDataCache.hpp"
#include "sequoiaFSFileLob.hpp"

namespace sequoiafs
{
   class sequoiaFSFileLobMgr;
   enum FS_TASK_TYPE
   {
      FS_TASK_FLUSH = 1,
      FS_TASK_DOWNLOAD
   } ;

   enum FS_TASK_PRIORITY
   {
      FS_TASK_PRIORITY_0 = 0,
      FS_TASK_PRIORITY_1 = 1
   } ;
   
   struct queueTask
   {
      public:
         UINT32 _hashKey;
         UINT32 _taskType;  // 1: upload, 2:download
         dataCache* _node;
         INT32  _flId;
         INT64  _offset;

      private:
         INT32 _priority ;

      public:
         queueTask( INT32 hashKey,
                    INT8 taskType, 
                    dataCache* node,
                    INT32 flId,
                    INT64 offset,
                    INT32 priority = 0 )
         {
            _hashKey = hashKey ;
            _taskType = taskType;
            _node = node;
            _priority = priority ;
            _flId = flId;
            _offset = offset;
         }
         queueTask()
         {
            _priority = 0 ;
         }

         bool operator< ( const queueTask &task ) const
         {
            if ( _priority < task._priority )
            {
               return true ;
            }
            return false ;
         }
   };
   
   class fsCacheQueue : public SDBObject
   {
      public:
         fsCacheQueue(sequoiaFSFileLobMgr* flMgr);
         ~fsCacheQueue(){}

         BOOLEAN addTask(queueTask task);
         void run();
         INT32 getQueueSize(){return _cacheQueue.size();}
         
      private:
         void _preReadCache(INT32 hashKey, INT32 flId, INT32 offset);
         void _flushCache(INT32 hashKey, dataCache* node);

      private:
         ossPriorityQueue<queueTask> _cacheQueue;
         sequoiaFSFileLobMgr * _flMgr;
   };
}
#endif

