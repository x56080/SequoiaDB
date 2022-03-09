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

   Source File Name = fixedBitset.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FIXED_BITSET_HPP_
#define VESSEL_FIXED_BITSET_HPP_

#include "core.hpp"
#include "oss.hpp"

/// i am not sure about aix, just leave it here. 
#if defined (_LINUX ) || defined (_AIX)
#include <bitset>
#else
#include <boost/dynamic_bitset.hpp>
#endif // defined (_LINUX ) || defined (_AIX)

namespace engine
{
namespace vessel
{
/* according to my test, it is much faster to do searching
   on std::bitset than boost::dynamic_bitset.
   but _Find_xx functions are not in common use on all os platforms.
*/

#if defined (_LINUX ) || defined (_AIX)
   template<UINT32 SET_SIZE>
   class fixedBitset : public SDBObject
   {
      public:
         fixedBitset()
         {
            static_assert(0 < SET_SIZE, "can not be zero");

            /// we use int32 as return value when do find.
            static_assert(SET_SIZE < OSS_SINT32_MAX, "out of bound");
         }
         ~fixedBitset(){}

      public:
         constexpr UINT32 getSize()const {return SET_SIZE;}
         UINT32 getNonzeroBitCount()const
         {
            return _bs.count();
         }

         INT32 findFirst()const
         {
            INT32 pos = _bs._Find_first();
            if (pos == SET_SIZE)
            {
               pos = -1;
            }
            return pos;
         }

         INT32 findNext(UINT32 prev)const
         {
            INT32 pos = _bs._Find_next(prev);
            if (pos == SET_SIZE)
            {
               pos = -1;
            }
            return pos;
         }

         void set(UINT32 pos, BOOLEAN *old=nullptr)
         {
            if (nullptr != old)
            {
               *old = _bs.test(pos);
            }
            _bs.set(pos);
         }

         void clear(UINT32 pos, BOOLEAN *old=nullptr)
         {
            if (nullptr != old)
            {
               *old = _bs.test(pos);
            }
            _bs.reset(pos);
         }

         void setAll()
         {
            _bs.set();
         }

         void clearAll()
         {
            _bs.reset();
         }

         void flip(UINT32 pos, BOOLEAN *old=nullptr)
         {
            if (nullptr != old)
            {
               *old = _bs.test(pos);
            }
            _bs.flip(pos);
         }

         BOOLEAN test(UINT32 pos)const
         {
            return _bs.test(pos);
         }
      private:
         std::bitset<SET_SIZE> _bs;
   };//class fixedBitset

#else
   template<UINT32 SET_SIZE>
   class fixedBitset : public SDBObject
   {
      public:
         fixedBitset()
         {
            static_assert(0 < SET_SIZE, "can not be zero");

            /// we use int32 as return value when do find.
            static_assert(SET_SIZE < OSS_SINT32_MAX, "out of bound");

            _bs.resize(SET_SIZE, FALSE);
         }
         ~fixedBitset()
         {
            _bs.clear();
         }

      public:
         constexpr UINT32 getSize()const {return SET_SIZE;}
         UINT32 getNonzeroBitCount()const
         {
            return _bs.count();
         }

         INT32 findFirst()const
         {
            boost::dynamic_bitset<>::size_type p = _bs.find_first();
            return boost::dynamic_bitset<>::npos == p ?
                   -1 : static_cast<INT32>(p);
         }

         INT32 findNext(UINT32 prev)const
         {
            boost::dynamic_bitset<>::size_type p = _bs.find_next(prev);
            return boost::dynamic_bitset<>::npos == p ?
                   -1 : static_cast<INT32>(p);
         }

         void set(UINT32 pos)
         {
            _bs.set(pos);
         }

         void set(UINT32 pos, BOOLEAN &old)
         {
            old = _bs.test(pos);
            _bs.set(pos);
         }

         void clear(UINT32 pos, BOOLEAN &old)
         {
            old = _bs.test(pos);
            _bs.reset(pos);
         }

         void clear(UINT32 pos)
         {
            _bs.reset(pos);
         }

         void setAll()
         {
            _bs.set();
         }

         void clearAll()
         {
            _bs.reset();
         }

         void flip(UINT32 pos)
         {
            _bs.flip(pos);
         }

         void flip(UINT32 pos, BOOLEAN &old)
         {
            old = _bs.test(pos);
            _bs.flip(pos);
         }

         BOOLEAN test(UINT32 pos)const
         {
            return _bs.test(pos);
         }
      private:
         boost::dynamic_bitset<> _bs;
   };//class fixedBitset
#endif// defined (_LINUX ) || defined (_AIX)
} // namespace vessel

} // namespace engine


#endif//VESSEL_FIXED_BITSET_HPP_
