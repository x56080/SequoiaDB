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

   Source File Name = deltaLogRecordBuilder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_RECORD_BUILDER_H_
#define VESSEL_DELTA_LOG_RECORD_BUILDER_H_

#include "vessel/deltaLogRecord.h"

namespace engine
{
namespace vessel
{
   class deltaLogRecordBuilder : public SDBObject
   {
      public:
         deltaLogRecordBuilder(){}
         deltaLogRecordBuilder(const deltaLogRecordBuilder &) = delete;
         deltaLogRecordBuilder &operator=(const deltaLogRecordBuilder &) = delete;

      public:
         INT32 beginToBuild(DELTA_LOG_RECORD_TYPE type);

         INT32 append(UINT32 size, const void *data);

         void done();

         OSS_INLINE const deltaLogRecordHead &getLogHead()const
         {
            return *((const deltaLogRecordHead *)_buffer);
         }
         OSS_INLINE const CHAR *getRawData()const
         {
            return _buffer;
         }

         OSS_INLINE BOOLEAN isBuilding()const
         {
            return 0 != _w;
         }

         OSS_INLINE UINT32 getFreeBufferSize()const
         {
            return MAX_DELTA_LOG_RECORD_SIZE - _w;
         }
         
         void reset();

         deltaLogRecord getDeltaLogRecord()const;

      public:
         INT32 buildDummyLog(UINT32 recordSize);

         INT32 buildCheckpointLog(const LPS_CHECKPOINT &checkpoint);

         INT32 buildMappingLog(PAGE_SNAPSHOT_VERION psv,
                               UINT8 count,
                               const mappedLogicalPageId *mpids);
                               
         INT32 buildRemappingLog(PAGE_SNAPSHOT_VERION psv,
                                 UINT8 count,
                                 const mappedLogicalPageId *mpids,
                                 const PAGE_ID *oldPids,
                                 BOOLEAN releaseOld);

         INT32 buildUnmappingLog(UINT8 count,
                                 const mappedLogicalPageId *mpids,
                                 BOOLEAN releaseOld);

         INT32 buildReleasingLog(UINT8 count,
                                 const PAGE_ID *pids);
      private:
         CHAR _buffer[MAX_DELTA_LOG_RECORD_SIZE] = {0};
         UINT32 _w = 0;
   };//class deltaLogRecordBuilder
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_RECORD_BUILDER_H_
