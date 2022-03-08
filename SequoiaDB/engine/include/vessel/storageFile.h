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
#include "vessel/storageFileName.h"
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

         INT32 create(const strSlice &dir,
                      const storageFileName &fn,
                      const createStorageFileOptions &options);

         INT32 open(const strSlice &dir,
                    const storageFileName &fn);

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
         OSS_INLINE FILE_TYPE getFileType()const
         {
            return _fileType;
         }
         OSS_INLINE SPACE_TYPE getSpaceType()const
         {
            return _spaceType;
         }
         OSS_INLINE UINT32 getSequence()const
         {
            return _sequence;
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

      protected:
         ossValuePtr getCommonHeaderPtr()const;
         ossValuePtr getUserDefinedHeaderPtr()const;
         ossValuePtr getReservedAreaPtr()const;
         
      private:
         virtual void _close() {return;}
         virtual INT32 _open(BOOLEAN isCreating) {return SDB_OK;}
         virtual void _onHeaderUpdated(const slice &hs) {return;}
      private:
         INT32 createFileAndInit(const strSlice &dir,
                                 const storageFileName &fn,
                                 const createStorageFileOptions &options);

         INT32 openFileHead(const storageFileName &fn);

         INT32 openReservedArea();

         INT32 openFileSegments();

         BOOLEAN validateOptions(const createStorageFileOptions &options)const;
         INT32 initCommonHead(const storageFileName &fn,
                              const createStorageFileOptions &options,
                              CHAR *headBuf);
         INT32 extendFileAndMMap(UINT32 len, ossValuePtr *ptr);

         INT32 validateHead(const void *head, const storageFileName &fn)const;
         UINT32 createChecksum(ossValuePtr headPtr)const;

      private:
         OSS_INLINE UINT32 getMMapSegmentID(UINT32 dataSegmentID)const
         {
            return dataSegmentID + getExtraMmapSegCount();
         }

         static constexpr UINT32 getHeadMMapSegmentCount()
         {
            return 1;
         }

         OSS_INLINE UINT32 getReservedAreaSegCount() const
         {
            return 0 == _headInMem.reservedAreaSize ? 0 : 1;
         }

         OSS_INLINE UINT32 getExtraMmapSegCount()const
         {
            return getHeadMMapSegmentCount() + getReservedAreaSegCount();
         }

         OSS_INLINE UINT32 getSegmentIDFromPageID(PAGE_ID pid)const
         {
            return pid / _headInMem.maxPageCountPerSeg;
         }

      private:
         storageFileHead _headInMem;
         FILE_TYPE _fileType = INVALID_FILE_TYPE;
         SPACE_TYPE _spaceType = INVALID_SPACE_TYPE;
         UINT32 _sequence = 0;
         UINT16 _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
         UINT32 _dataSegmentCount = 0;
   }; // class storageFile
} // namespace vessel
} // namespace engine

#endif // VESSEL_STORAGE_FILE_H_