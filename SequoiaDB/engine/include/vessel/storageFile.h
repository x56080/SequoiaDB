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

   Source File Name = storageFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_FILE_H_
#define VESSEL_STORAGE_FILE_H_

#include "ossMmap.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileName.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class storageFile : public ossMmapFile
   {
      public:
         storageFile();
         virtual ~storageFile();

         storageFile(const storageFile &o) = delete;
         storageFile &operator=(const storageFile &o) = delete;
      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return ossMmapFile::_file.isOpened();
         }
         OSS_INLINE BOOLEAN hasShadowSuffix()const
         {
            return INVALID_FILE_SHADOW_SUFFIX != _shadowSuffix;
         }
         OSS_INLINE UINT64 getTotalSegmentSize()const
         {
            return (UINT64)_dataSegmentCount * _headInMem.getSegmentSize();
         }

         INT32 create(const strSlice &dir,
                      const vesselFileName &fn,
                      const createStorageFileOptions &options,
                      const slice &userDefinedHead = slice());

         INT32 open(const strSlice &dir,
                    const vesselFileName &fn);

         void destroy();
         void close();

         const CHAR *getFullPath()const;

      public:
         INT32 allocateNewSegment(ossValuePtr *out=NULL);

         INT32 ensureSegmentCount(UINT32 count);

         INT32 getSegmentPtr(UINT32 seg, ossValuePtr &ptr)const;

         INT32 getPagePtr(PAGE_ID pid, ossValuePtr &ptr)const;

         INT32 fsyncPage(PAGE_ID pid, BOOLEAN sync=TRUE)const;

         INT32 fsyncSegment(UINT32 segmentId, BOOLEAN sync=TRUE)const;

         /// always begin from the first page in segment.
         INT32 fsyncPagesInSeg(UINT32 segmentId, UINT32 pageCount)const;

         INT32 fsyncFileHead(BOOLEAN sync=TRUE)const;

         INT32 fsync()const;

         OSS_INLINE const storageFileHead &getCommonHeadInMem()const
         {
            return _headInMem;
         }
         OSS_INLINE UINT32 getSegmentCount()const
         {
            return _dataSegmentCount;
         }

         OSS_INLINE UINT32 getMaxPageCountPerSeg()const
         {
            return _headInMem.maxPageCountPerSeg;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _headInMem.pageSize;
         }

         INT32 removeShadowSuffix();
   
         /// will auto update checksum in common header.
         /// file may not be reopen when crashed.
         INT32 updateUserDefinedHead(const slice &h);

         INT32 copySemgmentsTo(storageFile *file)const;

      protected:
         
         INT32 getCommonHeadPtr(ossValuePtr &ptr)const;
         INT32 getUserDefinedHeadPtr(ossValuePtr &ptr)const;
      private:
         virtual BOOLEAN validateUserDefinedHead(const void *head)const
         {
            return TRUE;
         }
      private:
         INT32 createFileAndInitHead(const strSlice &dir,
                                     const vesselFileName &fn,
                                     const createStorageFileOptions &options,
                                     const slice &userDefinedHead);

         INT32 openFileHead(const vesselFileName &fn);

         INT32 openFileSegments();

         BOOLEAN validateOptions(const createStorageFileOptions &options)const;
         INT32 initCommonHead(const vesselFileName &fn,
                              const createStorageFileOptions &options,
                              CHAR *headBuf);
         INT32 extendFileAndMMap(UINT32 len, ossValuePtr *ptr);

         INT32 validateHead(const void *head, const vesselFileName &fn)const;
         UINT32 createChecksum(ossValuePtr headPtr)const;

      private:
         OSS_INLINE UINT32 getMMapSegmentID(UINT32 dataSegmentID)const
         {
            return dataSegmentID + getHeadMMapSegmentCount();
         }

         static constexpr UINT32 getHeadMMapSegmentCount()
         {
            return 1;
         }

         OSS_INLINE UINT32 getSegmentIDFromPageID(PAGE_ID pid)const
         {
            return pid / _headInMem.maxPageCountPerSeg;
         }

      private:
         storageFileHead _headInMem;      
         UINT32 _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
         UINT32 _dataSegmentCount = 0;
   }; // class storageFile
} // namespace vessel
} // namespace engine

#endif // VESSEL_STORAGE_FILE_H_