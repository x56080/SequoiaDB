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

   Source File Name = dirtyLobcBufferList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DIRTY_LOBC_BUFFER_LIST_H_
#define VESSEL_DIRTY_LOBC_BUFFER_LIST_H_

#include "vessel/lobChunkBuffer.h"
#include "ossMemPool.hpp"
#include "dpsDef.hpp"
#include "vessel/lobcFlushList.h"

#include <atomic>  //c++11
#include <mutex>   //c++11

namespace engine
{
namespace vessel
{
   class dirtyLobcBufferList : public SDBObject
   {
      public:
         dirtyLobcBufferList(){}
         ~dirtyLobcBufferList(){}
         dirtyLobcBufferList(const dirtyLobcBufferList &) = delete;
         dirtyLobcBufferList &operator=(const dirtyLobcBufferList &) = delete;

      public:
         void clear();
         void upsert(sharedLobChunkBuffer &buffer);
         DPS_LSN_OFFSET peekMinDirtyLSN()const;
         INT64 getTotalBufferSize()const
         {
            return _totalBufferSize.load(std::memory_order_relaxed);
         }

      public:///WARNING: only for pool watcher !!!

         void makeFlushList(UINT64 bufferSizeLimit,
                            lobcFlushList &fl);
         void resetFlushLSN();
      
      private:
         void _pushBackToList(sharedLobChunkBuffer &buffer);
         void _pushFrontToList(sharedLobChunkBuffer &buffer);

      private:
         std::mutex _mutex;
         DPS_LSN_OFFSET _minFlushLSN = DPS_INVALID_LSN_OFFSET;
         DPS_LSN_OFFSET _minListLSN = DPS_INVALID_LSN_OFFSET;

         std::atomic_llong _totalBufferSize = {0};

         /// buffer sorted by min dirty lsn in list.
         SHARED_LOBC_BUFFER_LIST _list;
   };//class dirtyLobcBufferList
} // namespace vessel

} // namespace engine

#endif//VESSEL_DIRTY_LOBC_BUFFER_LIST_H_