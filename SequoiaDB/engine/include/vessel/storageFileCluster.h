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

   Source File Name = storageFileCluster.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_CLUSTER_H_
#define VESSEL_STORAGE_FILE_CLUSTER_H_

#include "vessel/keepHistoryPointerArray.h"
#include "vessel/storageManifest.h"
#include "vessel/mmapPagePointer.h"

namespace engine
{
namespace vessel
{
   class storageFileLoader;
   class storageFile;

   class storageFileCluster : public SDBObject
   {
      public:
         storageFileCluster();
         ~storageFileCluster();

         storageFileCluster(const storageFileCluster &) = delete;
         storageFileCluster &operator=(const storageFileCluster &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const {return _manifest.isValid();}
         OSS_INLINE const storageFileManifest &getManifest()const {return _manifest;}
         OSS_INLINE const storageCoreArgs &getCoreArgs()const {return _manifest.args;}
         OSS_INLINE SPACE_ID getSid()const {return _manifest.sid;}

         INT32 open(const storageFileManifest &manifest,
                    UINT32 ctlFlags,
                    const storageFileLoader *loader);

         void close();

         void destroy();

      public:
         UINT32 getTotalSegmentCount()const;

         UINT32 getFileCount()const;

         BOOLEAN isOutOfSpace(PAGE_ID pid)const;

         INT32 getFileSpaceId(PAGE_ID pid)const;

         INT32 ensureSegmentCount(UINT32 minSegmentCount);

         INT32 allocateNewSegment(UINT32 count);

         INT32 ensurePage(PAGE_ID pid);

         INT32 read(UINT64 offset, UINT64 size, void *buf);

         INT32 readPages(PAGE_ID pid,
                         UINT32 pcnt,
                         CHAR *data);
                         
         INT32 writePages(PAGE_ID pid,
                          UINT32 pcnt,
                          const CHAR *data);

         INT32 fsyncFile(UINT32 fileId)const;

      public:/// mmap only
         INT32 getPageMmapPtr(PAGE_ID pid, mmapPagePointer &ptr)const;
         ossValuePtr getPageMmapPtr(PAGE_ID pid)const;
         INT32 fsyncSegment(UINT32 globalSegmentId)const;
         INT32 fysncPage(PAGE_ID pid)const;

      private:
         INT32 loadFiles(const storageFileLoader *loader);

         INT32 extendNewSegment();

         INT32 createNewFile();

         UINT32 getFileId(PAGE_ID pid, PAGE_ID *pidInFile=nullptr)const;

      private:
         storageFile *createFilePtr();
         void releaseFilePtr(storageFile *ptr);
      private:
         UINT32 _ctl = 0;
         storageFileManifest _manifest;
         keepHistoryPointerArray _files;
   };//class storageFileCluster
} // namespace vessel

} // namespace engine


#endif//VESSEL_STORAGE_FILE_CLUSTER_H_