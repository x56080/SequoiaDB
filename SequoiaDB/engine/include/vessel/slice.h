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

   Source File Name = slice.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SLICE_H_
#define VESSEL_SLICE_H_

#include "oss.hpp"
#include "ossTypes.hpp"

namespace engine
{
namespace vessel
{
   class slice : public SDBObject
   {
      public:
         OSS_INLINE slice(){}
         OSS_INLINE explicit slice(UINT32 size, const void *data):
                             _size(size),
                             _data((const CHAR *)data){}
         OSS_INLINE slice(const slice &r):
                    _size(r._size),
                    _data(r._data){}

         OSS_INLINE ~slice(){}

         OSS_INLINE slice &operator=(const slice &r)
         {
            _size = r._size;
            _data = r._data;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _data && 0 < _size;
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE const CHAR *getData()const
         {
            return _data;
         }
         OSS_INLINE const CHAR *data()const
         {
            return _data;
         }
         
         OSS_INLINE void reset()
         {
            _size = 0;
            _data = NULL;
            return;
         }

         OSS_INLINE void reset(UINT32 size, const void *data)
         {
            _size = size;
            _data = (const CHAR *)data;
            return;
         }
      private:
         UINT32 _size = 0;
         const CHAR *_data = NULL;
   };//class slice

} // namespace vessel
} // namespace engine

#endif // VESSEL_SLICE_H_
