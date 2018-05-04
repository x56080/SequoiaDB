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

   Source File Name = impImporter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_IMPORTER_HPP_
#define IMP_IMPORTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impOptions.hpp"
#include "impWorker.hpp"
#include "impCoord.hpp"
#include "impRecordQueue.hpp"
#include "impLogFile.hpp"
#include "ossAtomic.hpp"
#include <vector>

using namespace std;

namespace import
{
   class Importer: public SDBObject
   {
   public:
      Importer();
      ~Importer();
      INT32 init(Options* options,
                 RecordQueue* workQueue,
                 INT32 workerNum);
      INT32 start();
      INT32 stop();
      inline BOOLEAN isStopped()
      {
         return 0 == _livingNum.fetch();
      }
      inline INT64 importedNum() { return _importedNum.fetch(); }
      inline INT64 failedNum() { return _failedNum.fetch(); }
      inline const string& logFileName() const
      {
         return _logFile.fileName();
      }

   private:
      Options*          _options;
      RecordQueue*      _workQueue;
      BOOLEAN           _inited;

      Coords            _coords;
      UINT32            _refCount;
      vector<Worker*>   _workers;
      ossAtomicSigned32 _livingNum;
      LogFile           _logFile;

      // statistics
      ossAtomicSigned64 _importedNum;
      ossAtomicSigned64 _failedNum;

      friend void _importerRoutine(WorkerArgs* args);
   };
}

#endif /* IMP_IMPORTER_HPP_ */
