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

   Source File Name = dpsTrivialStrDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_TRIVIAL_STR_DEF_HPP__
#define DPS_TRIVIAL_STR_DEF_HPP__

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
   using DPS_TS_FIELD_TAG = UINT8;
   using DPS_TS_FIELD_MB = UINT8;
   using DPS_TS_EXT_SIZE_BLOCK = UINT16;

   constexpr DPS_TS_FIELD_MB DPS_TS_FIELD_TAG_MASK = 0x7F;
   constexpr DPS_TS_FIELD_MB DPS_TS_FIELD_ENDING_FLAG = 0x80;
   constexpr UINT32 DPS_TS_SIZE_BOUND = 0xFF;

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
      OSS_INLINE void setEnding()
      {
         mb |= DPS_TS_FIELD_ENDING_FLAG;
      }
      OSS_INLINE BOOLEAN isEnding() const
      {
         return mb & DPS_TS_FIELD_ENDING_FLAG;
      }

      DPS_TS_FIELD_MB mb = 0;
      UINT8 size = 0;
   };
   constexpr UINT32 DPS_TS_FIELD_HEAD_SIZE = sizeof(dpsTsFieldHeader);

   struct dpsTsFieldHeaderExt
   {
      dpsTsFieldHeader h;
      DPS_TS_EXT_SIZE_BLOCK size = 0;
   };
   constexpr UINT32 DPS_TS_FIELD_HEAD_EXT_SIZE = sizeof(dpsTsFieldHeaderExt);
#pragma pack()

   OSS_INLINE UINT32 dpsGetTsFieldHeadSize(UINT32 valueSize)
   {
      return valueSize < DPS_TS_SIZE_BOUND ?
             DPS_TS_FIELD_HEAD_SIZE : DPS_TS_FIELD_HEAD_EXT_SIZE;
   }

   void dpsInitTsFieldHead(DPS_TS_FIELD_TAG tag,
                           UINT32 valueSize,
                           UINT32 bufSize,
                           void *buf);

} // namespace engine


#endif//DPS_TRIVIAL_STR_DEF_HPP__