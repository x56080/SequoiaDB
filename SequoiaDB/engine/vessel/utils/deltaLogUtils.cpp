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

   Source File Name = deltaLogUtils.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/deltaLogUtils.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   DELTA_LOG_CHECKSUM createDeltaLogRecordChecksum(const deltaLogRecord &dlr)
   {
      SDB_ASSERT(dlr.isValid(), "can not be invalid");
      DELTA_LOG_CHECKSUM checksum = 0;
      if (OSS_LIKELY(dlr.isValid()))
      {
         checksum = XXH3_64bits(dlr.getLogHead(), dlr.getLogHead()->_size);
      }
      return checksum;
   }

   UINT64 alignDeltaLogRecordOffset(UINT64 offset)
   {
      UINT64 algiendOffset = offset;
      UINT32 offsetInSegment = deltaLogFileDef::getOffsetInSegmentByOffset(offset);
      UINT32 remainSize = deltaLogFileDef::FILE_SEGMENT_SIZE - offsetInSegment;
      if (remainSize < deltaLogFileDef::MIN_RECORD_SIZE_ON_DISK)
      {
         algiendOffset += remainSize;
      }
      return algiendOffset;
   }
}//namespace vessel
}//namespace engine