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

   Source File Name = impParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_PARSER_HPP_
#define IMP_PARSER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impOptions.hpp"
#include "impWorker.hpp"
#include "impRecordQueue.hpp"
#include "impLogFile.hpp"

namespace import
{
   class Parser: public WorkerArgs
   {
   public:
      Parser();
      ~Parser();
      INT32 init(Options* options,
                 RecordQueue* workQueue);
      INT32 start();
      INT32 stop();
      inline BOOLEAN isStopped() const { return _stopped; }
      inline INT64 parsedNum() const { return _parsedNum; }
      inline INT64 failedNum() const { return _failedNum; }
      inline const string& logFileName() const
      {
         return _logFile.fileName();
      }

   private:
      Options*          _options;
      RecordQueue*      _workQueue;
      BOOLEAN           _inited;

      Worker*           _worker;
      BOOLEAN           _stopped;
      LogFile           _logFile;

      // statistics
      INT64             _parsedNum;
      INT64             _failedNum;

      friend void _parserRoutine(WorkerArgs* args);
   };
}

#endif /* IMP_PARSER_HPP_ */
