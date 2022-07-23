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

   Source File Name = indexEntryLocation.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_ENTRY_LOCATION_H_
#define VESSEL_INDEX_ENTRY_LOCATION_H_

#include "ossMemPool.hpp"
#include "vessel/recordID.h"
#include "vessel/slice.h"

#include <memory>

namespace engine
{
namespace vessel
{
   enum class IDX_ENTRY_LOCATION_TYPE : INT32
   {
      BTREE = 0x01,
      LSM = 0x02,
      MERGED = 0x03,
   };

   class indexEntryLocation : public _utilPooledObject
   {
      public:
         indexEntryLocation() = default;
         virtual ~indexEntryLocation() = default;

      public:
         virtual IDX_ENTRY_LOCATION_TYPE getType()const = 0;
         virtual BOOLEAN isValidToLocate()const = 0;
   };
   using IDX_ENTRY_LOCATION_UPTR = std::unique_ptr<indexEntryLocation>;

   class btreeIndexEntryLocation : public indexEntryLocation
   {
      public:
         btreeIndexEntryLocation() = default;
         virtual ~btreeIndexEntryLocation() = default;

      public:
         virtual IDX_ENTRY_LOCATION_TYPE getType()const override
         {
            return IDX_ENTRY_LOCATION_TYPE::BTREE;
         }
         virtual BOOLEAN isValidToLocate()const override
         {
            return !_key.empty();
         }
      public:
         OSS_INLINE const ossPoolString &getKey()const {return _key;}
         OSS_INLINE slice getKeySlice()const
         {
            return slice(_key.size(), _key.data());
         }

      public:
         void init(ossPoolString &&key,
                   UINT32 replayTick,
                   ossPoolVector<UINT64> &&path,
                   RECORD_SLOT_POS pos);

      private:
         ossPoolString _key;
         UINT32 _replayTick = 0;
         ossPoolVector<UINT64> _path;
         RECORD_SLOT_POS _pos = INVALID_RECORD_SLOT_POS;
   };//class btreeIndexEntryLocation

   class lsmIndexEntryLocation : public indexEntryLocation
   {
      public:
         lsmIndexEntryLocation() = default;
         virtual ~lsmIndexEntryLocation() = default;

      public:
         virtual IDX_ENTRY_LOCATION_TYPE getType()const override
         {
            return IDX_ENTRY_LOCATION_TYPE::LSM;
         }
         virtual BOOLEAN isValidToLocate()const override
         {
            return !_key.empty();
         }
      public:
         OSS_INLINE const ossPoolString &getKey()const {return _key;}
         OSS_INLINE slice getKeySlice()const
         {
            return slice(_key.size(), _key.data());
         }

         void init(ossPoolString &&key) {_key = std::move(key);}

         void assign(const slice &key)
         {
            _key.clear();
            if (key.isValid())
            {
               _key.reserve(key.getSize());
               _key.insert(0, key.getData(), key.getSize());
            }
         }

      private:
         ossPoolString _key;
   };//class lsmIndexEntryLocation

   class mergedIndexEntryLocation : public indexEntryLocation
   {
      public:
         mergedIndexEntryLocation() = default;
         virtual ~mergedIndexEntryLocation() = default;

      public:
         virtual IDX_ENTRY_LOCATION_TYPE getType()const override
         {
            return IDX_ENTRY_LOCATION_TYPE::MERGED;
         }

      public:
         void init(IDX_ENTRY_LOCATION_UPTR &&left,
                   IDX_ENTRY_LOCATION_UPTR &&right)
         {
            _left = std::move(left);
            _right = std::move(right);
         }

      private:
         IDX_ENTRY_LOCATION_UPTR _left;
         IDX_ENTRY_LOCATION_UPTR _right;
   };//class mergedIndexEntryLocation
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_ENTRY_LOCATION_H_