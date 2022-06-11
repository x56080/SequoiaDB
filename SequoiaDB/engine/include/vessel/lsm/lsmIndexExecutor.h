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

   Source File Name = lsmIndexExecutor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_LSM_INDEX_EXECUTOR_H_
#define VESSEL_LSM_INDEX_EXECUTOR_H_

#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmIndexKey.h"

namespace engine
{
namespace vessel
{
   class lsmIndexExecutor : public SDBObject
   {
      public:
         lsmIndexExecutor() = default;
         ~lsmIndexExecutor() = default;
         lsmIndexExecutor(const lsmIndexExecutor&) = delete;
         lsmIndexExecutor &operator=(const lsmIndexExecutor&) = delete;
      
      public:
         void init(const lsmIndexMeta &meta);
         void fini();

         INT32 put(const lsmPureKeyEntry &key);
         INT32 truncate();

         OSS_INLINE BOOLEAN isValid()const
         {
            return _cf.isValid() &&
                   _meta.isValid();
         }

      private:
         lsmColumnFamily _cf;
         lsmIndexMeta _meta;
         rocksdb::WriteOptions _wOpt;
         rocksdb::ReadOptions _rOpt;
      
   }; // class lsmIndexExecutor

} // namespace vessel
} // namespace engine
#endif // VESSEL_LSM_INDEX_EXECUTOR_H_

