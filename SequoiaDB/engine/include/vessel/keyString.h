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

   Source File Name = keyString.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/15/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_KEY_STRING_H_
#define VESSEL_KEY_STRING_H_

#include "vessel/slice.h"

#include <memory>

namespace engine
{
namespace vessel
{
   class keyString : public SDBObject
   {
      public:
         class holder : public SDBObject
         {
            public:
               holder() = delete;
               explicit holder(CHAR *buffer, UINT32 bufferSize);
               ~holder();
               holder(const holder &) = delete;
               holder &operator=(const holder &) = delete;

            public:
               OSS_INLINE const CHAR *getBuffer()const {return _buffer;}
               OSS_INLINE UINT32 getBufferSize()const {return _bufferSize;}
               
            private:
               CHAR *_buffer = nullptr;
               UINT32 _bufferSize = 0;
         };//class holder

         using KEY_STRING_HOLER = std::shared_ptr<holder>;

         static KEY_STRING_HOLER makeHolder(CHAR *buffer, UINT32 bufSize);

      public:
         keyString() = default;
         ~keyString() = default;
         explicit keyString(const slice &s);
         explicit keyString(KEY_STRING_HOLER &&holder, UINT32 size);
         keyString(const keyString &);
         keyString &operator=(const keyString &);
         keyString(keyString &&);
         keyString &operator=(keyString &&);

      public:
         OSS_INLINE void reset()
         {
            _ref = slice();
            _holder.reset();
         }
         OSS_INLINE BOOLEAN isValid() const
         {
            return _ref.isValid();
         }
         OSS_INLINE BOOLEAN isOwned()const {return !!_holder;}

         OSS_INLINE const slice &getDataSlice() const
         {
            return _ref;
         }

         INT32 getOwned();
      protected:
         slice _ref;  
         KEY_STRING_HOLER _holder;
   };//class keyString

} // namespace vessel
} // namespace engine
#endif // VESSEL_KEY_STRING_H_
