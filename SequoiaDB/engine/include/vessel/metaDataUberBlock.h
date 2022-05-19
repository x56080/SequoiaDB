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

   Source File Name = metaDataUberBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_META_DATA_UBER_BLOCK_H_
#define VESSEL_META_DATA_UBER_BLOCK_H_

#include "vessel/pageIdentifier.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct smeUberBlock
   {
      UINT32 totalSegments = 0;
      UINT32 entryPid = INVALID_PAGE_ID;
   };//struct smeUberBlock

   struct lpmUberBlock
   {
      static constexpr UINT32 VERSION = 1;
      static constexpr UINT32 MAPPING_ENTRY_SIZE = 8;

      lpmUberBlock() {reset();}
      lpmUberBlock(const lpmUberBlock &o):
      version(o.version),
      sub(o.sub)
      {
         for (UINT32 i = 0; i < MAPPING_ENTRY_SIZE; ++i)
         {
            mappingEntries[i] = o.mappingEntries[i];
         }
      }

      lpmUberBlock &operator=(const lpmUberBlock &o)
      {
         version = o.version;
         sub = o.sub;
         for (UINT32 i = 0; i < MAPPING_ENTRY_SIZE; ++i)
         {
            mappingEntries[i] = o.mappingEntries[i];
         }
         return *this;
      }

      OSS_INLINE BOOLEAN isVaild()const
      {
         return VERSION == version;
      }

      OSS_INLINE void reset()
      {
         version = 0;
         sub = smeUberBlock();
         for (UINT32 i = 0; i < MAPPING_ENTRY_SIZE; ++i)
         {
            mappingEntries[i] = INVALID_PAGE_ID;
         }
      }

      UINT32 version = 0;
      smeUberBlock sub;
      UINT32 mappingEntries[MAPPING_ENTRY_SIZE] = {};
   };//struct lpmUberBlock
   constexpr UINT32 LPM_UBER_BLOCK_SIZE = sizeof(lpmUberBlock);

   struct lobmUberBlock
   {
      static constexpr UINT32 VERSION = 1;

      OSS_INLINE BOOLEAN isValid()const
      {
         return VERSION == version;
      }

      void reset()
      {
         version = 0;
         sub = smeUberBlock();
         bucketEntryPid = INVALID_PAGE_ID;
      }

      UINT32 version = 0;
      smeUberBlock sub;
      UINT32 bucketEntryPid = INVALID_PAGE_ID;
   };//struct lobmUberBlock
   constexpr UINT32 LOBM_UBER_BLOCK_SIZE = sizeof(lobmUberBlock);
#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_META_DATA_UBER_BLOCK_H_
