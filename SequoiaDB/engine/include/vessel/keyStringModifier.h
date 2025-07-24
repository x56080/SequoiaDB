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

   Source File Name = keyStringModifier.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_KEY_STRING_MODIFIER_H_
#define VESSEL_KEY_STRING_MODIFIER_H_

#include "vessel/keyString.h"
#include "utilAllocator.hpp"

namespace engine
{
namespace vessel
{
   class keyStringModifier : public SDBObject
   {
      public:
         keyStringModifier() = default;
         keyStringModifier(const keyString &src);
         ~keyStringModifier();
         keyStringModifier(const keyStringModifier &) = delete;
         keyStringModifier &operator=(const keyStringModifier &) = delete;

      public:
         void init(const keyString &ks);
         void reset();

         INT32 incRid(BOOLEAN force=FALSE);
         INT32 decRid(BOOLEAN force=FALSE);

         keyString getShallowKeyString()const;

      private:
         INT32 _ensureBuffer(UINT32 size);
      
      private:
         keyString _src;
         CHAR *_buffer = nullptr;
         UINT32 _bufferSize = 0;
         utilStackAllocator<> _allocator;
   };//class keyStringModifier
} // namespace vessel

} // namespace engine


#endif//VESSEL_KEY_STRING_MODIFIER_H_