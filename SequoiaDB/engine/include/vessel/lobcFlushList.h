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

   Source File Name = lobcFlushList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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