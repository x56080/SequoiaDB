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

   Source File Name = dirtyLobcBufferList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_DIRTY_LOBC_BUFFER_LIST_H_
#define VESSEL_DIRTY_LOBC_BUFFER_LIST_H_

#include "ossTypes.h"
#include "vessel/lobChunkBuffer.h"
#include "vessel/dirtyBufferList.hpp"
#include "ossMemPool.hpp"
#include "dpsDef.hpp"
#include "vessel/lobcFlushList.h"
#include "vessel/vesselIdDef.h"

#include <mutex>   //c++11

namespace engine
{
namespace vessel
{
   class dirtyLobcBufferList : public dirtyBufferList<sharedLobChunkBuffer>
   {
      public:
         dirtyLobcBufferList() = default;
         virtual ~dirtyLobcBufferList() = default;

      public:
         void insert(sharedLobChunkBuffer &buffer);
         void discard(SPACE_ID sid, CL_MB_ID mbid, SHARED_LOBC_BUFFER_LIST &l);

      public:
         ///WARNING: only for pool watcher !!!
         void makeFlushList(UINT64 bufferSizeLimit,
                            lobcFlushList &fl);
   
   };//class dirtyLobcBufferList
} // namespace vessel

} // namespace engine

#endif//VESSEL_DIRTY_LOBC_BUFFER_LIST_H_