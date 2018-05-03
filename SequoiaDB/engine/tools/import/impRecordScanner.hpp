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

   Source File Name = impRecordScanner.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_SCANNER_HPP_
#define IMP_RECORD_SCANNER_HPP_

#include "impOptions.hpp"
#include "core.hpp"
#include "oss.hpp"
#include <string>

using namespace std;

namespace import
{
   class RecordScanner: public SDBObject
   {
   public:
      RecordScanner(const string& recordDelimiter,
                    const string& stringDelimiter,
                    INPUT_FORMAT format,
                    BOOLEAN linePriority);
      ~RecordScanner();
      inline INPUT_FORMAT format() { return _format; }
      INT32 scan(const CHAR* data, INT32 length, BOOLEAN final,
                 INT32& recordLength);

   private:
      INT32 _scanCSV(const CHAR* data, INT32 length, BOOLEAN final,
                     INT32& recordLength);
      INT32 _scanJSON(const CHAR* data, INT32 length, BOOLEAN final,
                      INT32& recordLength);

   private:
      string         _recordDelimiter;
      string         _stringDelimiter;
      INPUT_FORMAT   _format;
      BOOLEAN        _linePriority;
   };
}

#endif /* IMP_RECORD_SCANNER_HPP_ */
