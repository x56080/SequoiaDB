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
#include "partialImpCache.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct deltaLogCheckpointRecord
   {
      deltaLogCheckpointRecord(){}
      ~deltaLogCheckpointRecord(){}
      deltaLogCheckpointRecord(const deltaLogCheckpointRecord &) = delete;
      deltaLogCheckpointRecord &operator=(const deltaLogCheckpointRecord &o)
      {
         flags = o.flags;
         checkpoint = o.checkpoint;
         elementCount = o.elementCount;
         checksum = o.checksum;
         pad = o.pad;
         return *this;
      }

      BOOLEAN isBegin()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_BEGIN_OF_BATCH);
      }
      void setBegin()
      {
         OSS_BIT_SET(flags, FLAG_BEGIN_OF_BATCH);
      }
      BOOLEAN isEnd()const
      {
         return 0 != OSS_BIT_TEST(flags, FLAG_END_OF_BATCH);
      }
      void setEnd()
      {
         OSS_BIT_SET(flags, FLAG_END_OF_BATCH);
      }

      static const UINT32 FLAG_BEGIN_OF_BATCH = 0x01;
      static const UINT32 FLAG_END_OF_BATCH = 0x02;


      UINT32 flags = 0;
      LPS_CHECKPOINT checkpoint;
      UINT32 elementCount = 0;
      UINT32 checksum = 0;
      UINT64 pad = 0;
   };//struct deltaLogFileHead

   static const UINT32 DELTA_LOG_CHECKPOINT_RECORD_SIZE = sizeof(deltaLogCheckpointRecord);

   struct deltaLogDumpRecord
   {
      UINT32 imp;
      UINT32 offset;
      CHAR cache[ID_MAP_PARTIAL_PAGE_CACHE_SIZE];
   };//struct deltaLogDumpRecord

   static const UINT32 DELTA_LOG_DUMP_RECORD_SIZE = sizeof(deltaLogDumpRecord);
   
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
         static const UINT32 MAX_SEGMENT_COUNT_PER_FILE = 1024;

         static const UINT32 MAX_RECORD_COUNT_PER_SEGMENT = 
                      (FILE_SEGMENT_SIZE - sizeof(UINT32) - DELTA_LOG_CHECKPOINT_RECORD_SIZE) /
                      DELTA_LOG_DUMP_RECORD_SIZE;
         
   };//class deltaLogFile
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_FILE_H_