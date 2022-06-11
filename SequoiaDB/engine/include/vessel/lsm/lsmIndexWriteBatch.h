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

   Source File Name = lsmIndexWriteBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_LSM_INDEX_WRITE_BATCH_H_
#define VESSEL_LSM_INDEX_WRITE_BATCH_H_

#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/lsm/lsmIndexMeta.hpp"
#include "vessel/lsm/lsmIndexKey.h"
#include "vessel/lsm/lsmIndexValue.hpp"

namespace engine
{
namespace vessel
{
   class lsmIndexWriteBatch : public lsmColumnFamily::writeBatch
   {
      public:
         lsmIndexWriteBatch() = default;
         ~lsmIndexWriteBatch() = default;

      public:
         void open();
         INT32 put(const lsmIndexMeta &meta,
                   const lsmPureKeyEntry &key,
                   const lsmIndexValue &value);

   }; // class lsmIndexWriteBatch   
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_INDEX_WRITE_BATCH_H_