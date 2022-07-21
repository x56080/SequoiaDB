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
#include <bitset>

namespace engine
{
   /// the bit at the position is 0 means inclusive.
   class inclusiveVec : public SDBObject
   {
      public:
         inclusiveVec() = default;
         ~inclusiveVec() = default;
         explicit inclusiveVec(UINT32 size, BOOLEAN inclusive)
         {
            if (!inclusive)
            {
               for (UINT32 i = 0; i < size; ++i)
               {
                  _bs.set(i);
               }
            }
         }

      public:
         OSS_INLINE void reset()
         {
            _bs.reset();
         }

         OSS_INLINE BOOLEAN operator[](UINT32 pos)const
         {
            return isInclusive(pos);
         }

         OSS_INLINE void setAll(UINT32 size, BOOLEAN inclusive)
         {
            SDB_ASSERT(size <= _bs.size(), "out of bound");
            _bs.reset();

            if (!inclusive)
            {
               for (UINT32 i = 0; i < size; ++i)
               {
                  _bs.set(i);
               }
            }
         }
         OSS_INLINE void setInclusive(UINT32 pos)
         {
            SDB_ASSERT(pos < _bs.size(), "out of bound");
            _bs.reset(pos);
         }
         OSS_INLINE void setExclusive(UINT32 pos)
         {
            SDB_ASSERT(pos < _bs.size(), "out of bound");
            _bs.set(pos);
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
         OSS_INLINE BOOLEAN allInclusive()const
         {
            return _bs.none();
         }
         OSS_INLINE BOOLEAN isInclusive(UINT32 pos)const
         {
            SDB_ASSERT(pos < _bs.size(), "out of bound");
            return !_bs.test(pos);
         }
         OSS_INLINE BOOLEAN isExclusive(UINT32 pos)const
         {
            SDB_ASSERT(pos < _bs.size(), "out of bound");
            return _bs.test(pos);
         }

      private:
         static constexpr UINT32 _VEC_CAPACITY = 32;
         std::bitset<_VEC_CAPACITY> _bs;
   };//class inclusiveVec
}//namespace engine

#endif//VESSEL_INCLUSIVE_VEC_H_