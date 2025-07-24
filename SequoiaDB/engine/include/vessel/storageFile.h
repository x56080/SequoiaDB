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

   Source File Name = storageFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_STORAGE_FILE_H_
#define VESSEL_STORAGE_FILE_H_

#include "ossMmap.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselIdDef.h"
#include "vessel/storageFileName.h"
#include "vessel/slice.h"
#include "vessel/mmapPagePointer.h"
#include "vessel/strictBuffer.h"
#include "vessel/invalidFileReason.h"

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

         /// return SDB_VESSEL_INVALID_FILE if failed to validate.
         INT32 open(const strSlice &dir,
                    const storageFileName &fn,
                    UINT32 flags,
                    invalidFileReason &reason);

         void destroy();
         void close();

         const CHAR *getFullPath()const;

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

         OSS_INLINE UINT32 getPageCount()const
         {
            return _headInMem.maxPageCountPerSeg * _dataSegmentCount;
         }

         OSS_INLINE UINT32 getMaxPageCountPerSeg()const
         {
            return _headInMem.maxPageCountPerSeg;
         }
         OSS_INLINE UINT32 getPageSize()const
         {
            return _headInMem.pageSize;
         }
         OSS_INLINE BOOLEAN isSegmentMmaped()const
         {
            return 0 != OSS_BIT_TEST(_ctl, storageFileCtlFlag::MMAP_DATA_SEGMENT);
         }

      public:
         INT32 allocateNewSegment();

         INT32 ensureSegmentCount(UINT32 count);

         
         INT32 removeShadowSuffix();
   
         /// will auto update checksum in common header.
         /// file may not be reopen when crashed.
         INT32 updateUserDefinedHead(const slice &h);

         INT32 fsyncFileHead(BOOLEAN sync=TRUE)const;

         INT32 fsync()const;

         INT32 readDataFromPage(PAGE_ID pid,
                                UINT32 offset,
                                UINT32 size,
                                CHAR *data);

         INT32 writeDataToPage(PAGE_ID pid,
                               UINT32 offset,
                               UINT32 size,
                               const CHAR *data);

         INT32 readPages(PAGE_ID pid,
                         UINT32 pcnt,
                         CHAR *data);

         INT32 writePages(PAGE_ID pid,
                          UINT32 pcnt,
                          const CHAR *data);

         /// reserved area not inclusive
         INT32 readData(UINT64 offset, UINT64 size, CHAR *buf);
                  
         ossValuePtr getReservedAreaPtr()const;
      public:/// mmap file only

         INT32 getSegmentPtr(UINT32 seg, ossValuePtr &ptr)const;

         INT32 getPagePtr(PAGE_ID pid, ossValuePtr &ptr)const;

         ossValuePtr getPagePtr(PAGE_ID pid)const;

         INT32 getPagePtr(PAGE_ID, mmapPagePointer &ptr)const;

         INT32 makeReadableBuffer(PAGE_ID pid, strictBuffer &buffer)const;

         INT32 makeWritableBuffer(PAGE_ID pid, strictBuffer &buffer)const;

         INT32 fsyncPage(PAGE_ID pid, BOOLEAN sync=TRUE)const;

         INT32 fsyncSegment(UINT32 segmentId, BOOLEAN sync=TRUE)const;

         /// always begin from the first page in segment.
         INT32 fsyncPagesInSeg(UINT32 segmentId, UINT32 pageCount)const;

      protected:
         ossValuePtr getCommonHeaderPtr()const;
         ossValuePtr getUserDefinedHeaderPtr()const;
         INT32 getReservedAreaMmapSegmentID() const;
         
      private:
         virtual void _close() {return;}
         virtual INT32 _open(BOOLEAN isCreating) {return SDB_OK;}
         virtual void _onHeaderUpdated(const slice &hs) {return;}

         /// the area will not managed by data segment.
         /// file will be auto extended when creating.
         /// must be aligned by page size.
         virtual UINT32 _getReservedAreaSize()const {return 0;}

      private:
         INT32 createFileAndInit(const strSlice &dir,
                                 const storageFileName &fn,
                                 const createStorageFileOptions &options);

         INT32 openFileHead(const storageFileName &fn, invalidFileReason &reason);

         INT32 openReservedArea(invalidFileReason &reason);

         INT32 openFileSegments();

         BOOLEAN validateOptions(const createStorageFileOptions &options)const;
         INT32 initCommonHead(const storageFileName &fn,
                              const createStorageFileOptions &options,
                              CHAR *headBuf);
         INT32 extendFileAndMmap(UINT32 len, ossValuePtr *ptr);
         INT32 extendFile(UINT32 len);

         INT32 validateHead(const void *head,
                            const storageFileName &fn,
                            invalidFileReason &reason)const;
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
         UINT32 _ctl = 0;
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