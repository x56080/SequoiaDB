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

   Source File Name = lsmIndexKey.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmIndexKey.h"

namespace engine
{
namespace vessel
{
   INT32 lsmIdxFullKeySlice::compare(const lsmIdxFullKeySlice &o)const
   {
      INT32 result = 0;
      SDB_ASSERT(isValid() && o.isValid(), "must be valid");
      const lsmIdxFixedKey *lFixedKey = getFixedKey();
      const lsmIdxFixedKey *rFixedKey = o.getFixedKey();

      result = lFixedKey->indexid.compare(rFixedKey->indexid);
      if (0 != result)
      {
         goto done;
      }

      result = ixmKey(getIxmKeyData()).woCompare(ixmKey(o.getIxmKeyData()),
                                                 lFixedKey->ordering.toBsonOrdering());
      if (0 != result)
      {
         goto done;
      }

      result = lFixedKey->rid.compare(rFixedKey->rid);
      if (0 != result)
      {
         goto done;
      }

      /// lsn ordered desc.
      if (lFixedKey->lsn < rFixedKey->lsn)
      {
         result = 1;
      }
      else if (lFixedKey->lsn > rFixedKey->lsn)
      {
         result = -1;
      }
      else
      {
         result = 0;
      }

   done:
      return result;
   }

///////////////////lsmPureKeyEntry
   INT32 lsmPureKeyEntry::compare(const lsmPureKeyEntry &o,
                                  const orderingWrapper &ordering)const
   {
      INT32 result = 0;
      SDB_ASSERT(_key.isValid() && o._key.isValid(), "can not be invalid");
      result = _key.woCompare(o._key, ordering.toBsonOrdering());
      if (0 != result)
      {
         goto done;
      }

      result = _rid.compare(o._rid);
      if (0 != result)
      {
         goto done;
      }

      if (_lsn < o._lsn)
      {
         result = 1;
      }
      else if (_lsn > o._lsn)
      {
         result = -1;
      }
      else
      {
         result = 0;
      }

   done:
      return result;
   }

   INT32 lsmPureKeyEntry::shallowCopy(const rocksdb::Slice &fullKey)
   {
      INT32 rc = SDB_OK;
      reset();
      lsmIdxFullKeySlice ks(fullKey.data(), fullKey.size(), TRUE);
      if (!ks.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _key.assign(ks.getIxmKeyData());
      _rid = ks.getFixedKey()->rid;
      _lsn = ks.getFixedKey()->lsn;
   done:
      return rc;
   error:
      goto done;
   }

///////////////////lsmIdxKeyComparatorImpl
   class lsmIdxKeyComparatorImpl : public rocksdb::Comparator
   {
      public:
         virtual INT32 Compare(const rocksdb::Slice & a,
                               const rocksdb::Slice & b)const override
         {
            INT32 result = 0;
            if (LSM_IDX_MIN_FULL_KEY_SIZE <= a.size() &&
                LSM_IDX_MIN_FULL_KEY_SIZE <= b.size())
            {
               lsmIdxFullKeySlice l(a.data(), a.size(), FALSE);
               lsmIdxFullKeySlice r(b.data(), b.size(), FALSE);
               result = l.compare(r);
            }
            else if (LSM_IDX_BOUNDARY_SIZE <= a.size() &&
                     LSM_IDX_BOUNDARY_SIZE <= b.size())
            {
               const globalIndexID *l = reinterpret_cast<const globalIndexID *>(a.data());
               const globalIndexID *r = reinterpret_cast<const globalIndexID *>(b.data());
               result = l->compare(*r);
            }
            else
            {
               SDB_ASSERT(FALSE, "invalid slice size");
               result = a.size() - b.size();
            }

            return result;
         }

         virtual const char* Name() const override { return "sdb.lsmIdxKeyComparator"; }
         void FindShortestSeparator(std::string*,const rocksdb::Slice&)const override{}
         void FindShortSuccessor(std::string*) const override {}
   };//class lsmIdxKeyComparatorImpl

   const rocksdb::Comparator* lsmIdxKeyComparator()
   {
      static lsmIdxKeyComparatorImpl _lsmIdxKeyComparator;
      return & _lsmIdxKeyComparator;
   }
} // namespace vessel

} // namespace engine
