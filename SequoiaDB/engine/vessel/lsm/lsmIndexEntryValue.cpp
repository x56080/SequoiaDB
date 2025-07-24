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

   Source File Name = lsmIndexEntryValue.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/17/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmIndexEntryValue.h"
#include "vessel/sliceTransfer.h"

namespace engine
{
namespace vessel
{
   lsmIndexEntryValueRef::lsmIndexEntryValueRef(const slice &value,
                                                BOOLEAN isStrict)
   {
      init(value, isStrict);
   }

   lsmIndexEntryValueRef::lsmIndexEntryValueRef(const rocksdb::Slice &value,
                                                BOOLEAN isStrict)
   {
      init(value, isStrict);
   }

   void lsmIndexEntryValueRef::reset()
   {
      _value.reset();
   }

   BOOLEAN lsmIndexEntryValueRef::isValid() const
   {
      return _value.isValid();
   }

   INT32 lsmIndexEntryValueRef::init(const slice &value,
                                     BOOLEAN isStrict)
   {
      INT32 rc = SDB_OK;
      const lsmIndexEntryValue *val = nullptr;

      if (!value.isValid() ||
          LSM_INDEX_ENTRY_VALUE_SIZE > value.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isStrict && LSM_INDEX_ENTRY_VALUE_SIZE != value.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      val = reinterpret_cast<const lsmIndexEntryValue *>(value.getData());
      if (!val->isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _value = value;

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexEntryValueRef::init(const rocksdb::Slice &value,
                                     BOOLEAN isStrict)
   {
      INT32 rc = SDB_OK;

      if (value.empty() ||
          nullptr == value.data())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = init(toSlice(value), isStrict);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "init lsm index entry value ref failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   const lsmIndexEntryValue *lsmIndexEntryValueRef::getValuePtr() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return reinterpret_cast<const lsmIndexEntryValue *>(_value.data());
   }
   
} // namespace vessel
} // namespace engine
