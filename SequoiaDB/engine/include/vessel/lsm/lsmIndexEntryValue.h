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

#include "vessel/slice.h"
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
   class lsmIndexEntryValueRef : public SDBObject
   {
      public:
         lsmIndexEntryValueRef() = default;
         ~lsmIndexEntryValueRef() = default;
         explicit lsmIndexEntryValueRef(const slice &value,
                                        BOOLEAN isStrict = TRUE);
         explicit lsmIndexEntryValueRef(const rocksdb::Slice &value,
                                        BOOLEAN isStrict = TRUE);
         
      public:
         void reset();
         BOOLEAN isValid() const;
         INT32 init(const rocksdb::Slice &value,
                    BOOLEAN isStrict = TRUE);
         INT32 init(const slice &value,
                    BOOLEAN isStrict = TRUE);
      
      public:
         const lsmIndexEntryValue *getValuePtr() const;
      
      private:
         slice _value;
   }; // class lsmIndexEntryValueRef

} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_INDEX_ENTRY_VALUE_H_