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

   Source File Name = prefixedKeyString.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/17/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_PREFIXED_KEY_STRING_H_
#define VESSEL_PREFIXED_KEY_STRING_H_

#include "vessel/slice.h"
#include "ossMemPool.hpp"
#include "vessel/keyString.h"

namespace engine
{
namespace vessel
{
   class prefixedKeyString : public SDBObject
   {
      public:
         prefixedKeyString() = default;
         explicit prefixedKeyString(const slice &suffix, const slice &prefix=slice());
         prefixedKeyString(const prefixedKeyString &) = default;
         prefixedKeyString &operator=(const prefixedKeyString &) = default;
         CHAR operator[](UINT32 pos)const;
      public:
         OSS_INLINE BOOLEAN isValid() const {return _suffix.isValid();}
         OSS_INLINE BOOLEAN hasPrefix() const {return _prefix.isValid();}
         OSS_INLINE UINT32 getTotalSize() const {return _suffix.size() + _prefix.size();}
         OSS_INLINE const slice &getPrefix() const {return _prefix;}
         OSS_INLINE const slice &getSuffix() const {return _suffix;}
         INT32 init(const slice &suffix, const slice &prefix=slice());
         void reset();

      public:
         slice getComparableSuffix() const;
         INT32 compare(const prefixedKeyString &r) const;
         INT32 compare(const slice &s) const;

         /// buffer size must be enough to save key string.
         keyString getKeyString(UINT32 bufferSize, CHAR *buffer) const;

         /// return invalid key string if failed to allocate mem.
         keyString getOwnedKeyString() const;

         ossPoolString getConcatenatedString() const;

      private:
         slice _suffix;
         slice _prefix;
         keyStringDescriptor _desc;
   };
} // namespace vessel
} // namespace engine

#endif