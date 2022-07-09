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

   Source File Name = lsmIndexKeyPacker.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_INDEX_KEY_PACKER_H_
#define VESSEL_LSM_INDEX_KEY_PACKER_H_

#include "vessel/lsm/lsmIndexKey.h"
#include "utilAllocator.hpp"
#include "rocksdb/slice.h"

namespace engine
{
namespace vessel
{
   class lsmIndexKeyPacker : public SDBObject
   {
      public:
         OSS_INLINE UINT32 getFullKeySliceSize(UINT32 ixmKeySize)const
         {
            return LSM_IDX_FIXED_KEY_SIZE + ixmKeySize;
         }

         INT32 pack(const lsmIdxFixedKey &fixedKey,
                    const ixmKey &key,
                    UINT32 bufferSize,
                    CHAR *buffer)const;
   };//class lsmIndexKeyPacker

   class lsmIndexKeyStackPacker : public lsmIndexKeyPacker
   {
      static constexpr UINT32 KEY_PACKER_STACK_BUF_SIZE = 512;

      public:
         lsmIndexKeyStackPacker() = default;
         ~lsmIndexKeyStackPacker();
         lsmIndexKeyStackPacker(const lsmIndexKeyStackPacker &) = delete;
         lsmIndexKeyStackPacker &operator=(const lsmIndexKeyStackPacker &) = delete;

      public:
         void reset();

         INT32 packFullKey(const ixmKey &key,
                           const globalIndexID &idxId,
                           const orderingWrapper &ow,
                           const recordID &rid,
                           UINT64 lsn);

         rocksdb::Slice getFullKeySlice()const;

      private:
         CHAR *_keyBuf = nullptr;
         UINT32 _keySize = 0;
         utilStackAllocator<KEY_PACKER_STACK_BUF_SIZE> _keyAllocator;
   };
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_INDEX_KEY_PACKER_H_