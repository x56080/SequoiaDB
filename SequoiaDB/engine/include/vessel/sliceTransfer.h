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

   Source File Name = sliceTransfer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SLICE_TRANSFER_H_
#define VESSEL_SLICE_TRANSFER_H_

#include "vessel/slice.h"
#include "rocksdb/slice.h"

namespace engine
{
namespace vessel
{
   OSS_INLINE rocksdb::Slice toRocksdbSlice(const slice &o)
   {
      return rocksdb::Slice(o.getData(), o.getSize());
   }

   OSS_INLINE slice toSlice(const rocksdb::Slice &o)
   {
      return slice(o.size(), o.data());
   }
} // namespace vessel

} // namespace engine


#endif//VESSEL_SLICE_TRANSFER_H_