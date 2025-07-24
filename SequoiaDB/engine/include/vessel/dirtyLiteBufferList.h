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

   Source File Name = dirtyLiteBufferList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_DIRTY_LITE_BUFFER_LIST_H_
#define VESSEL_DIRTY_LITE_BUFFER_LIST_H_

#include "vessel/dirtyBufferList.hpp"
#include "vessel/ioBufferControlBlock.h"

namespace engine
{
namespace vessel
{
   class dirtyLiteBufferList : public dirtyBufferList<SHARED_IO_BUFFER_CB>
   {
      public:
         dirtyLiteBufferList() = default;
         virtual ~dirtyLiteBufferList() = default;

      public:
         void insert(SHARED_IO_BUFFER_CB &bcb);

         ///WARNING: only for pool watcher !!!
         /// flush all buffers if maxBufferCount is zero.
         void makeFlushList(UINT32 maxBufferCount,
                            SHARED_IO_BUFFER_CB_LIST &fl,
                            UINT64 &maxLSN);

         void discard(SPACE_ID sid, SHARED_IO_BUFFER_CB_LIST &discarded);
   };//class dirtyLiteBufferList
} // namespace vessel

} // namespace engine


#endif//VESSEL_DIRTY_LITE_BUFFER_LIST_H_