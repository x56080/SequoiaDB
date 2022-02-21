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

   Source File Name = dataStorageFileCluster.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DATA_STORAGE_FILE_CLUSTER_H_
#define VESSEL_DATA_STORAGE_FILE_CLUSTER_H_

#include "vessel/dataPageCluster.h"
#include "vessel/keepHistoryPointerArray.h"
#include "vessel/inMemBitmap.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class storageFile;

   class dataStorageFileCluster : public dataPageCluster
   {
      public:
         dataStorageFileCluster();
         virtual ~dataStorageFileCluster();

      public:
         virtual INT32 open(SPACE_ID sid,
                            SPACE_TYPE type,
                            UINT32 secretValue,
                            const storageFileLoader *loader, 
                            const storageCoreArgs &args,
                            const options &o) override;

         virtual void close()override;

         virtual void destroy()override;

         virtual INT32 allocatePages(UINT32 count,
                                     PAGE_ID *pids)override;

         virtual INT32 occupyPages(UINT32 count,
                                   const PAGE_ID *pids)override;

         virtual void releasePages(UINT32 count,
                                   const PAGE_ID *pids)override;

         virtual INT32 ensureSegmentCount(UINT32 totalSegmentCount)override;

         virtual INT32 ensurePidSpace(PAGE_ID pid)override;

      public:
         virtual INT32 fsyncSegment(UINT32 globalSegmentId)const override;

         virtual INT32 fysncPage(PAGE_ID pid)const override;

         virtual INT32 getDataPagePtr(PAGE_ID pid, mmapPagePointer &ptr)const override; 

         virtual FILE_TYPE getDataFileType()const override
         {
            return FILE_TYPE_DATA_STORAGE;
         }

         virtual UINT32 getTotalSegmentCount() override;

      private:
         INT32 openFiles(const storageFileLoader *loader);

         /// must open file first
         INT32 initAllocator();

         void closeFiles();

         void destroyFiles();

         INT32 _createNewSegment(UINT32 count);

         INT32 createNewFile();

         void _close();

         UINT32 getFileIdByGlobalSegmentId(UINT32 globalSegment,
                                           UINT32 *segmentInFile)const;
         UINT32 getFileIdByGlobalPageId(PAGE_ID pid,
                                        PAGE_ID *pidInFile)const;
      private:
         ossSpinXLatch _latch;
         inMemBitmap _allocator;
         UINT32 _segmentsCreatedEver = 0;
         keepHistoryPointerArray _files;

   };//class dataStorageFileCluster

}//namespace vessel
}//namespace engine

#endif//VESSEL_DATA_STORAGE_FILE_CLUSTER_H_