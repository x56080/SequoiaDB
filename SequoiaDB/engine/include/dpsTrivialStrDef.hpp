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

   Source File Name = dpsTrivialStrDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_TRIVIAL_STR_DEF_HPP__
#define DPS_TRIVIAL_STR_DEF_HPP__

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
   using DPS_TS_FIELD_TAG = UINT8;
   using DPS_TS_FIELD_MB = UINT8;

   constexpr DPS_TS_FIELD_MB DPS_TS_FIELD_TAG_MASK = 0x7F;
   constexpr DPS_TS_FIELD_MB DPS_TS_MORE_FIELD_FLAG = ~DPS_TS_FIELD_TAG_MASK;
   constexpr UINT32 DPS_TS_SIZE_BOUND = 0xFF;

   OSS_INLINE BOOLEAN dpsIsValidTsTag(DPS_TS_FIELD_TAG tag)
   {
      return tag <= DPS_TS_FIELD_TAG_MASK;
   }
#pragma pack(1)
   struct dpsTsFieldHeader
   {
      OSS_INLINE void reset()
      {
         mb = 0;
         size = 0;
      }

      OSS_INLINE void setTag(DPS_TS_FIELD_TAG tag)
      {
         mb = (tag & DPS_TS_FIELD_TAG_MASK);
      }
      OSS_INLINE DPS_TS_FIELD_TAG getTag() const
      {
         return mb & DPS_TS_FIELD_TAG_MASK;
      }
      OSS_INLINE void setMore()
      {
         mb |= DPS_TS_MORE_FIELD_FLAG;
      }
      OSS_INLINE BOOLEAN isEnding() const
      {
         return 0 == (mb & DPS_TS_MORE_FIELD_FLAG);
      }
      OSS_INLINE BOOLEAN hasMore() const 
      {
         return 0 != (mb & DPS_TS_MORE_FIELD_FLAG);
      }

      DPS_TS_FIELD_MB mb = 0;
      UINT8 size = 0;
   };
   constexpr UINT32 DPS_TS_FIELD_HEAD_SIZE = sizeof(dpsTsFieldHeader);
#pragma pack()

} // namespace engine


#endif//DPS_TRIVIAL_STR_DEF_HPP__