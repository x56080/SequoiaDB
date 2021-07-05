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

   Source File Name = deltaLogReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_READER_H_
#define VESSEL_DELTA_LOG_READER_H_

#include "vessel/deltaLogFileDef.h"
#include "vessel/storageFileMap.h"
#include "vessel/deltaLogRecord.h"

namespace engine
{
namespace vessel
{
   class deltaLogReader : public SDBObject
   {
      public:
         deltaLogReader();
         deltaLogReader(const deltaLogReader &) = delete;
         ~deltaLogReader();
         deltaLogReader &operator=(const deltaLogReader &) = delete;

      public:
         void fini();

         INT32 init(const storageFileMap *logFiles,
                    UINT64 firstRecordOffset,
                    UINT64 lastRecordHeadOffset = DPS_INVALID_LSN_OFFSET);

         /// return SDB_VESSEL_EOC when hit the end.
         INT32 getNext(deltaLogRecord &dlr,
                       UINT64 *offset);

      private:
         BOOLEAN getRecord(UINT64 offset,
                          deltaLogRecord &dlr)const;
      private:
         const storageFileMap *_logFiles = NULL;
         UINT64 _firstRecordOffset = DPS_INVALID_LSN_OFFSET;
         UINT64 _lastRecordHeadOffset = DPS_INVALID_LSN_OFFSET;
         UINT64 _maxFileOffset = 0;
         UINT64 _currentOffset = 0;
   };//class deltaLogReader
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_READER_H_