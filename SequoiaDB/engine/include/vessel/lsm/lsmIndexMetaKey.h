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

   Source File Name = lsmIndexMetaKey.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef VESSEL_LSM_INDEX_META_KEY_H_
#define VESSEL_LSM_INDEX_META_KEY_H_

#include "oss.hpp"
#include "vessel/globalIndexID.h"
#include "rocksdb/slice.h"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
   class lsmIndexMetaKey : public SDBObject
   {
      public:
         lsmIndexMetaKey() = default;
         ~lsmIndexMetaKey() = default;
         lsmIndexMetaKey(const lsmIndexMetaKey &key)
         {
            ossMemcpy(_data, key._data, sizeof(_data));
         }
         lsmIndexMetaKey &operator=(const lsmIndexMetaKey &key)
         {
            ossMemcpy(_data, key._data, sizeof(_data));
            return *this;
         }
         OSS_INLINE INT32 compare(const lsmIndexMetaKey &o)const
         {
            return ossMemcmp(_data, o._data, sizeof(_data));
         }

      public:
         OSS_INLINE rocksdb::Slice getKeySlice() const
         {
            return rocksdb::Slice(_data, sizeof(_data));
         }
         OSS_INLINE const CHAR *getData()const {return _data;}
         OSS_INLINE UINT32 getSize()const {return sizeof(_data);}

      public:
         void reset();

         void init(const globalIndexID &id);

         void initAsUpKey(const globalIndexID &id);

         globalIndexID toGlobalIndexId()const;

      private:
         /// big endian store
         /// 0-3: CS Logical ID
         /// 4-7: CL Logical ID
         /// 8-11: Index Logical ID
         CHAR _data[12] = {};
   };

   class lsmIndexManifestKey : public SDBObject
   {
      public:
         lsmIndexManifestKey() = default;
         ~lsmIndexManifestKey() = default;
         lsmIndexManifestKey(const lsmIndexManifestKey &key)
         {
            ossMemcpy(_data, key._data, sizeof(_data));
         }
         lsmIndexManifestKey &operator=(const lsmIndexManifestKey &key)
         {
            ossMemcpy(_data, key._data, sizeof(_data));
            return *this;
         }

      public:
         OSS_INLINE rocksdb::Slice getKeySlice() const
         {
            return rocksdb::Slice(_data, sizeof(_data));
         }

      public:
         void reset();

         void init(UINT32 csLid);

         void initAsUpKey(UINT32 csLid);

      private:
         /// big endian store
         /// 0-3: CS Logical Id
         CHAR _data[4] = {};
   };

   class lsmCLIdKey : public SDBObject
   {
      public:
         lsmCLIdKey() = default;
         ~lsmCLIdKey() = default;
         lsmCLIdKey(const lsmCLIdKey &key)
         {
            ossMemcpy(_data, key._data, sizeof(_data));
         }
         lsmCLIdKey &operator=(const lsmCLIdKey &key)
         {
            ossMemcpy(_data, key._data, sizeof(_data));
            return *this;
         }

      public:
         OSS_INLINE rocksdb::Slice getKeySlice() const
         {
            return rocksdb::Slice(_data, sizeof(_data));
         }

      public:
         void reset();

         void init(UINT32 csLid, UINT32 clLid);

         void initAsUpKey(UINT32 csLid, UINT32 clLid);

      private:
         /// big endian store
         /// 0-3: CS Logical Id
         /// 4-7: CL Logical Id
         CHAR _data[8] = {};
   };
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_INDEX_META_KEY_H_
