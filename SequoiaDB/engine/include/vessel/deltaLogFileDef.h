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

   Source File Name = deltaLogFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_FILE_H_
#define VESSEL_DELTA_LOG_FILE_H_

#include "vessel/storageFile.h"
#include "vessel/deltaLogRecord.h"

namespace engine
{
namespace vessel
{
   typedef UINT16 DELTA_LOG_CHECKSUM;

   class deltaLogFileDef
   {
      public:
         deltaLogFileDef(){}
         ~deltaLogFileDef() = delete;

      public:
         static const UINT32 PAGE_SIZE = DMS_PAGE_SIZE4K;
         static const UINT32 PAGE_COUNT_PER_SEGMENT = 1024;
         static const UINT32 FILE_SEGMENT_SIZE = PAGE_SIZE *
                                                 PAGE_COUNT_PER_SEGMENT;
         static const UINT32 MAX_SEGMENT_COUNT_PER_FILE = 16;
         static const UINT32 MAX_PAGE_COUNT_PER_FILE = PAGE_COUNT_PER_SEGMENT *
                                                       MAX_SEGMENT_COUNT_PER_FILE;
         static const UINT32 MAX_FILE_SIZE = PAGE_SIZE * MAX_PAGE_COUNT_PER_FILE;

         static const UINT32 CHECKSUM_SIZE = sizeof(DELTA_LOG_CHECKSUM);
         static const UINT32 MIN_RECORD_SIZE_ON_DISK = DELTA_LOG_RECORD_HEAD_SIZE + CHECKSUM_SIZE;

      public:
         OSS_INLINE static UINT64 getLogFileSequenceByOffset(UINT64 offset)
         {
            return offset / MAX_FILE_SIZE;
         }
         OSS_INLINE static UINT32 getSegmentIdInFileByOffset(UINT64 offset)
         {
            return (offset / FILE_SEGMENT_SIZE) & (MAX_SEGMENT_COUNT_PER_FILE - 1);
         }
         OSS_INLINE static UINT32 getOffsetInSegmentByOffset(UINT64 offset)
         {
            return offset & (FILE_SEGMENT_SIZE - 1);
         }
         
   };//class deltaLogFile
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_FILE_H_