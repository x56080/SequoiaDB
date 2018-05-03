/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = impSharding.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_SHARDING_HPP_
#define IMP_SHARDING_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impRecordSharding.hpp"
#include "impRecordQueue.hpp"
#include "impWorker.hpp"
#include "impOptions.hpp"
#include "impLogFile.hpp"
#include <map>

using namespace std;

namespace import
{
   typedef map<UINT32, RecordArray*> SubShardingGroups;
   typedef map<string, SubShardingGroups> ShardingGroups;

   class Sharding: public WorkerArgs
   {
   public:
      Sharding();
      ~Sharding();
      INT32 init(Options* options,
                 RecordQueue* inQueue,
                 RecordQueue* outQueue);
      BOOLEAN needSharding() const;
      INT32 getGroupNum() const;
      INT32 start();
      INT32 stop();
      inline BOOLEAN isStopped() const { return _stopped; }
      inline INT64 shardingNum() const { return _shardingNum; }
      inline INT64 failedNum() const { return _failedNum; }
      inline const string& logFileName() const
      {
         return _logFile.fileName();
      }

   private:
      Options*          _options;
      RecordQueue*      _inQueue;
      RecordQueue*      _outQueue;
      BOOLEAN           _inited;

      RecordSharding    _sharding;
      Worker*           _worker;
      BOOLEAN           _stopped;

      ShardingGroups    _groups;

      LogFile           _logFile;

      // statistics
      INT64             _shardingNum;
      INT64             _failedNum;

      friend void _shardingRoutine(WorkerArgs* args);
   };
}

#endif /* IMP_SHARDING_HPP_ */
