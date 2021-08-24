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

   Source File Name = lsmInsertBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSE_LSM_INSERT_BATCH_H_
#define VESSE_LSM_INSERT_BATCH_H_

#include "vessel/lsm/lsmIdxKey.hpp"
#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmIndexValue.hpp"
#include "rocksdb/write_batch.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class lsmInsertBatch : public SDBObject
   {
      public:
         lsmInsertBatch(){}
         ~lsmInsertBatch();
         lsmInsertBatch(const lsmInsertBatch &) = delete;
         lsmInsertBatch &operator=(const lsmInsertBatch &) = delete;

      public:
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _batch.Count();
         }

         INT32 put(const lsmIndexMeta &meta,
                   const lsmKeyEntry &ke,
                   const lsmIndexValue *value=NULL);

         void clear();

         rocksdb::WriteBatch *getBatch()
         {
            return &_batch;
         }

      private:
         rocksdb::WriteBatch _batch;
         memoryBlock _mb;
   };//class lsmInsertBatch
}//namespace vessel
}//namesapce engine

#endif//VESSE_LSM_INSERT_BATCH_H_