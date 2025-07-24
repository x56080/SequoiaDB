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

   Source File Name = lobcFlushList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_FLUSH_LIST_H_
#define VESSEL_LOBC_FLUSH_LIST_H_

#include "vessel/lobChunkBuffer.h"

namespace engine
{
namespace vessel
{
   class lobcFlushList : public SDBObject
   {
      public:
         lobcFlushList(){}
         ~lobcFlushList(){}
         lobcFlushList(const lobcFlushList &) = delete;
         lobcFlushList &operator=(const lobcFlushList &) = delete;

      public:
         void clear()
         {
            _maxDirtyLSN = DPS_INVALID_LSN_OFFSET;
            _totalBufferSize = 0;
            _dirtyPageCount = 0;
            _list.clear();
         }

         BOOLEAN isEmpty()const
         {
            return _list.empty();
         }

      public:
         DPS_LSN_OFFSET _maxDirtyLSN = DPS_INVALID_LSN_OFFSET;
         UINT64 _totalBufferSize = 0;
         UINT32 _dirtyPageCount = 0;
         SHARED_LOBC_BUFFER_LIST _list;
   };//class lobcFlushList
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOBC_FLUSH_LIST_H_