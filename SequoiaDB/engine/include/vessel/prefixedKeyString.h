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

#include "oss.hpp"
#include "vessel/slice.h"
#include "vessel/keyString.h"

namespace engine
{
namespace vessel
{
   class prefixedKeyString : public SDBObject
   {
   public:
      static BOOLEAN calcCommonPrefix(const slice &l,
                                      const slice &r,
                                      prefixedKeyString &outLeft,
                                      prefixedKeyString &outRight);

   public:
      prefixedKeyString() = delete;
      prefixedKeyString(const slice &suffix);
      prefixedKeyString(const slice &suffix, const slice &prefix);
      prefixedKeyString(const prefixedKeyString &) = default;
      prefixedKeyString &operator=(const prefixedKeyString &) = default;
      CHAR operator[](UINT32 pos);
   public:
      void reset();

   public:
      BOOLEAN isValid() const;
      BOOLEAN hasPrefix() const;
      INT32 compare(const prefixedKeyString &r) const;
      slice comparableSuffixSlice() const;
      const slice& getPrefix() const;
      const slice& getSuffix() const;
      keyString getOwnedKeyString() const;
      ossPoolString getConcatenatedString() const;

   private:
      slice _suffix;
      slice _prefix;
      UINT32 _comparableSize = 0;
   };
} // namespace vessel
} // namespace engine

#endif