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

   Source File Name = storageFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageFile.h"
#include "pdTrace.hpp"
#include "dms.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/vesselIdDef.h"
#include "ossLikely.hpp"
#include "vessel/pageDef.h"
#include "utilStr.hpp"
#include "utilCRC.hpp"

namespace engine
{
namespace vessel
{
   storageFile::storageFile()
   {}

   storageFile::~storageFile()
   {}

   INT32 storageFile::open(const strSlice &dir,
                           const storageFileName &fn,
                           UINT32 flags,
                           invalidFileReason &reason)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "can not be open");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

      reason = invalidFileReason::NONE;

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (dir.empty() || !fn.isValid())
      {
         PD_LOG(PDERROR, "invalid path or name");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (fn.hasShadowSuffix())
      {
         /// Do not open file with shadow suffix.
         /// User should rename it to formal file first.
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = utilBuildFullPath(dir.str(), fn.getFileName(), OSS_MAX_PATHSIZE + 1, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path;%d", rc);
         goto error;
      }

      rc = ossMmapFile::open(fullPath, OSS_READWRITE|OSS_EXCLUSIVE,
                             OSS_RU|OSS_WU|OSS_RG);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file:%s, %d", fullPath, rc);
         goto error;
      }

      _ctl = flags;

      rc = openFileHead(fn, reason);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (0 != _headInMem.reservedAreaSize)
      {
         rc = openReservedArea(reason);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = openFileSegments();
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _open(FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to _open file:%d", rc);
         goto error;
      }
   
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 storageFile::openFileHead(const storageFileName &fn,
                                   invalidFileReason &reason)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(fn.isValid(), "can not be invalid");
      UINT64 fileSize = 0;
      void *headBuf = NULL;

      rc = ossMmapFile::size(fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file[%s] size:%d", fn.getFileName(), rc);
         goto error;
      }
      /// crashed when creating file
      if (fileSize < SOTRAGE_FILE_TOTAL_HEAD_SIZE)
      {
         PD_LOG(PDERROR, "[%s]invalid file size:%lld", fn.getFileName(), fileSize);
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::INVALID_HEADER_SIZE;
         goto error;
      }

      /// map from offset 0
      rc = map(0, SOTRAGE_FILE_TOTAL_HEAD_SIZE, &headBuf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mmap file head:%s, %d", fn.getFileName(), rc);
         goto error;
      }

      rc = validateHead(headBuf, fn, reason);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate file[%s] head: %d",
                ossMmapFile::_fileName, rc);
         goto error;
      }

