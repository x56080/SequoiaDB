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

   Source File Name = impRoutine.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_ROUTINE_HPP_
#define IMP_ROUTINE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impOptions.hpp"
#include "impRecordQueue.hpp"
#include "impParser.hpp"
#include "impImporter.hpp"
#include "impSharding.hpp"

using namespace std;

namespace import
{
   class Routine
   {
   public:
      Routine(Options& options);
      ~Routine();
      INT32 run();
      void printStatistics();

   private:
      INT32 _startParser();
      INT32 _waitParserStop();
      INT32 _startImporter(INT32 workerNum);
      INT32 _stopImporter();
      INT32 _startSharding();
      INT32 _stopSharding();

   private:
      Options&          _options;
      RecordQueue       _parsedQueue;
      RecordQueue       _shardingQueue;

      Parser            _parser;
      Importer          _importer;
      Sharding          _sharding;
   };
}

#endif /* IMP_ROUTINE_HPP_ */
