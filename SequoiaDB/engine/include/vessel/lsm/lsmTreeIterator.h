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

   Source File Name = lsmTreeIterator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
   };//class lsmTreeIterator
} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_TREE_ITERATOR_H_
