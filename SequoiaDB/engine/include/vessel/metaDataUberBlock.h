/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = metaDataUberBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_META_DATA_UBER_BLOCK_H_
#define VESSEL_META_DATA_UBER_BLOCK_H_

#include "vessel/pageIdentifier.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct lpmUberBlock
   {
      static constexpr UINT32 VERSION = 1;
      static constexpr UINT32 MAPPING_ENTRY_SIZE = 4;

      lpmUberBlock() {reset();}
      lpmUberBlock(const lpmUberBlock &o):
      version(o.version),
      smeEntryPid(o.smeEntryPid)
      {
         for (UINT32 i = 0; i < MAPPING_ENTRY_SIZE; ++i)
         {
            mappingEntries[i] = o.mappingEntries[i];
         }
      }

      lpmUberBlock &operator=(const lpmUberBlock &o)
      {
         version = o.version;
         smeEntryPid = o.smeEntryPid;
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
         checksum = 0;
         smeEntryPid = INVALID_PAGE_ID;
         for (UINT32 i = 0; i < MAPPING_ENTRY_SIZE; ++i)
         {
            mappingEntries[i] = INVALID_PAGE_ID;
         }
      }

      UINT32 generateChecksum()const;

      void refillChecksum();

      UINT32 version = 0;
      UINT32 checksum = 0;
      UINT32 smeEntryPid = INVALID_PAGE_ID;
      UINT32 mappingEntries[MAPPING_ENTRY_SIZE] = {};
   };//struct lpmUberBlock
   constexpr UINT32 LPM_UBER_BLOCK_SIZE = sizeof(lpmUberBlock);

   extern BOOLEAN inspectUberBlockChecksum(const lpmUberBlock &ub);

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
         smeEntryPid = INVALID_PAGE_ID;
         bucketEntryPid = INVALID_PAGE_ID;
      }

      UINT32 version = 0;
      UINT32 smeEntryPid = INVALID_PAGE_ID;
      UINT32 bucketEntryPid = INVALID_PAGE_ID;
   };//struct lobmUberBlock
   constexpr UINT32 LOBM_UBER_BLOCK_SIZE = sizeof(lobmUberBlock);
#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_META_DATA_UBER_BLOCK_H_
