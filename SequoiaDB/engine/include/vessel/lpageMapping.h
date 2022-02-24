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

   Source File Name = lpageMapping.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LPAGE_MAPPING_H_
#define VESSEL_LPAGE_MAPPING_H_

#include "ossRWMutex.hpp"
#include "vessel/lpageDescriptor.h"
#include "ossMemPool.hpp"
#include "vessel/lpageHashTable.h"
#include "vessel/deltaPageList.h"
#include <atomic>

namespace engine
{
namespace vessel
{
   class idMapFile;

   class lpageMapping : public SDBObject
   {
      public:
         lpageMapping();
         ~lpageMapping();
         lpageMapping(const lpageMapping &) = delete;
         lpageMapping &operator=(const lpageMapping &) = delete;

      public:
         OSS_INLINE BOOLEAN isReady()const {return nullptr != _base;}
         OSS_INLINE UINT32 peekDeltaPageCount()const {return _counter.load(std::memory_order_relaxed);}
         OSS_INLINE void resetCounter() {_counter.store(0, std::memory_order_relaxed);}

         void rebase(const idMapFile *base);
         void fini();
         INT32 set(PAGE_ID lpid, const lpageDescriptor &desc);
         INT32 get(PAGE_ID lpid, lpageDescriptor &desc);
         void archive();
         void dumpArchivedTable(ossPoolMap<PAGE_ID, lpageDescriptor> &m);
         void dumpArchivedTable(DELTA_PAGE_LIST &dpl);
         UINT64 getWorkingTableBufferSize()const;

      private:
         INT32 getFromBase(PAGE_ID lpid, lpageDescriptor &desc);

      private:
         ossRWMutex _mutex;
         const idMapFile *_base = nullptr;
         UINT32 _basePageCount = 0;
         lpageHashTable _workingTable;
         lpageHashTable _archivedTable;
         std::atomic_uint _counter = {0};
   };//class lpageMapping
} // namespace vesel

} // namespace engine


#endif//VESSEL_LPAGE_MAPPING_H_