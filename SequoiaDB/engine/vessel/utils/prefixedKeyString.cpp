/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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
#include "vessel/bytesReader.h"
#include "vessel/keyString.h"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   prefixedKeyString::prefixedKeyString(const slice &suffix) : _suffix(suffix)
   {
      keyStringDescriptor desc;
      if(SDB_OK != keyString::parseMetaFromSlice(_suffix, desc))
      {
         reset();
      }
      _comparableSize = desc.keySize;
   }

   prefixedKeyString::prefixedKeyString(const slice &suffix,
                                        const slice &prefix)
       : _suffix(suffix), _prefix(prefix)
   {
      keyStringDescriptor desc;
      if(SDB_OK != keyString::parseMetaFromSlice(_suffix, desc))
      {
         reset();
      }
      _comparableSize = desc.keySize;
   }

   void prefixedKeyString::reset()
   {
      _prefix.reset();
      _suffix.reset();
   }

   BOOLEAN prefixedKeyString::isValid() const
   {
      return _suffix.isValid();
   }

   BOOLEAN prefixedKeyString::hasPrefix() const
   {
      return _prefix.isValid();
   }

   CHAR prefixedKeyString::operator[](UINT32 pos)
   {
      if (pos < _prefix.size())
      {
         return _prefix.data()[pos];
      }
      else
      {
         return _suffix.data()[pos];
      }
   }

   INT32 comparePrefixedWithNot(slice prefixLeft,
                                slice suffixLeft,
                                UINT32 comparableSizeLeft,
                                slice suffixRight,
                                UINT32 comparableSizeRight)
   {
      if (prefixLeft.size() > comparableSizeRight)
      {
         if (prefixLeft.compare(
                 slice(comparableSizeRight, suffixRight.data())) < 0)
         {
            return -1;
         }
         else
         {
            return +1;
         }
      }
      else
      {
         UINT32 prefixLen = prefixLeft.size();
         INT32 x = ossMemcmp(prefixLeft.data(), suffixRight.data(), prefixLen);
         if (x != 0)
         {
            return x;
         }
         else
         {
            slice remainLeftSuffix(comparableSizeLeft - prefixLen,
                                   suffixLeft.data());
            slice remainRightSuffix(comparableSizeRight - prefixLen,
                                    suffixRight.data() + prefixLen);
            x = remainLeftSuffix.compare(remainRightSuffix);
            return x;
         }
      }
   }

   INT32 prefixedKeyString::compare(const prefixedKeyString &r) const
   {
      SDB_ASSERT(isValid() && r.isValid(), "must be valid");
      if (!hasPrefix() && !r.hasPrefix())
      {
         return comparableSuffixSlice().compare(r.comparableSuffixSlice());
      }
      else if (hasPrefix() && !r.hasPrefix())
      {
         return comparePrefixedWithNot(
             _prefix, _suffix, _comparableSize, r._suffix, r._comparableSize);
      }
      else if (!hasPrefix() && r.hasPrefix())
      {
         return -r.compare(*this);
      }
      else // hasPrefix() && r.hasPrefix()
      {
         if (_prefix.size() < r._prefix.size())
         {
            return -r.compare(*this);
         }
         UINT32 minLen = r._prefix.size();
         INT32 x = ossMemcmp(_prefix.data(), r._prefix.data(), minLen);
         if (x != 0)
         {
            return x;
         }
         if (_prefix.size() == r._prefix.size())
         {
            slice remainLeftSuffix(_comparableSize - minLen, _suffix.data());
            slice remainRightSuffix(r._comparableSize - minLen,
                                    r._suffix.data());
            x = remainLeftSuffix.compare(remainRightSuffix);
            return x;
         }
         UINT32 remainPrefixLen = _prefix.size() - minLen;

         // _prefix.size() > r._prefix.size()
         slice remainLeftPrefix(remainPrefixLen,
                                _prefix.data() + remainPrefixLen);

         return comparePrefixedWithNot(remainLeftPrefix,
                                       _suffix,
                                       _comparableSize - minLen,
                                       r._suffix,
                                       r._comparableSize - minLen);
      }
   }

   slice prefixedKeyString::comparableSuffixSlice() const
   {
      UINT32 remainLen = _comparableSize;
      if (hasPrefix())
      {
         remainLen -= _prefix.size();
      }
      return slice(remainLen, _suffix.data());
   }
} // namespace vessel
} // namespace engine