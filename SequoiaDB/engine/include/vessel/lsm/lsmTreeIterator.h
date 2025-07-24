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

   Source File Name = lsmTreeIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_TREE_ITERATOR_H_
#define VESSEL_LSM_TREE_ITERATOR_H_

#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/keyString.h"
#include "vessel/lsm/lsmIteratorBound.h"
#include "vessel/lsm/lsmIndexEntryValue.h"

namespace engine
{
namespace vessel
{
   class indexObject;

   class lsmTreeIterator : public SDBObject
   {
      public:
         lsmTreeIterator() = default;
         ~lsmTreeIterator();
         lsmTreeIterator(const lsmTreeIterator &) = delete;
         lsmTreeIterator &operator=(const lsmTreeIterator &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid() const {return nullptr != _bound;}

         INT32 init(const lsmColumnFamily &cf,
                    const lsmIteratorBound *bound);

         void reset();

         ///WARNING: pointGetOptimized is used to active rocksdb's bloomfilter.
         /// it will not guarantee to return correct entries in range query or
         /// multiple points get!
         INT32 seek(const keyString &ks, BOOLEAN pointGetOptimized=FALSE);

         INT32 seekForPrev(const keyString &ks);

         OSS_INLINE BOOLEAN isReadyToRead() const {return _ks.isValid();}

      public:
         INT32 next(BOOLEAN forward);
         INT32 advance(const keyString &ks, BOOLEAN forPrev=FALSE);
         BOOLEAN isMarkedRemoved() const;
         OSS_INLINE const keyString &getCurrentEntry() const {return _ks;}
         UINT64 getLSN() const;
         DPS_TRANS_ID getTransID() const;
         recordID getRid() const;

      private:
         OSS_INLINE BOOLEAN _isInternalItrReady() const {return nullptr != _itr;}

      private:
         INT32 _reinitInternalItr(BOOLEAN pointGetOptimized);
         void _resetInternalItr();
         INT32 _seek(const keyString &ks, BOOLEAN forPrev);
         INT32 _initCurrentEntry();
         void _resetCurrentEntry();

      private:
         const lsmIteratorBound *_bound = nullptr;
         lsmColumnFamily _cf;
         rocksdb::Iterator *_itr = nullptr;
         keyString _ks;
         lsmIndexEntryValueRef _valueRef;
   };//class lsmTreeIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_TREE_ITERATOR_H_
