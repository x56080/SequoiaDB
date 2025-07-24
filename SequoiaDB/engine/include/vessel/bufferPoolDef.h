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

   Source File Name = bufferPoolDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BUFFER_POOL_DEF_H_
#define VESSEL_BUFFER_POOL_DEF_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   enum class BUFFER_STATUS : UINT8
   {
      INVALID = 0,
      NORMAL,
      RECYCLING,
      DISCARDED,
   };//enum class BUFFER_STATUS

   typedef UINT32 BUFFER_CTL_FLAG_WORD;

   struct LOBC_BUFFER_CTL_FLAGS
   {
      static constexpr BUFFER_CTL_FLAG_WORD BUSY = 0x01;
      static constexpr BUFFER_CTL_FLAG_WORD IN_DIRTY_LIST = 0x02;
      static constexpr BUFFER_CTL_FLAG_WORD PENDING_FLUSH = 0x04;
   };//struct BUFFER_CTL_FLAGS

   struct LITE_IO_BUFFER_CTL_FLAGS
   {
      static constexpr BUFFER_CTL_FLAG_WORD DIRTY = 0x01;
      static constexpr BUFFER_CTL_FLAG_WORD PENDDING_FLUSH = 0x02;
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_BUFFER_POOL_DEF_H_
