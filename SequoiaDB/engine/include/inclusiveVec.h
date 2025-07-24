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

   Source File Name = inclusiveVec.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
            setAll(size, inclusive);
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

            reset();
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
         OSS_INLINE void setBatch(INT32 begin, INT32 end, BOOLEAN inclusive)
         {
            for (INT32 i = 0; i <= end; ++i)
            {
               set(i, inclusive);
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