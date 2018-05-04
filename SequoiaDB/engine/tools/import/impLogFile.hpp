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

   Source File Name = impLogFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_LOG_FILE_HPP_
#define IMP_LOG_FILE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "oss.h"
#include "../client/bson/bson.h"
#include <string>

using namespace std;

namespace import
{
   class LogFile: public SDBObject
   {
   public:
      LogFile();
      ~LogFile();
      INT32 init(const string& fileName, BOOLEAN threadSafe = FALSE);
      INT32 write(const CHAR* buf, INT32 length);
      INT32 write(const bson* obj);

      inline const string& fileName() const { return _fileName; }

   private:
      INT32 _open();
      inline void _beginWrite()
      {
         if (_threadSafe)
         {
            ossMutexLock(&_fileLock);
         }
      }

      inline void _endWrite()
      {
         if (_threadSafe)
         {
            ossMutexUnlock(&_fileLock);
         }
      }

   private:
      string      _fileName;
      OSSFILE     _file;
      BOOLEAN     _isOpened;
      BOOLEAN     _threadSafe;
      ossMutex    _fileLock;
      BOOLEAN     _inited;
   };

   string makeRecordLogFileName(const string& csname, 
                                const string& clname,
                                const string& custom);
}

#endif /* IMP_LOG_FILE_HPP_ */
