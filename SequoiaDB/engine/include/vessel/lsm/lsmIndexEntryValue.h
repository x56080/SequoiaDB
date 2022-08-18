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

   Source File Name = lsmIndexEntryValue.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/12/2021  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_LSM_INDEX_ENTRY_VALUE_H_
#define VESSEL_LSM_INDEX_ENTRY_VALUE_H_

#include "dpsTransID.hpp"
#include "rocksdb/slice.h"
#include "dpsDef.hpp"

namespace engine
{
namespace vessel
{
   enum LSM_INDEX_ENTRY_TYPE : UINT8
   {
      LSM_INDEX_ENTRY_TYPE_INVALID = 0,
      LSM_INDEX_ENTRY_TYPE_INSERT = 1,
      LSM_INDEX_ENTRY_TYPE_DELETE = 2,
   };

#pragma pack(4)

   struct lsmIndexEntryValue
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return LSM_INDEX_ENTRY_TYPE_INVALID != type &&
                DPS_INVALID_LSN_OFFSET != lsn;
      }
      OSS_INLINE BOOLEAN isDeleted()const
      {
         return LSM_INDEX_ENTRY_TYPE_DELETE == type;
      }

      UINT8 version = 0;
      UINT8 type = LSM_INDEX_ENTRY_TYPE_INVALID;
      DPS_TRANS_ID transID;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
   };//struct lsmHitEntryValue
   constexpr UINT32 LSM_INDEX_ENTRY_VALUE_SIZE = sizeof(lsmIndexEntryValue);

   struct lsmHisTroricIndexEntryValue
   {
      lsmIndexEntryValue val;
      INT64 rbsOffsetLid = -1;
      UINT16 rbsOffsetCL = ~0;
      UINT16 pad = 0;
   };//struct lsmHisTroricIndexEntryValue
   constexpr UINT32 LSM_H_INDEX_ENTRY_VALUE_SIZE = sizeof(lsmHisTroricIndexEntryValue);

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_INDEX_ENTRY_VALUE_H_