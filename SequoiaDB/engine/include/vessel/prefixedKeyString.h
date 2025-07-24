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