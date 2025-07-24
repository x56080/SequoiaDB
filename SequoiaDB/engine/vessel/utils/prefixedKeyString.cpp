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

   Source File Name = prefixedKeyString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/17/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/prefixedKeyString.h"
#include "vessel/keyString.h"
#include "ossLikely.hpp"
#include "pd.hpp"
#include <memory>
#include "utilAllocator.hpp"

namespace engine
{
namespace vessel
{
   prefixedKeyString::prefixedKeyString(const slice &suffix,
                                        const slice &prefix)
   {
      init(suffix, prefix);
   }

   void prefixedKeyString::reset()
   {
      _prefix.reset();
      _suffix.reset();
      _desc.reset();
   }

   CHAR prefixedKeyString::operator[](UINT32 pos)const
   {
      SDB_ASSERT(pos < getTotalSize(), "out of bound");
      if (pos < _prefix.size())
      {
         return _prefix.data()[pos];
      }
      else
      {
         return _suffix.data()[pos];
      }
   }

   INT32 prefixedKeyString::init(const slice &suffix, const slice &prefix)
   {
      INT32 rc = SDB_OK;
      reset();

      if (OSS_UNLIKELY(!suffix.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = keyString::parseMetaFromSlice(suffix, _desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to parse suffix:%d", rc);
         goto error;
      }
      else if (_desc.getStringSizeExpected() !=
               (prefix.size() + suffix.size()))
      {
         PD_LOG(PDERROR, "unexpected total size[%d, %d]",
                _desc.getStringSizeExpected(),
                prefix.size() + suffix.size());
         rc = SDB_VESSEL_INVALID_KEY_STR_DATA;
         goto error;
      }
      else
      {
         _suffix = suffix;
         _prefix = prefix;
      }
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   slice prefixedKeyString::getComparableSuffix() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _suffix.getSlice(0, _desc.keySize - _prefix.size());
   }

   INT32 prefixedKeyString::compare(const prefixedKeyString &r) const
   {
      SDB_ASSERT(isValid() && r.isValid(), "must be valid");
      return compareSlicePairs(_prefix, getComparableSuffix(),
                               r._prefix, r.getComparableSuffix());
   }

   keyString prefixedKeyString::getKeyString(UINT32 bufferSize, CHAR *buffer) const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != buffer && getTotalSize() <= bufferSize, "can not be invalid");
      keyString ks;
      if (_prefix.isValid())
      {
         ossMemcpy(buffer, _prefix.data(), _prefix.size());
      }
      ossMemcpy(buffer + _prefix.size(), _suffix.data(), _suffix.size());
      ks._ref.reset(getTotalSize(), buffer);
      ks._desc = _desc;
      return ks;
   }

   keyString prefixedKeyString::getOwnedKeyString() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      utilPoolAllocator allocator;
      keyString ks;
      UINT32 bufferSize = getTotalSize();
      CHAR *buffer = (CHAR *)allocator.malloc(bufferSize);
      if (OSS_UNLIKELY(nullptr == buffer))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
      }
      else
      {
         if (_prefix.isValid())
         {
            ossMemcpy(buffer, _prefix.data(), _prefix.size());
         }
         ossMemcpy(buffer + _prefix.size(), _suffix.data(), _suffix.size());
         ks._ref.reset(bufferSize, buffer);
         ks._desc = _desc;
         ks._bufferOwned = buffer;
         ks._bufferSize = bufferSize;
      }

      return std::move(ks);
   }

   ossPoolString prefixedKeyString::getConcatenatedString() const
   {
      ossPoolString s;
      if (isValid())
      {
         s.reserve(getTotalSize());
         if (_prefix.isValid())
         {
            s.append(_prefix.data(), _prefix.size());
         }
         s.append(_suffix.data(), _suffix.size());
      }
      return std::move(s);
   }

   INT32 prefixedKeyString::compare(const slice &s) const
   {
      SDB_ASSERT(isValid() && s.isValid(), "can not be invalid");
      return 0 - s.compare(_prefix, getComparableSuffix());
   }
} // namespace vessel
} // namespace engine