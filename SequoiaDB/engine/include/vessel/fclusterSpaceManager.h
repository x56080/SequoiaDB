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

   Source File Name = fclusterSpaceManager.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_FCLUSTER_SPACE_MANAGER_H_
#define VESSEL_FCLUSTER_SPACE_MANAGER_H_

#include "vessel/pageIdentifier.h"
#include "vessel/storageFileDef.h"
#include "vessel/variableExtentAllocator.h"
#include "vessel/strictBuffer.h"
#include "vessel/metaDataUberBlock.h"
#include "vessel/sparseBitmap32.h"

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

         void releaseBatch(const sparseBitmap32 &bm);

         BOOLEAN test(PAGE_ID pid);

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
