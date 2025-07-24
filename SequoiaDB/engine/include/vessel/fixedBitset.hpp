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

   Source File Name = fixedBitset.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

   template<UINT32 SET_SIZE>
   class fixedBitset : public SDBObject
   {
      public:
         fixedBitset()
         {
            static_assert(0 < SET_SIZE, "can not be zero");

            /// we use int32 as return value when do find.
            static_assert(SET_SIZE < OSS_SINT32_MAX, "out of bound");

#if defined (_LINUX ) || defined (_AIX)
            /// do no thing
#else
            _bs.resize(SET_SIZE, FALSE);
#endif
         }
         ~fixedBitset(){}

         fixedBitset(const fixedBitset &o):
         _bs(o._bs)
         {

         }

         fixedBitset &operator=(const fixedBitset &o)
         {
            _bs = o._bs;
            return *this;
         }

      public:
         constexpr UINT32 getSize()const {return SET_SIZE;}
         UINT32 getNonzeroBitCount()const
         {
            return _bs.count();
         }

         BOOLEAN none()const
         {
            return _bs.none();
         }

         BOOLEAN all()const
         {
            /// dynamic_bitset does has all() function.
            return SET_SIZE == _bs.count();
         }

         BOOLEAN any()const
         {
            return _bs.any();
         }

         INT32 findFirst()const
         {
            INT32 pos = -1;
#if defined (_LINUX ) || defined (_AIX)
            pos = _bs._Find_first();
            if (pos == SET_SIZE)
            {
               pos = -1;
            }
#else
            boost::dynamic_bitset<>::size_type t = _bs.find_first();
            pos = (t == boost::dynamic_bitset<>::npos) ?
                  -1 : static_cast<INT32>(t);
#endif
            return pos;
         }

         INT32 findNext(INT32 prev)const
         {
            if (prev < 0)
            {
               return findFirst();
            }
#if defined (_LINUX ) || defined (_AIX)
            else
            {
               INT32 pos = _bs._Find_next(prev);
               if (pos == SET_SIZE)
               {
                  pos = -1;
               }

               return pos;
            }
#else
            else
            {
               boost::dynamic_bitset<>::size_type t = _bs.find_next(prev);
               INT32 pos = (t == boost::dynamic_bitset<>::npos) ?
                           -1 : static_cast<INT32>(t);
               return pos;
            }
#endif
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

         void flipAll()
         {
            _bs.flip();
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
#if defined (_LINUX ) || defined (_AIX)
         std::bitset<SET_SIZE> _bs;
#else
         boost::dynamic_bitset<> _bs;
#endif
   };//class fixedBitset
} // namespace vessel

} // namespace engine


#endif//VESSEL_FIXED_BITSET_HPP_
