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

namespace engine
{
namespace vessel
{
   class keyString : public SDBObject
   {
      public:
         keyString() = default;
         ~keyString() = default;
         keyString(const slice &s):
         _data(s)
         {}
         keyString(const keyString &k):
         _data(k._data)
         {}
         keyString &operator=(const keyString &k)
         {
            _data = k._data;
            return *this;
         }

      public:
         OSS_INLINE void reset()
         {
            _data.reset();
         }

         OSS_INLINE BOOLEAN isValid() const
         {
            return _data.isValid();
         }

         OSS_INLINE const CHAR *getData() const
         {
            return _data.getData();
         }

         OSS_INLINE UINT32 getSize() const
         {
            return _data.getSize();
         }

         OSS_INLINE const slice &getDataSlice() const
         {
            return _data;
         }

      protected:
         slice _data;  
   };

   class keyStringOwned : public keyString
   {
      public:
         keyStringOwned() = default;
         ~keyStringOwned();
         keyStringOwned(const keyStringOwned &k) = delete;
         keyStringOwned &operator=(const keyStringOwned &k) = delete;
         keyStringOwned(keyStringOwned &&k);
         keyStringOwned &operator=(keyStringOwned &&k);

      public:
         INT32 own(const keyString &k);

         INT32 adopt(CHAR *buf,
                     UINT32 bufSize,
                     UINT32 keyStringSize);

         void reset();

      private:
         CHAR *_buf = nullptr;
         UINT32 _bufSize = 0;
   };

} // namespace vessel
} // namespace engine
#endif // VESSEL_KEY_STRING_H_
