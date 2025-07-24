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

   Source File Name = lsmIteratorBound.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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