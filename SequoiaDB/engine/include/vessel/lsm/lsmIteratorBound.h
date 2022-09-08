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

   Source File Name = lsmIteratorBound.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LSM_ITERATOR_BOUND_H_
#define VESSEL_LSM_ITERATOR_BOUND_H_

#include "vessel/globalIndexID.h"
#include "vessel/slice.h"
#include "rocksdb/slice.h"

namespace engine
{
namespace vessel
{
   class lsmIteratorBound : public SDBObject
   {
      public:
         lsmIteratorBound() = default;
         ~lsmIteratorBound() = default;
         lsmIteratorBound(const lsmIteratorBound &);
         lsmIteratorBound &operator=(const lsmIteratorBound &);

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _indexId.isValid();
         }
         OSS_INLINE void reset() {_indexId.reset();}

         OSS_INLINE const globalIndexID &getId()const {return _indexId;}

         OSS_INLINE const rocksdb::Slice *getLowBound()const
         {
            return &_low;
         }

         OSS_INLINE const rocksdb::Slice *getUpBound()const
         {
            return &_up;
         }

         INT32 init(const globalIndexID &id);

         slice getEncodedIndexId()const;
      private:
         static constexpr UINT32 _BOUND_BUF_SIZE = 16;
         globalIndexID _indexId;
         CHAR _lowBuf[_BOUND_BUF_SIZE] = {};
         CHAR _upBuf[_BOUND_BUF_SIZE] = {};
         rocksdb::Slice _low{_lowBuf, _BOUND_BUF_SIZE};
         rocksdb::Slice _up{_upBuf, _BOUND_BUF_SIZE};
   };//class lsmIteratorBound
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_ITERATOR_BOUND_H_