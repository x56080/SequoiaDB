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

   Source File Name = dataPageCluster.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DATA_PAGE_CLUSTER_H_
#define VESSEL_DATA_PAGE_CLUSTER_H_

#include "vessel/pageDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/storageFileDef.h"
#include "ossLatch.hpp"
#include "vessel/inMemBitmap.h"
#include "vessel/mmapPagePointer.h"

namespace engine
{
namespace vessel
{
   class storageFileCreater;
   class storageFileLoader;

   class dataPageCluster : public SDBObject
   {
      public:
         dataPageCluster();
         virtual ~dataPageCluster();
         dataPageCluster(const dataPageCluster &) = delete;
         dataPageCluster &operator=(const dataPageCluster &) = delete;

      public:
         OSS_INLINE const storageCoreArgs &getCoreArgs()const
         {
            return _args;
         }
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _creater;
         }

      public:
         /// set loader as null when create.
         INT32 open(const storageCoreArgs &args,
                    const storageFileCreater *creater,
                    const storageFileLoader *loader,
                    UINT32 freeBound);

         void close();

         void destroy();

         INT32 allocatePage(PAGE_ID &pid);

         INT32 allocatePages(UINT32 count,
                             PAGE_ID *pids);

         INT32 occupyPage(PAGE_ID pid);

         INT32 occupyPages(UINT32 count, const PAGE_ID *pids);

         void releasePages(UINT32 count, const PAGE_ID *pids);

         void releasePage(PAGE_ID pid);

         /// If oldSegmentCount set as valid value,
         /// will extend space only when (segmentCount + *oldSegmentCount) > current segment count
         INT32 extendPageSpace(UINT32 segmentCount,
                               const UINT32 *oldSegmentCount);

         INT32 ensurePidSpace(PAGE_ID pid);
      public:
         virtual INT32 fsyncSegment(UINT32 globalSegmentId)const = 0;

         virtual INT32 fysncPage(PAGE_ID pid)const = 0;

         virtual INT32 getPagePtr(FILE_TYPE type,
                                  PAGE_ID pid,
                                  mmapPagePointer &ptr)const;

         virtual INT32 getDataPagePtr(PAGE_ID pid, mmapPagePointer &ptr)const = 0;

         virtual FILE_TYPE getDataFileType()const = 0;

         virtual UINT32 getTotalSegmentCountAllocated()const = 0;

      private:
         /// loader may be null
         virtual INT32 openFiles(const storageFileLoader *loader) = 0;
         virtual void closeFiles() = 0;
         virtual void destroyFiles() = 0;

         virtual INT32 allocateNewSegment() = 0;
         virtual INT32 ensureSegmentNotSparse(UINT32 globalSegmentId) = 0;
         virtual INT32 isSparseSegment(UINT32 globalSegmentId,
                                       BOOLEAN &isSparse)const = 0;

         virtual BOOLEAN mayBeSparse()const = 0;

         virtual BOOLEAN hasSparseFile()const = 0;
      
      protected:
         OSS_INLINE const storageFileCreater *getCreater()const
         {
            return _creater;
         }

         void _close();

      private:
         INT32 ensureAllPagesNotSparse(UINT32 count, const PAGE_ID *pids);

      private:
         storageCoreArgs _args;
         const storageFileCreater *_creater = NULL;
         ossSpinXLatch _extendingLatch;
         inMemBitmap _allocator;
         UINT32 _segmentCountOnDisk = 0;

   };//class dataPageCluster
}//namespace vessel
}//namespace engine

#endif//VESSEL_DATA_PAGE_CLUSTER_H_