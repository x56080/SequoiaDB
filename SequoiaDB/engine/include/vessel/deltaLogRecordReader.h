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

   Source File Name = deltaLogRecordReader.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_RECORD_READER_H_
#define VESSEL_DELTA_LOG_RECORD_READER_H_

#include "vessel/deltaLogRecord.h"

namespace engine
{
namespace vessel
{
   class dlrMappingReader : public SDBObject
   {
      public:
         ~dlrMappingReader() = delete;

         static INT32 read(const deltaLogRecord &dlr,
                           PAGE_SNAPSHOT_VERION &psv,
                           UINT8 &count);

         static INT32 getItem(const deltaLogRecord &dlr,
                              UINT8 i,
                              mappedLogicalPageId &mappedId);
   };//class dlrMappingReader

   class dlrRemappingReader : public SDBObject
   {
      public:
         ~dlrRemappingReader() = delete;
         static INT32 read(const deltaLogRecord &dlr,
                           PAGE_SNAPSHOT_VERION &psv,
                           UINT8 &flags,
                           UINT8 &count);

         static INT32 getItem(const deltaLogRecord &dlr,
                              UINT8 i,
                              mappedLogicalPageId &mappedId,
                              PAGE_ID &oldPid);
   };//class dlrRemappingReader

   class dlrUnmapingReader : public SDBObject
   {
      public:
         ~dlrUnmapingReader() = delete;
      public:
         static INT32 read(const deltaLogRecord &dlr,
                           UINT8 &flags,
                           UINT8 &count);

         static INT32 getItem(const deltaLogRecord &dlr,
                              UINT8 i,
                              mappedLogicalPageId &mappedId);
   };//class dlrUnmapingReader

/*
   class dlrReleasingReader : public SDBObject
   {
      public:
         ~dlrReleasingReader() = delete;
      public:
         static INT32 read(const deltaLogRecord &dlr, UINT8 &count);
         static INT32 getItem(const deltaLogRecord &dlr, UINT8 i, PAGE_ID &pid);
   };//class dlrReleasingReader
   */
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_RECORD_READER_H_
