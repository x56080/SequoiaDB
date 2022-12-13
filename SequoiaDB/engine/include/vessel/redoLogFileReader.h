/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = redoLogFileReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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