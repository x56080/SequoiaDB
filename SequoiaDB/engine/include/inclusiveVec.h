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

   Source File Name = inclusiveVec.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INCLUSIVE_VEC_H_
#define VESSEL_INCLUSIVE_VEC_H_

#include "core.hpp"
#include "oss.hpp"
#include "pdTrace.hpp"

namespace engine
{
   static const UINT32 MAX_INCLUSIVE_VEC_SIZE = 32;
   /// the bit at the position is 1 means inclusive.
   class inclusiveVec : public SDBObject
   {
      public:
         OSS_INLINE inclusiveVec(){}
         OSS_INLINE ~inclusiveVec(){}
         OSS_INLINE inclusiveVec(const inclusiveVec &o):
         _size(o._size),
         _vec(o._vec){}
         OSS_INLINE explicit inclusiveVec(UINT32 size, BOOLEAN inclusive)
         {
            setAll(size, inclusive);
         }
         OSS_INLINE inclusiveVec &operator=(const inclusiveVec &o)
         {
            _size = o._size;
            _vec = o._vec;
            return *this;
         }

      public:

         OSS_INLINE BOOLEAN operator[](UINT32 pos)const
         {
            return isInclusive(pos);
         }

         OSS_INLINE void setAll(UINT32 size, BOOLEAN inclusive)
         {
            SDB_ASSERT(size <= MAX_INCLUSIVE_VEC_SIZE, "out of bound");
            _size = size;
            _vec = 0;

            if (inclusive)
            {
               for (UINT32 i = 0; i < _size; ++i)
               {
                  UINT32 mask = ((UINT32)1 << i);
                  OSS_BIT_SET(_vec, mask);
               }
            }
         }
         OSS_INLINE void setInclusive(UINT32 pos)
         {
            SDB_ASSERT(pos < _size, "out of bound");
            UINT32 mask = ((UINT32)1 << pos);
            OSS_BIT_SET(_vec, mask);
         }
         OSS_INLINE void setExclusive(UINT32 pos)
         {
            SDB_ASSERT(pos < _size, "out of bound");
            UINT32 mask = ((UINT32)1 << pos);
            OSS_BIT_CLEAR(_vec, mask);
         }
         OSS_INLINE void set(UINT32 pos, BOOLEAN inclusive)
         {
            if (inclusive)
            {
               setInclusive(pos);
            }
            else
            {
               setExclusive(pos);
            }
         }
         OSS_INLINE void setSize(UINT32 s)
         {
            SDB_ASSERT(s <= MAX_INCLUSIVE_VEC_SIZE, "out of bound");
            _size = s;
            return;
         }
         OSS_INLINE BOOLEAN allInclusive()const
         {
            BOOLEAN r = TRUE;
            SDB_ASSERT(0 < _size, "can not be invalid");
            for (UINT32 i = 0; i < _size; ++i)
            {
               if (!isInclusive(i))
               {
                  r = FALSE;
                  break;
               }
            }
            return r;
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE BOOLEAN isInclusive(UINT32 pos)const
         {
            SDB_ASSERT(pos < _size, "out of bound");
            UINT32 mask = ((UINT32)1 << pos);
            return 0 != OSS_BIT_TEST(_vec, mask);
         }

      private:
         UINT32 _size = 0;
         UINT32 _vec = 0;
   };//class inclusiveVec
}//namespace engine

#endif//VESSEL_INCLUSIVE_VEC_H_