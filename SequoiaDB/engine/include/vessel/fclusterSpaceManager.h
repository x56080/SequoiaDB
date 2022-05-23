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

   Source File Name = fclusterSpaceManager.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FCLUSTER_SPACE_MANAGER_H_
#define VESSEL_FCLUSTER_SPACE_MANAGER_H_

#include "vessel/pageIdentifier.h"
#include "vessel/storageFileDef.h"
#include "vessel/variableExtentAllocator.h"
#include "vessel/strictBuffer.h"
#include "vessel/metaDataUberBlock.h"

#include <mutex>

namespace engine
{
namespace vessel
{
   class baseMetaDataFile;
   class storageFileCluster;

   class fclusterSpaceManager : public SDBObject
   {
      public:
         fclusterSpaceManager() = default;
         ~fclusterSpaceManager() = default;
         fclusterSpaceManager(const fclusterSpaceManager &) = delete;
         fclusterSpaceManager &operator=(const fclusterSpaceManager &) = delete;

      public:
         OSS_INLINE BOOLEAN isReady()const {return INVALID_PAGE_ID != _entryPid;}

         INT32 init(PAGE_ID smeEntryPid,
                    baseMetaDataFile *mfile,
                    storageFileCluster *fcluster,
                    UINT32 minFreePcntReused = 8);
         void fini();

      public:
         INT32 reserve(PAGE_ID &pid);
         INT32 reserveExtent(UINT32 pcnt, PAGE_ID &pid);

         void release(PAGE_ID pid);
         void releaseExtent(PAGE_ID pid, UINT32 pcnt);

         void releaseBatch(UINT32 size, const PAGE_ID *pids);

      private:
         INT32 _loadFclusterSme();
         INT32 _getSegmentSmeBuffer(UINT32 segmentId, strictBuffer &buffer);

         /// lock out side first
         INT32 _ensureSegmentSmeBuffer(UINT32 segmentId, strictBuffer &buffer);
         /// lock out side first
         INT32 _ensureSmePage(UINT32 pageNo, PAGE_ID &pid);

         /// lock out side first
         INT32 _extendNewDataSegment();

      private:
         std::mutex _mutex;
         PAGE_ID _entryPid = INVALID_PAGE_ID;
         baseMetaDataFile *_mfile = nullptr;
         storageFileCluster *_fcluster = nullptr;
         variableExtentAllocator _allocator;
   };//class fclusterSpaceManager

} // namespace vessel

} // namespace engine


#endif//VESSEL_FCLUSTER_SPACE_MANAGER_H_
