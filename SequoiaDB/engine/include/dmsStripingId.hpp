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

   Source File Name = dmsStripingId.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_STRIPING_ID_HPP_
#define SDB_DMS_STRIPING_ID_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pdTrace.hpp"

namespace engine
{
   constexpr INT32 DMS_INVALID_STRIPING_ID = -1;

   class _dmsStripingId : public SDBObject
   {
      public:
         _dmsStripingId(){}
         explicit _dmsStripingId(INT32 v):
         _value(v){}
         ~_dmsStripingId(){}
         _dmsStripingId(const _dmsStripingId &o):
         _value(o._value){}
         _dmsStripingId &operator=(const _dmsStripingId &o)
         {
            _value = o._value;
            return *this;
         }
         _dmsStripingId &operator=(INT32 v)
         {
            _value = v;
            return *this;
         }

         BOOLEAN operator==(const _dmsStripingId &o)const
         {
            return _value == o._value;
         }
         BOOLEAN operator==(INT32 v)const
         {
            return _value == v;
         }

         BOOLEAN operator<(const _dmsStripingId &o)const
         {
            return _value < o._value;
         }
         BOOLEAN operator<=(const _dmsStripingId &o)const
         {
            return _value <= o._value;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const {return 0 <= _value;}
         OSS_INLINE INT32 getValue()const {return _value;}
         static BOOLEAN isValidIdValue(INT32 v) {return 0 <= v;}
         OSS_INLINE void reset()
         {
            _value = DMS_INVALID_STRIPING_ID;
         }

      private:
         INT32 _value = DMS_INVALID_STRIPING_ID;
   };//class class _dmsStripingId

   typedef class _dmsStripingId dmsStripingId;

   class dmsStripingRange : public SDBObject
   {
      public:
         dmsStripingRange(){}
         explicit dmsStripingRange(INT32 low, INT32 high):
                  _low(low), _high(high)
                  {
                     SDB_ASSERT(_low <= _high, "invalid range");
                  }
         ~dmsStripingRange(){}
         dmsStripingRange(const dmsStripingRange &o):
         _low(o._low),
         _high(o._high){}
         dmsStripingRange &operator=(const dmsStripingRange &o)
         {
            _low = o._low;
            _high = o._high;
            return *this;
         }
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _low.isValid() && _high.isValid() &&
                   _low <= _high;
         }
         OSS_INLINE const dmsStripingId &getLow()const {return _low;}
         OSS_INLINE const dmsStripingId &getHigh()const {return _high;}

         OSS_INLINE BOOLEAN contains(const dmsStripingId &striping)const
         {
            SDB_ASSERT(isValid() && striping.isValid(), "can not be invalid");
            return _low <= striping && striping <= _high;
         }

         OSS_INLINE UINT32 getStripingCount()const
         {
            SDB_ASSERT(isValid(), "can not be invalid");
            return _high.getValue() - _low.getValue() + 1;
         }

         OSS_INLINE void reset()
         {
            _low.reset();
            _high.reset();
         }

      private:
         dmsStripingId _low;
         dmsStripingId _high;
   };
} // namespace engine


#endif//SDB_DMS_STRIPING_ID_HPP_