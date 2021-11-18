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
#include "vessel/logicalPageSpaceCheckpoint.h"

namespace engine
{
namespace vessel
{
   typedef UINT16 DELTA_LOG_CHECKSUM;

#pragma pack(4)
   struct deltaLogFileHead
   {
      deltaLogFileHead(){}
      ~deltaLogFileHead(){}
      deltaLogFileHead(const deltaLogFileHead &) = delete;
      deltaLogFileHead &operator=(const deltaLogFileHead &o)
      {
         version = o.version;
         baseIdMapFile = o.baseIdMapFile;
         checkpoint = o.checkpoint;
         elementCount = o.elementCount;
         return *this;
      }

      UINT32 version = 0;
      UINT64 baseIdMapFile = 0;
      LPS_CHECKPOINT checkpoint;
      UINT64 elementCount = 0;

   };//struct deltaLogFileHead

   
#pragma pack()

   class deltaLogFile : public storageFile
   {
      public:
         deltaLogFile(){}
         virtual ~deltaLogFile(){}

      public:
         static const UINT32 VERSION = 1;

         static const UINT32 PAGE_SIZE = DMS_PAGE_SIZE4K;
         static const UINT32 PAGE_COUNT_PER_SEGMENT = 16;
         static const UINT32 FILE_SEGMENT_SIZE = PAGE_SIZE *
                                                 PAGE_COUNT_PER_SEGMENT;
         static const UINT32 MAX_SEGMENT_COUNT_PER_FILE = 2048;
         static const UINT32 MAX_PAGE_COUNT_PER_FILE = PAGE_COUNT_PER_SEGMENT *
                                                       MAX_SEGMENT_COUNT_PER_FILE;
         static const UINT32 MAX_FILE_SIZE = PAGE_SIZE * MAX_PAGE_COUNT_PER_FILE;

      public:
         virtual BOOLEAN validateUserDefinedHead(const void *head)const;

      public:
         OSS_INLINE static UINT64 getLogFileSequenceByOffset(UINT64 offset)
         {
            return offset / MAX_FILE_SIZE;
         }
         OSS_INLINE static UINT32 getInFileSegmentId(UINT64 offset)
         {
            return (offset / FILE_SEGMENT_SIZE) % MAX_SEGMENT_COUNT_PER_FILE;
         }
         OSS_INLINE static UINT32 getOffsetInSegment(UINT64 offset)
         {
            return offset % FILE_SEGMENT_SIZE;
         }

         INT32 getDeltaLogFileHead(deltaLogFileHead &h)const;
         
   };//class deltaLogFile
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_FILE_H_