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
