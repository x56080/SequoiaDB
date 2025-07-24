/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sliceTransfer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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