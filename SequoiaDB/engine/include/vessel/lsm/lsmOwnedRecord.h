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

   Source File Name = lsmOwnedRecord.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LSM_OWNED_RECORD_H_
#define VESSEL_LSM_OWNED_RECORD_H_

#include "vessel/lsm/lsmIndexKey.h"
#include "vessel/lsm/lsmIndexValue.hpp"
#include "ixmKey.hpp"
#include "rocksdb/slice.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class lsmOwnedRecord : public SDBObject
   {
      public:
         lsmOwnedRecord();
         ~lsmOwnedRecord();

         lsmOwnedRecord(const lsmOwnedRecord &) = delete;
         lsmOwnedRecord &operator=(const lsmOwnedRecord &) = delete;

      public:
         INT32 init(const rocksdb::Slice &k,
                    const rocksdb::Slice &v);

         void fini();

         OSS_INLINE BOOLEAN isValid()const
         {
            return _key.isValid();
         }
      
         OSS_INLINE const lsmPureKeyEntry &getKey()const
         {
            return _key;
         }
         OSS_INLINE const lsmIndexValue &getValue()const
         {
            return _value;
         }

         INT32 copy(const lsmOwnedRecord &o);
      private:
         lsmPureKeyEntry _key;
         lsmIndexValue _value;
         memoryBlock _mb;
   };//class lsmOwnedRecord
}//namespace vessel
}//namespace engine

#endif//VESSEL_LSM_OWNED_RECORD_H_