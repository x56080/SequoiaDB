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

   Source File Name = lsmIndexKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LSM_INDEX_KEY_H_
#define VESSEL_LSM_INDEX_KEY_H_

#include "ixmKey.hpp"
#include "dpsTransID.hpp"
#include "vessel/globalIndexID.h"
#include "dpsDef.hpp"
#include "vessel/orderingWrapper.h"
#include "vessel/slice.h"
#include "vessel/recordID.h"
#include "rocksdb/comparator.h"

namespace engine
{
namespace vessel
{
// SDB LSM Tree Index is inspired by RocksDB key-value storage.
// It serves as a LSM Tree type index manager with MVCC support.
//
// As implemented with a K-V store, each SDB LSM entry( K-V pair ) may serve
// for different purpose, so it requires specific encoding regarding its type.
// There are two types of entry in current desgin:
//
// Key:   IndexID_Flags_LSN_RowID_Ordering_EncodedKey
// Value: Type_Flag_RBSOffset_TransID
//
// Key:
// * IndexID:     sdbIndexID, unique index id, { csID, clID, indexLLID }
// * LSN:         UINT64, sdb log sequence number
// * Flags:       UINT16
// * RowID:       dmsRecordID, record id
// * Ordering:    Ordering, key object( BSON object ) ordering
// * EncodedKey:  ixmKey, key object, the raw data stream
//
// Value:
// * Type:        UINT8 operation type: insert/delete/...
// * Flag:        UINT8, entry flag,
// * RBSOffset:   offset/position in roll back segment ( RBS )
// * TransID:     SDB global transaction number

   constexpr UINT8 LSM_ENTRY_TYPE_INVALID = 0xFF;
   constexpr UINT8 LSM_ENTRY_TYPE_IDX = 0x0;

#pragma pack(4)

   struct lsmIdxFixedKey
   {
      OSS_INLINE void reset()
      {
         indexid.reset();
         flags = 0;
         lsn = DPS_INVALID_LSN_OFFSET;
         rid.reset();
         ordering = orderingWrapper();
         return;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return indexid.isValid() &&
                DPS_INVALID_LSN_OFFSET != lsn &&
                rid.isValid();
      }

      globalIndexID indexid;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      UINT16 flags = 0;
      recordID rid;
      orderingWrapper ordering;
   };//struct lsmIdxFixedKey
   constexpr UINT32 LSM_IDX_FIXED_KEY_SIZE = sizeof(lsmIdxFixedKey);

   static_assert(36 == LSM_IDX_FIXED_KEY_SIZE, "invalid size");

   constexpr UINT32 LSM_IDX_MIN_FULL_KEY_SIZE = LSM_IDX_FIXED_KEY_SIZE + 1;

   typedef globalIndexID LSM_IDX_KEY_BOUNDARY;
   constexpr UINT32 LSM_IDX_BOUNDARY_SIZE = sizeof(LSM_IDX_KEY_BOUNDARY);

   ///WARNING: buffer not owned!
   class lsmIdxFullKeySlice : public SDBObject
   {
      public:
         lsmIdxFullKeySlice() = default;
         ~lsmIdxFullKeySlice() = default;
         explicit lsmIdxFullKeySlice(const CHAR *data,
                                     UINT32 size,
                                     BOOLEAN check=TRUE)
         {
            if (nullptr != data && LSM_IDX_MIN_FULL_KEY_SIZE <= size)
            {
               _data = data;
               _size = size;

               if (check && !getFixedKey()->isValid())
               {
                  reset();
               }
            }
         }

         INT32 compare(const lsmIdxFullKeySlice &o)const;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _data;
         }
         OSS_INLINE void reset()
         {
            _data = nullptr;
            _size = 0;
         }

         OSS_INLINE const lsmIdxFixedKey *getFixedKey()const
         {
            return reinterpret_cast<const lsmIdxFixedKey *>(_data);
         }
         OSS_INLINE const CHAR *getIxmKeyData()const
         {
            return isValid() ?
                   (_data + LSM_IDX_FIXED_KEY_SIZE) : nullptr;
         }
         OSS_INLINE UINT32 getSize()const {return _size;}
         OSS_INLINE const CHAR *getRawData()const {return _data;}
         OSS_INLINE slice toSlice()const {return slice(_size, _data);}
      private:
         const CHAR *_data = nullptr;
         UINT32 _size = 0;
   };//class lsmIdxFullKeySlice

   ///WARNING: buffer not owned!
   class lsmPureKeyEntry : public SDBObject
   {
      public:
         lsmPureKeyEntry() = default;
         ~lsmPureKeyEntry() = default;
         lsmPureKeyEntry(const lsmPureKeyEntry &o):
         _key(o._key),
         _rid(o._rid),
         _lsn(o._lsn){}

         lsmPureKeyEntry &operator=(const lsmPureKeyEntry &o)
         {
            _key.assign(o._key);
            _rid = o._rid;
            _lsn = o._lsn;
            return *this;
         }

         INT32 compare(const lsmPureKeyEntry &o,
                       const orderingWrapper &ordering)const;

      public:
         OSS_INLINE BOOLEAN isValid()const {return _key.isValid();}

         OSS_INLINE void set(const ixmKey &key,
                             const recordID &rid,
                             UINT64 lsn)
         {
            _key.assign(key);
            _rid = rid;
            _lsn = lsn;
         }

         OSS_INLINE void set(const CHAR *ixmKeyData,
                             const recordID &rid,
                             UINT64 lsn)
         {
            _key.assign(ixmKey(ixmKeyData));
            _rid = rid;
            _lsn = lsn;
         }

         OSS_INLINE void reset()
         {
            _key.assign(ixmKey());
            _rid.reset();
            _lsn = DPS_INVALID_LSN_OFFSET;
            return;
         }

         OSS_INLINE const ixmKey &getKey()const {return _key;}
         OSS_INLINE const recordID &getRid()const {return _rid;}
         OSS_INLINE UINT64 getDataLsn()const {return _lsn;}

         INT32 shallowCopy(const rocksdb::Slice &fullKey);

      private:
         ixmKey _key;
         recordID _rid;
         UINT64 _lsn = DPS_INVALID_LSN_OFFSET;
   };//lsmPureKeyEntry

   extern const rocksdb::Comparator* lsmIdxKeyComparator();

#pragma pack()

} // namespace vessel

} // namespace engine


#endif//VESSEL_LSM_INDEX_RECORD_DEF_H_
