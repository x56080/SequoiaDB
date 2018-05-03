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

   Source File Name = impRecordReader.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_RECORD_READER_HPP_
#define IMP_RECORD_READER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impInputStream.hpp"
#include "impRecordScanner.hpp"

namespace import
{
   class RecordReader: public SDBObject
   {
   public:
      RecordReader();
      ~RecordReader();
      void reset(CHAR* buffer, INT32 bufferSize,
                 InputStream* input, RecordScanner* scanner,
                 INT32 recordDelimiterLength);
      INT32 read(CHAR*& record, INT32& recordLength);

   private:
      void _clear();

   private:
      BOOLEAN        _set;
      InputStream*   _input;
      RecordScanner* _scanner;
      CHAR*          _buffer;
      INT32          _bufferSize;
      INT32          _recDelLen;

      // runtime
      CHAR*          _data;
      INT32          _dataLength;
      INT32          _remainSize;
      INT64          _readSize;
      INT64          _totalSize;
      BOOLEAN        _final;
   };
}

#endif /* IMP_RECORD_READER_HPP_ */