      _headInMem = *((const storageFileHead *)headBuf);
      _fileType = fn.getFileType();
      _spaceType = fn.getSpaceType();
      _sequence = fn.getSequence();

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::openReservedArea(invalidFileReason &reason)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ossMmapFile::_file.isOpened(), "must be open");
      SDB_ASSERT(getHeadMMapSegmentCount() == ossMmapFile::segmentSize(),
                 "file head must be open");
      SDB_ASSERT(0 < _headInMem.reservedAreaSize, "can not be zero");
      UINT64 mmapOffset = 0;
      UINT64 fileSize = 0;
      UINT32 segmentSize = 0;

      if (_getReservedAreaSize() != _headInMem.reservedAreaSize)
      {
         PD_LOG(PDERROR, "reserved size[%d] does not match the one in head[%d]",
                _getReservedAreaSize(), _headInMem.reservedAreaSize);
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::UNEXPECTED_HEADER_CONTENT;
         goto error;
      }
      
      rc = ossMmapFile::size(fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

      if (fileSize < ((UINT64)SOTRAGE_FILE_TOTAL_HEAD_SIZE + _headInMem.reservedAreaSize))
      {
         PD_LOG(PDERROR, "invalid file size to open reserved area");
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::UNEXPECTED_FILE_SIZE;
         goto error;
      }

      mmapOffset = SOTRAGE_FILE_TOTAL_HEAD_SIZE;
      segmentSize = _headInMem.reservedAreaSize;
      rc = map(mmapOffset, segmentSize, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mmap file[%s] at reserved area",
                ossMmapFile::_fileName, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::openFileSegments()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ossMmapFile::_file.isOpened(), "must be open");
      SDB_ASSERT(getExtraMmapSegCount() == ossMmapFile::segmentSize(),
                 "extra segments must be open");
      UINT64 mmapOffset = 0;
      UINT64 fileSize = 0;
      UINT32 segmentSize = 0;

      rc = ossMmapFile::size(fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

      mmapOffset = SOTRAGE_FILE_TOTAL_HEAD_SIZE + _headInMem.reservedAreaSize;
      segmentSize = _headInMem.maxPageCountPerSeg * _headInMem.pageSize;

      while ((mmapOffset + segmentSize) <= fileSize)
      {
         if (isSegmentMmaped())
         {
            rc = map(mmapOffset, segmentSize, NULL);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to mmap file[%s] segment:%d",
                     ossMmapFile::_fileName, rc);
               goto error;
            }
         }
         mmapOffset += segmentSize;
         ++_dataSegmentCount;
      }

      if (mmapOffset < fileSize)
      {
         /// crashed when extending file, resize it
         UINT64 newFileSize = fileSize - (fileSize - mmapOffset);
         PD_LOG(PDWARNING, "crashed when extending, truncate size to:%lld",
               newFileSize);
         rc = ossTruncateFile(&_file, newFileSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate file:%s, %d", _fileName, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   ossValuePtr storageFile::getCommonHeaderPtr()const
   {
      return 0 < ossMmapFile::segmentSize() ?
             getSegmentInfo(0, NULL, NULL) : 0;
   }

   ossValuePtr storageFile::getUserDefinedHeaderPtr()const
   {
      ossValuePtr ptr = 0;
      ossValuePtr commonPtr = getCommonHeaderPtr();
      if (0 != commonPtr)
      {
         ptr = commonPtr + STORAGE_FILE_COMMON_HEAD_SIZE;
      }
      return ptr;
   }

   INT32 storageFile::getReservedAreaMmapSegmentID() const
   {
      return (0 < _headInMem.reservedAreaSize &&
             getExtraMmapSegCount() <= ossMmapFile::segmentSize()) ?
             getHeadMMapSegmentCount() : -1;
   }

   ossValuePtr storageFile::getReservedAreaPtr()const
   {
      return (0 < _headInMem.reservedAreaSize &&
             getExtraMmapSegCount() <= ossMmapFile::segmentSize()) ?
             getSegmentInfo(getHeadMMapSegmentCount(), NULL, NULL) : 0;
   }

   INT32 storageFile::create(const strSlice &dir,
                             const storageFileName &fn,
                             const createStorageFileOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not recreate file");
      SDB_ASSERT(!fn.hasShadowSuffix(), "do not create file with shadow suffix");

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(dir.empty() ||
                       !fn.isValid() ||
                       fn.hasShadowSuffix()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!validateOptions(options)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(dir.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _ctl = options.flags;

      rc = createFileAndInit(dir, fn, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%d", rc);
         goto error;
      }

      rc = _open(TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to _open file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      destroy();
      goto done;
   }

   INT32 storageFile::createFileAndInit(const strSlice &dir,
                                        const storageFileName &fn,
                                        const createStorageFileOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!dir.empty(), "can not be empty");
      SDB_ASSERT(fn.isValid(), "must be valid");
      CHAR fullPath[OSS_MAX_PATHSIZE+1] = {0};
      ossValuePtr headPtr = 0;
      UINT32 createFlags = OSS_READWRITE|OSS_EXCLUSIVE;
      const CHAR *fileName = NULL;
      storageFileName tmpFn;

      if (options.replaceWhenCreate)
      {
         createFlags |= OSS_REPLACE;
      }
      else
      {
         createFlags |= OSS_CREATEONLY;
      }

      if (!options.createAsTmpFile)
      {
         fileName = fn.getFileName();
      }
      else
      {
          if (!tmpFn.build(fn.getFileType(),
                           fn.getSpaceType(),
                           fn.getSequence(),
                           FILE_SHADOW_SUFFIX_TMP))
         {
            PD_LOG(PDERROR, "failed to build tmp file name");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         fileName = tmpFn.getFileName();
      }

      rc = utilBuildFullPath(dir.str(), fileName, OSS_MAX_PATHSIZE,
                             fullPath) ;

      if (SDB_OK != rc)
      {
         PD_LOG ( PDERROR, "Path+filename are too long: %s, %s", dir.str(),
                  fn.getFileName()) ;
         goto error ;
      }

      /// open file
      rc = ossMmapFile::open(fullPath, createFlags,
                             OSS_RU|OSS_WU|OSS_RG );
      if (SDB_OK != rc)
      {
         PD_LOG ( PDERROR, "Failed to create new file %s, rc=%d", fullPath, rc) ;
         goto error ;
      }

      /// extend file space for file head
      rc = extendFileAndMmap(SOTRAGE_FILE_TOTAL_HEAD_SIZE, &headPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file:%d", rc);
         goto error;
      }
      ossMemset((void *)headPtr, 0, SOTRAGE_FILE_TOTAL_HEAD_SIZE);

      /// init common file head
      rc = initCommonHead(fn, options, (CHAR *)headPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// init user defined file head
      if (options.userDefinedHeader.isValid())
      {
         UINT32 copySize = std::min(options.userDefinedHeader.getSize(),
                                    STORAGE_FILE_USER_DEFINED_HEAD_SIZE);
         ossMemcpy((void *)(headPtr + STORAGE_FILE_COMMON_HEAD_SIZE),
                   options.userDefinedHeader.getData(), copySize);
      }

      if (0 < _getReservedAreaSize())
      {
         ossValuePtr reservedAreaPtr = 0;
         rc = extendFileAndMmap(_getReservedAreaSize(), &reservedAreaPtr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend file:%d", rc);
            goto error;
         }
      }

      /// create checksum
      ((storageFileHead *)headPtr)->headChecksum = createChecksum(headPtr);
      _headInMem = *((const storageFileHead *)headPtr);
      _fileType = fn.getFileType();
      _spaceType = fn.getSpaceType();
      _sequence = fn.getSequence();
      if (options.createAsTmpFile)
      {
         _shadowSuffix = FILE_SHADOW_SUFFIX_TMP;
      }
      else
      {
         fsync();
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   const CHAR *storageFile::getFullPath()const
   {
      return isOpen() ? ossMmapFile::_fileName : NULL;
   }

   void storageFile::destroy()
   {
      _close();
      _ctl = 0;
      _headInMem.reset();
      _fileType = INVALID_FILE_TYPE;
      _spaceType = INVALID_SPACE_TYPE;
      _sequence = 0;
      _dataSegmentCount = 0;
      if (ossMmapFile::_file.isOpened())
      {
         ossMmapFile::unlink();
      }
      _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
      return ;
   }

   void storageFile::close()
   {
      _close();
      _headInMem.reset();
      _fileType = INVALID_FILE_TYPE;
      _spaceType = INVALID_SPACE_TYPE;
      _sequence = 0;
      _dataSegmentCount = 0;
      _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
      ossMmapFile::close();
      return;
   }

   INT32 storageFile::getPagePtr(PAGE_ID pid, ossValuePtr &ptr)const
   {
      INT32 rc = SDB_OK;
      UINT32 segID = 0;
      ossValuePtr segPtr = 0;
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSegmentMmaped()))
      {
         SDB_ASSERT(FALSE, "not a mmap file");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      segID = getSegmentIDFromPageID(pid);
      rc = getSegmentPtr(segID, segPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ptr = segPtr + ((pid % _headInMem.maxPageCountPerSeg) * _headInMem.pageSize);
   done:
      return rc;
   error:
      goto done;
   }

   ossValuePtr storageFile::getPagePtr(PAGE_ID pid)const
   {
      INT32 rc = SDB_OK;
      UINT32 segID = 0;
      ossValuePtr segPtr = 0;
      ossValuePtr pagePtr = 0;
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         goto done;
      }
      else if (OSS_UNLIKELY(!isSegmentMmaped()))
      {
         SDB_ASSERT(FALSE, "not a mmap file");
         goto done;
      }

      segID = getSegmentIDFromPageID(pid);
      rc = getSegmentPtr(segID, segPtr);
      if (SDB_OK != rc)
      {
         goto done;
      }

      pagePtr = segPtr + ((pid % _headInMem.maxPageCountPerSeg) * _headInMem.pageSize);
   done:
      return pagePtr;
   }

   INT32 storageFile::getPagePtr(PAGE_ID pid, mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      ossValuePtr p = 0;
      ptr.reset();
      rc = getPagePtr(pid, p);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ptr.reset(p);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::makeReadableBuffer(PAGE_ID pid, strictBuffer &buffer)const
   {
      INT32 rc = SDB_OK;
      buffer.reset();
      ossValuePtr p = 0;
      rc = getPagePtr(pid, p);
      if (SDB_OK != rc)
      {
         goto error;
      }

      buffer.reset(_headInMem.pageSize, (const void *)p);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::makeWritableBuffer(PAGE_ID pid, strictBuffer &buffer)const
   {
      INT32 rc = SDB_OK;
      buffer.reset();
      ossValuePtr p = 0;
      rc = getPagePtr(pid, p);
      if (SDB_OK != rc)
      {
         goto error;
      }

      buffer.makeWritable(_headInMem.pageSize, (void *)p);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::getSegmentPtr(UINT32 seg, ossValuePtr &ptr)const
   {
      INT32 rc = SDB_OK;
      UINT32 mmapSegID = 0;
      ossValuePtr segPtr = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         PD_LOG(PDERROR, "file has not been open");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSegmentMmaped()))
      {
         SDB_ASSERT(FALSE, "not a mmap file");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      if (_dataSegmentCount <= seg)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      mmapSegID = getMMapSegmentID(seg);
      segPtr = ossMmapFile::getSegmentInfo(mmapSegID, NULL, NULL);
      if (0 == segPtr)
      {
         PD_LOG(PDERROR, "failed to get segment ptr, mmap seg:%d", mmapSegID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ptr = segPtr;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::fsyncPage(PAGE_ID pid, BOOLEAN sync)const
   {
      INT32 rc = SDB_OK;
      UINT32 seg = 0;
      UINT32 offsetInSegment = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSegmentMmaped()))
      {
         SDB_ASSERT(FALSE, "not a mmap file");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      seg = getSegmentIDFromPageID(pid);
      if (OSS_UNLIKELY(_dataSegmentCount <= seg))
      {
         PD_LOG(PDERROR, "fsync out of file range");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      offsetInSegment = ((pid % _headInMem.maxPageCountPerSeg) * _headInMem.pageSize);
      rc = ossMmapFile::flushBlock(getMMapSegmentID(seg),
                                   offsetInSegment,
                                   _headInMem.pageSize, sync);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync pid[%d], rc:%d", pid, rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::fsyncSegment(UINT32 segmentId, BOOLEAN sync)const
   {
      INT32 rc = SDB_OK;
      UINT32 mmapSegId = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(getSegmentCount() <= segmentId))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSegmentMmaped()))
      {
         SDB_ASSERT(FALSE, "not a mmap file");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      mmapSegId = getMMapSegmentID(segmentId);
      rc = ossMmapFile::flush(mmapSegId, sync);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::fsyncPagesInSeg(UINT32 segmentId,
                                      UINT32 pageCount)const
   {
      INT32 rc = SDB_OK;
      UINT32 mmapSegId = 0;
      INT32 len = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(getSegmentCount() <= segmentId))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSegmentMmaped()))
      {
         SDB_ASSERT(FALSE, "not a mmap file");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      mmapSegId = getMMapSegmentID(segmentId);
      if (_headInMem.maxPageCountPerSeg < pageCount)
      {
         pageCount = _headInMem.maxPageCountPerSeg;
      }
      len = pageCount * _headInMem.pageSize;
      
      rc = ossMmapFile::flushBlock(mmapSegId, 0, len, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::fsyncFileHead(BOOLEAN sync)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = ossMmapFile::flush(0, sync);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file head:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::fsync()const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = ossFdatasync(&(ossMmapFile::_file));
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN storageFile::validateOptions(const createStorageFileOptions &options)const
   {
      BOOLEAN r = FALSE;

      if (!options.args.isValid())
      {
         PD_LOG(PDERROR, "invalid file core args");
         goto done;
      }

      if (STORAGE_FILE_USER_DEFINED_HEAD_SIZE < options.userDefinedHeader.getSize())
      {
         PD_LOG(PDERROR, "invalid user defined header size:%d",
                options.userDefinedHeader.getSize());
         goto done;
      }

      if (0 != _getReservedAreaSize() &&
          0 != _getReservedAreaSize() % DMS_PAGE_SIZE64K)
      {
         PD_LOG(PDERROR, "reserved area size must be aligned by 64KB");
         goto done;
      }

      r = TRUE;
         
   done:
      return r;
   }

   INT32 storageFile::initCommonHead(const storageFileName &fn,
                                     const createStorageFileOptions &options,
                                     CHAR *headBuf)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != headBuf, "can not be null");
      storageFileHead *head = (storageFileHead *)headBuf;

      ossMemcpy(head->magicChars, FILE_MAGICAL_CHARS, sizeof(head->magicChars));
      head->version = STORAGE_FILE_HEAD_VERSION;
      ossStrcpy(head->name, fn.getFileName());
      head->headChecksum = 0;
      head->createTime = ossGetCurrentMilliseconds();
      head->fingerprint = ossRand();
      head->secretValue = options.secretValue;
      head->flags = 0;
      head->pageSize = options.args.pageSize;
      head->maxSegmentCountPerFile = options.args.maxSegmentCountPerFile;
      head->maxPageCountPerSeg = options.args.maxPageCountPerSeg;
      head->reservedAreaSize = _getReservedAreaSize();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::extendFile(UINT32 len)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != len, "can not be zero");
      UINT64 originalFileSize = 0;

      rc = ossMmapFile::size(originalFileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

#if defined( _LINUX )
      rc = ossFallocate(&_file, 0, originalFileSize, len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file with fallocate: %s, %d, %d",
                  _fileName, len, rc);
         goto error;
      }
#else
      rc = ossExtendFile(&_file, len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extent file: %s, %d, %d", _fileName, len, rc);
         goto error;
      }
#endif//#if defined( _LINUX )

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::extendFileAndMmap(UINT32 len, ossValuePtr *ptr)
   {
      INT32 rc = SDB_OK;
      UINT64 originalFileSize = 0;
      void *mmapPtr = nullptr;
      
      rc = ossMmapFile::size(originalFileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

      rc = extendFile(len);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file[%s], rc:%d", getFullPath(), rc);
         goto error;
      }

      rc = ossMmapFile::map(originalFileSize, len, &mmapPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mmap: %s, %d", _fileName, rc);
         INT32 trc = ossTruncateFile(&_file, originalFileSize);
         if (SDB_OK != trc)
         {
            PD_LOG(PDSEVERE, "failed to rollback file to orignal size:%s, %lld, %d", _fileName, originalFileSize, rc);
            ossPanic();
         }
         goto error;
      }

      if (NULL != ptr)
      {
         *ptr = (ossValuePtr)mmapPtr;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::allocateNewSegment()
   {
      INT32 rc = SDB_OK;
      UINT32 extendLen = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_dataSegmentCount == _headInMem.maxSegmentCountPerFile)
      {
         PD_LOG(PDERROR, "hit the max value of maxSegmentCountPerFile");
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }

      extendLen = _headInMem.pageSize * _headInMem.maxPageCountPerSeg;
      if (isSegmentMmaped())
      {
         rc = extendFileAndMmap(extendLen, nullptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend and mmap file:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = extendFile(extendLen);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend file:%d", rc);
            goto error;
         }
      }

      ++_dataSegmentCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::ensureSegmentCount(UINT32 count)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (_headInMem.maxSegmentCountPerFile < count)
      {
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }

      while (_dataSegmentCount < count)
      {
         rc = allocateNewSegment();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new segment, current count:%", _dataSegmentCount);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::validateHead(const void *head,
                                   const storageFileName &fn,
                                   invalidFileReason &reason)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != head, "can not be null");
      const storageFileHead *suHead = (const storageFileHead *)head;
      UINT32 checksum = 0;
      storageCoreArgs args;

      if (0 != ossMemcmp(FILE_MAGICAL_CHARS, suHead->magicChars,
                         sizeof(suHead->magicChars)))
      {
         PD_LOG(PDERROR, "invaid magic chars of head");
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::INVLAID_HEADER_MAGIC_CHARS;
         goto error;
      }

      checksum = createChecksum((ossValuePtr)head);
      if (suHead->headChecksum != checksum)
      {
         PD_LOG(PDERROR, "invalid checksum, in file:%u, current:%u",
                suHead->headChecksum, checksum);
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::INVALID_HEADER_CHECKSUM;
         goto error;
      }

      if (STORAGE_FILE_HEAD_VERSION != suHead->version)
      {
         PD_LOG(PDERROR, "invalid su version:%d", suHead->version);
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::UNEXPECTED_HEADER_VERSION;
         goto error;
      }

      if (0 != ossStrcmp(fn.getFileName(), suHead->name))
      {
         PD_LOG(PDERROR, "file name not match:%s,%s", fn.getFileName(), suHead->name);
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::UNEXPECTED_HEADER_CONTENT;
         goto error;
      }

      args.pageSize = suHead->pageSize;
      args.maxPageCountPerSeg = suHead->maxPageCountPerSeg;
      args.maxSegmentCountPerFile = suHead->maxSegmentCountPerFile;
      if (!args.isValid())
      {
         PD_LOG(PDERROR, "invalid storage core args in head");
         rc = SDB_VESSEL_INVALID_FILE;
         reason = invalidFileReason::UNEXPECTED_HEADER_CONTENT;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 storageFile::createChecksum(ossValuePtr headPtr)const
   {
      SDB_ASSERT(0 != headPtr, "can not be null");
      const void *buf = (const void *)((ossValuePtr)headPtr + 8); /// skip some fields in head.
      return utilCRC32(buf, SOTRAGE_FILE_TOTAL_HEAD_SIZE - 8);
   }

   INT32 storageFile::removeShadowSuffix()
   {
      INT32 rc = SDB_OK;
      ossPoolString fullPath;
      std::size_t pos = std::string::npos;
      BOOLEAN reset = FALSE;
      storageFileName fn;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!hasShadowSuffix())
      {
         goto done;
      }

      /// we never append shadow suffix to file name in header.
      /// must extract it from full path.
      fullPath.append(_fileName);
      pos = fullPath.find_last_of(OSS_FILE_SEP);
      if (std::string::npos == pos ||
          fullPath.size() == (pos + 1))
      {
         PD_LOG(PDERROR, "unexpected full path name[%s]", _fileName);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      fullPath.resize(pos + 1);
      fullPath.append(_headInMem.name);
      reset = TRUE;

#if defined( _LINUX ) || defined (_AIX)
      rc = ossRenamePath(_fileName, fullPath.c_str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename file from %s to %s",
                _fileName, fullPath.c_str());
         goto error;
      }

      _shadowSuffix = INVALID_FILE_SHADOW_SUFFIX;
      ossMemcpy(_fileName, fullPath.c_str(), fullPath.size() + 1);
#else
      fn.extract(strSlice(_headInMem.name));
      close();
      /// close will not clear _fileName
      rc = ossRenamePath(_fileName, fullPath.c_str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename file from %s to %s",
                _fileName, fullPath.c_str());
         goto error;
      }

      fullPath.resize(pos + 1);
      rc = open(strSlice(fullPath.c_str(), fullPath.size()), fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reopen dir[%s], name[%s], rc:%d",
                fullPath.c_str(), fn.getFileName(), rc);
         goto error;
      }
#endif// ( _LINUX ) || defined (_AIX)
   done:
      return rc;
   error:
      if (reset)
      {
         close();
      }
      goto done;
   }

   INT32 storageFile::updateUserDefinedHead(const slice &h)
   {
      INT32 rc = SDB_OK;
      ossValuePtr ptr = 0;
      ossValuePtr commonPtr = 0;
      UINT32 checksum = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!h.isValid() ||
               STORAGE_FILE_USER_DEFINED_HEAD_SIZE < h.getSize())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ptr = getUserDefinedHeaderPtr();
      if (0 == ptr)
      {
         PD_LOG(PDERROR, "failed to get head ptr:%d", rc);
         goto error;
      }

      commonPtr = getCommonHeaderPtr();
      if (0 == commonPtr)
      {
         PD_LOG(PDERROR, "failed to get common ptr:%d", rc);
         goto error;
      }

      ossMemset((void *)ptr, 0x00, STORAGE_FILE_USER_DEFINED_HEAD_SIZE);
      ossMemcpy((void *)ptr, h.getData(), h.getSize());

      checksum = createChecksum(commonPtr);
      ((storageFileHead *)commonPtr)->headChecksum = checksum;
      _headInMem.headChecksum = checksum;
      _onHeaderUpdated(h);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::readDataFromPage(PAGE_ID pid,
                                       UINT32 offset,
                                       UINT32 size,
                                       CHAR *data)
   {
      INT32 rc = SDB_OK;
      INT64 seekOffset = 0;
      UINT32 read = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       0 == size ||
                       _headInMem.pageSize < (offset + size) ||
                       nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((_dataSegmentCount * _headInMem.maxPageCountPerSeg) <= pid)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      seekOffset = static_cast<INT64>(SOTRAGE_FILE_TOTAL_HEAD_SIZE) +
                   _headInMem.reservedAreaSize;
      seekOffset += static_cast<INT64>(_headInMem.pageSize) * pid;
      do
      {
         SINT64 readThisLoop = 0;
         rc = ossSeekAndRead(&_file, seekOffset,
                             data + read,
                             static_cast<INT64>(size - read),
                             &readThisLoop);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file[%s], [%lld,%d], rc:%d",
                   getFullPath(), seekOffset, size - read, rc);
            goto error;
         }

         read += readThisLoop;
         seekOffset += readThisLoop;
      } while (read < size);
      

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::writeDataToPage(PAGE_ID pid,
                                      UINT32 offset,
                                      UINT32 size,
                                      const CHAR *data)
   {
      INT32 rc = SDB_OK;
      INT64 seekOffset = 0;
      UINT32 written = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       0 == size ||
                       _headInMem.pageSize < (offset + size) ||
                       nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((_dataSegmentCount * _headInMem.maxPageCountPerSeg) <= pid)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      seekOffset = static_cast<INT64>(SOTRAGE_FILE_TOTAL_HEAD_SIZE) +
                   _headInMem.reservedAreaSize;
      seekOffset += static_cast<INT64>(_headInMem.pageSize) * pid;
      do
      {
         SINT64 writtenThisLoop = 0;
         rc = ossSeekAndWrite(&_file, seekOffset,
                              data + written,
                              static_cast<INT64>(size - written),
                              &writtenThisLoop);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write file[%s], [%lld,%d], rc:%d",
                   getFullPath(), seekOffset, size - written, rc);
            goto error;
         }

         written += writtenThisLoop;
         seekOffset += writtenThisLoop;
      } while (written < size);
      

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::readPages(PAGE_ID pid,
                                UINT32 pcnt,
                                CHAR *data)
   {
      INT32 rc = SDB_OK;
      INT64 seekOffset = 0;
      INT64 read = 0;
      INT64 totalSize = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       0 == pcnt ||
                       nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((_dataSegmentCount * _headInMem.maxPageCountPerSeg) <
               (pid + pcnt))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      totalSize = static_cast<INT64>(_headInMem.pageSize) * pcnt;
      seekOffset = static_cast<INT64>(SOTRAGE_FILE_TOTAL_HEAD_SIZE) +
                   _headInMem.reservedAreaSize;
      seekOffset += static_cast<INT64>(_headInMem.pageSize) * pid;
      do
      {
         SINT64 readThisLoop = 0;
         rc = ossSeekAndRead(&_file, seekOffset,
                             data + read,
                             static_cast<INT64>(totalSize - read),
                             &readThisLoop);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file[%s], [%lld,%d], rc:%d",
                   getFullPath(), seekOffset, totalSize - read, rc);
            goto error;
         }

         read += readThisLoop;
         seekOffset += readThisLoop;
      } while (read < totalSize);
      

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::writePages(PAGE_ID pid,
                                 UINT32 pcnt,
                                 const CHAR *data)
   {
      INT32 rc = SDB_OK;
      INT64 seekOffset = 0;
      INT64 written = 0;
      INT64 totalSize = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       0 == pcnt ||
                       nullptr == data))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if ((_dataSegmentCount * _headInMem.maxPageCountPerSeg) <
               (pid + pcnt))
      {
         SDB_ASSERT(FALSE, "out of bound");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      totalSize = static_cast<INT64>(_headInMem.pageSize) * pcnt;
      seekOffset = static_cast<INT64>(SOTRAGE_FILE_TOTAL_HEAD_SIZE) +
                   _headInMem.reservedAreaSize;
      seekOffset += static_cast<INT64>(_headInMem.pageSize) * pid;
      do
      {
         SINT64 writtenThisLoop = 0;
         rc = ossSeekAndWrite(&_file, seekOffset,
                              data + written,
                              static_cast<INT64>(totalSize - written),
                              &writtenThisLoop);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write file[%s], [%lld,%d], rc:%d",
                   getFullPath(), seekOffset, totalSize - written, rc);
            goto error;
         }

         written += writtenThisLoop;
         seekOffset += writtenThisLoop;
      } while (written < totalSize);
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::readData(UINT64 offset, UINT64 size, CHAR *buf)
   {
      INT32 rc = SDB_OK;
      UINT64 fileSize = 0;
      UINT64 readSize = 0;
      INT64 readOffset = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == size || nullptr == buf))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fileSize = static_cast<UINT64>(_dataSegmentCount) *
                 _headInMem.pageSize * _headInMem.maxPageCountPerSeg;
      if (fileSize < (offset + size))
      {
         SDB_ASSERT(FALSE, "out of bound");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      readOffset = static_cast<INT64>(SOTRAGE_FILE_TOTAL_HEAD_SIZE) +
                   _headInMem.reservedAreaSize;

      do
      {
         SINT64 readThisLoop = 0;
         rc = ossSeekAndRead(&_file, readOffset + readSize,
                             buf + readSize,
                             static_cast<INT64>(size - readSize),
                             &readThisLoop);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file[%s], [%lld,%d], rc:%d",
                   getFullPath(), readOffset + readSize, size - readSize, rc);
            goto error;
         }

         readSize += readThisLoop;
      } while (readSize < size);
      
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel
} // namespace engine