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

   Source File Name = lpageMetaDataFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LPAGE_META_DATA_FILE_H_
#define VESSEL_LPAGE_META_DATA_FILE_H_

#include "vessel/baseMetaDataFile.h"

namespace engine
{
namespace vessel
{
   class lpageMetaDataFile : public baseMetaDataFile
   {
      public:
         static constexpr UINT32 PAGE_SIZE = 65536;
         static constexpr UINT32 PAGE_COUNT_PER_SEG = 64; /// 4MB per segment
         static constexpr UINT64 MAX_FILE_SIZE = (UINT64)8 << 30; /// 8GB
         static constexpr UINT32 MAX_PAGE_COUNT = MAX_FILE_SIZE / PAGE_SIZE;
         static constexpr UINT32 MAX_SEG_COUNT = MAX_PAGE_COUNT / PAGE_COUNT_PER_SEG;
         static constexpr UINT32 SME_SIZE = MAX_PAGE_COUNT >> 3;

      public:
         virtual UINT32 _getReservedAreaSize()const override
         {
            return ossAlignX(SME_SIZE, DMS_PAGE_SIZE64K);
         }
   };//class lpageMetaDataFile
} // namespace vessel

} // namespace engine


#endif//VESSEL_LPAGE_META_DATA_FILE_H_
