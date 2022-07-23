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
         keyStringModifier(const keyString &src);
         ~keyStringModifier();
         keyStringModifier(const keyStringModifier &) = delete;
         keyStringModifier &operator=(const keyStringModifier &) = delete;

      public:
         void reset();

         INT32 incRid(BOOLEAN force=FALSE);
         INT32 decRid(BOOLEAN force=FALSE);

         keyString getShallowKeyString()const;

      private:
         INT32 _ensureBuffer(UINT32 size);
      
      private:
         const keyString &_src;
         CHAR *_buffer = nullptr;
         UINT32 _bufferSize = 0;
         utilStackAllocator<> _allocator;
   };//class keyStringModifier
} // namespace vessel

} // namespace engine


#endif//VESSEL_KEY_STRING_MODIFIER_H_