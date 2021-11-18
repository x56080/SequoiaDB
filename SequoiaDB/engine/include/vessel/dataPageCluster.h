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
#include "vessel/storageFileLoader.h"

namespace engine
{
namespace vessel
{
   class requestContext;
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
            return INVALID_SPACE_ID != _sid;
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }
         OSS_INLINE SPACE_TYPE getSpaceType()const
         {
            return _type;
         }
         OSS_INLINE UINT32 getSecretValue()const
         {
            return _secretValue;
         }

      public:
         INT32 open(requestContext *context,
                    SPACE_TYPE type,
                    UINT32 secretValue,
                    const storageFileLoader *loader, 
                    const storageCoreArgs &args,
                    const inMemBitmap::options &allocator);

         void close();

         void destroy();

         INT32 allocatePage(requestContext *context,
                            PAGE_ID &pid);

         INT32 allocatePages(requestContext *context,
                             UINT32 count,
                             PAGE_ID *pids);

         INT32 occupyPage(requestContext *context,
                          PAGE_ID pid);

         INT32 occupyPages(requestContext *context,
                           UINT32 count,
                           const PAGE_ID *pids);

         void releasePages(UINT32 count,
                           const PAGE_ID *pids);

         void releasePage(PAGE_ID pid);

         /// If oldSegmentCount set as valid value,
         /// will extend space only when (segmentCount + *oldSegmentCount) > current segment count
         INT32 extendPageSpace(requestContext *context,
                               UINT32 segmentCount,
                               const UINT32 *oldSegmentCount);

         INT32 ensurePidSpace(requestContext *context,
                              PAGE_ID pid);
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
         virtual INT32 openFiles(requestContext *context,
                                 const storageFileLoader *loader) = 0;
         virtual void closeFiles() = 0;
         virtual void destroyFiles() = 0;

         virtual INT32 allocateNewSegment(requestContext *context) = 0;
         virtual INT32 ensureSegmentNotSparse(requestContext *context,
                                              UINT32 globalSegmentId) = 0;
         virtual INT32 isSparseSegment(UINT32 globalSegmentId,
                                       BOOLEAN &isSparse)const = 0;

         virtual BOOLEAN mayBeSparse()const = 0;

         virtual BOOLEAN hasSparseFile()const = 0;
      
      protected:
         void _close();

      private:
         INT32 ensureAllPagesNotSparse(requestContext *context,
                                       UINT32 count,
                                       const PAGE_ID *pids);

      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         UINT32 _secretValue = 0;
         storageCoreArgs _args;
         ossSpinXLatch _extendingLatch;
         inMemBitmap _allocator;
         UINT32 _segmentCountOnDisk = 0;

   };//class dataPageCluster
}//namespace vessel
}//namespace engine

#endif//VESSEL_DATA_PAGE_CLUSTER_H_