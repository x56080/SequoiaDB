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

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct deltaLogFilePage
   {
      deltaLogFilePage(const deltaLogFilePage &) = delete;
      deltaLogFilePage &operator=(const deltaLogFilePage &) = delete;

      static constexpr UINT32 CURRENT_VERSION = 1;


      BOOLEAN isValid()const
      {
         return CURRENT_VERSION == version &&
                frontChecksum == backChecksum &&
                dataOffset <= getDataCapacity();
      }

      BOOLEAN hasCheckpoint()const
      {
         return 0 <= checkpointOffset;
      }

      static constexpr UINT32 getDataCapacity()
      {
         return sizeof(data);
      }

      void init(UINT32 prechecksum);

      UINT32 version = 0;
      UINT32 frontChecksum = 0;
      UINT32 prechecksum = 0;
      UINT32 flags = 0;
      INT32 checkpointOffset = -1; /// offset in data, not in page
      UINT32 dataOffset = 0;       /// offset in data, not in page
      CHAR data[4068];
      UINT32 backChecksum = 0;
   };//struct deltaLogFilePage

   constexpr UINT32 DELTA_LOG_FILE_PAGE_SIZE = sizeof(deltaLogFilePage);
   constexpr UINT32 DELTA_LOG_FILE_PAGE_COUNT_PER_SEG = 256;
   constexpr UINT32 DELTA_LOG_FILE_SEG_COUNT_PER_FILE = 4096;
   
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_FILE_H_