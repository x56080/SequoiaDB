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

   Source File Name = redoLogFileReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_REDO_LOG_FILE_READER_H_
#define VESSEL_REDO_LOG_FILE_READER_H_

#include "vessel/redoLogFile.h"
#include "utilUniqueBuffer.hpp"
#include "dpsLogRecord.hpp"
#include "dpsRecordElements.hpp"

namespace engine
{
namespace vessel
{
   class redoLogFileReader : public SDBObject
   {
      public:
         redoLogFileReader() = default;
         ~redoLogFileReader() = default;

      public:
         INT32 open(const redoLogFile *rfile, UINT64 lsnUpBound);
         void reset();
         INT32 next();
         INT32 moveToLastRecord();
      
      public:
         OSS_INLINE BOOLEAN isReady() const { return 0 < _header._length; }
         BOOLEAN isTailInBound() const;
         BOOLEAN hasElements() const;
         INT32 getElements(utilUniqueBuffer *outerBuf,
                           dpsRecordElements &elements) const;

         OSS_INLINE const dpsLogRecordHeader &getRecordHeader() const { return _header; }

      private:
         OSS_INLINE BOOLEAN _hasNextRecordSpace() const
         {
            return (_offset + DPS_LOG_HEAD_SIZE) <= _rfile->getFileBodySize();
         }

      private:
         const redoLogFile *_rfile = nullptr;
         UINT64 _lsnUpBound = 0;
         UINT32 _offset = 0;
         dpsLogRecordHeader _header;
         utilUniqueBuffer _decompressionBuf;
   };//class redoLogFileReader
} // namespace vessel
   
} // namespace engine


#endif//VESSEL_REDO_LOG_FILE_READER_H_