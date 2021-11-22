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

   Source File Name = storageFile.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
   :_dataSegmentCount(0)
   {}

   storageFile::~storageFile()
   {}

   INT32 storageFile::open(const strSlice &dir,
                           const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "can not be open");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

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
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
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

      rc = openFileHead(fn);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = openFileSegments();
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 storageFile::openFileHead(const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(fn.isValid(), "can not be invalid");
      UINT64 fileSize = 0;
      void *headBuf = NULL;

      rc = ossMmapFile::size(fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }
      /// crashed when creating file
      if (fileSize < SOTRAGE_FILE_TOTAL_HEAD_SIZE)
      {
         PD_LOG(PDERROR, "[%s]invalid file size:%lld", fn.getFileName(), fileSize);
         rc = SDB_VESSEL_CRASHED_WHEN_CREATING;
         goto error;
      }

      rc = map(0, SOTRAGE_FILE_TOTAL_HEAD_SIZE, &headBuf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mmap file head:%s, %d", fn.getFileName(), rc);
         goto error;
      }

      rc = validateHead(headBuf, fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate file[%s] head: %d",
                ossMmapFile::_fileName, rc);
         goto error;
      }

      if (!validateUserDefinedHead((const CHAR *)headBuf + STORAGE_FILE_COMMON_HEAD_SIZE))
      {
         PD_LOG(PDERROR, "failed to validate user defined head of file[%s], rc:%d",
                ossMmapFile::_fileName, rc);
         /// checksum is correct but validation not passed.
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      _headInMem = *((const storageFileHead *)headBuf);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::openFileSegments()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(ossMmapFile::_file.isOpened(), "must be open");
      SDB_ASSERT(getHeadMMapSegmentCount() == ossMmapFile::segmentSize(),
                 "file head must be open");
      UINT64 mmapOffset = 0;
      UINT64 fileSize = 0;
      UINT32 segmentSize = 0;

      rc = ossMmapFile::size(fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

      mmapOffset = SOTRAGE_FILE_TOTAL_HEAD_SIZE;
      segmentSize = _headInMem.maxPageCountPerSeg * _headInMem.pageSize;

      while ((mmapOffset + segmentSize) <= fileSize)
      {
         rc = map(mmapOffset, segmentSize, NULL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to mmap file[%s] segment:%d",
                   ossMmapFile::_fileName, rc);
            goto error;
         }
         mmapOffset += segmentSize;
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

      _dataSegmentCount = ossMmapFile::segmentSize() - getHeadMMapSegmentCount();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::getCommonHeadPtr(ossValuePtr &ptr)const
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (0 == ossMmapFile::segmentSize())
      {
         PD_LOG(PDERROR, "not valid file");
         rc = SDB_INVALIDARG;
         goto error;
      }

      ptr = getSegmentInfo(0, NULL, NULL);
      if (0 == ptr)
      {
         PD_LOG(PDERROR, "invalid head segment");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::getUserDefinedHeadPtr(ossValuePtr &ptr)const
   {
      INT32 rc = SDB_OK;
      ossValuePtr commonHeadPtr = 0;
      rc = getCommonHeadPtr(commonHeadPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ptr = commonHeadPtr + STORAGE_FILE_COMMON_HEAD_SIZE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::create(const strSlice &dir,
                             const vesselFileName &fn,
                             const createStorageFileOptions &options,
                             const slice &userDefinedHead)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not recreate file");
      SDB_ASSERT(!fn.hasShadowSuffix(), "do not create file with shadow suffix");

      if (OSS_UNLIKELY(isOpen()))
      {
         close();
      }

      if (OSS_UNLIKELY(!fn.isValid() ||
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
      else if (OSS_UNLIKELY(STORAGE_FILE_USER_DEFINED_HEAD_SIZE < userDefinedHead.getSize()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(fn.hasShadowSuffix()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = createFileAndInitHead(dir, fn, options, userDefinedHead);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      destroy();
      goto done;
   }

   INT32 storageFile::createFileAndInitHead(const strSlice &dir,
                                            const vesselFileName &fn,
                                            const createStorageFileOptions &options,
                                            const slice &userDefinedHead)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(fn.isValid(), "must be valid");
      CHAR fullPath[OSS_MAX_PATHSIZE+1] = {0};
      UINT32 checksum = 0;
      ossValuePtr headPtr = 0;
      UINT32 createFlags = OSS_READWRITE|OSS_EXCLUSIVE;
      const CHAR *fileName = NULL;
      vesselFileName tmpFn;

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
          if (!tmpFn.build(fn.getSpaceID(),
                           fn.getFileType(),
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
      rc = extendFileAndMMap(SOTRAGE_FILE_TOTAL_HEAD_SIZE, &headPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extent file:%d", rc);
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
      if (userDefinedHead.isValid())
      {
         ossMemcpy((void *)(headPtr + STORAGE_FILE_COMMON_HEAD_SIZE),
                   userDefinedHead.getRPtr(), userDefinedHead.getSize());
      }

      /// create checksum
      rc = createChecksum(headPtr, checksum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checksum");
         goto error;
      }
      ((storageFileHead *)headPtr)->headChecksum = checksum;

      /// do not flush head if create as tmp one.
      if (!options.createAsTmpFile)
      {
         rc = ossMmapFile::flush(0, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fsync file head:%d", rc);
            goto error;
         }
      }

      _headInMem = *((const storageFileHead *)headPtr);
      if (options.createAsTmpFile)
      {
         _shadowSuffix = FILE_SHADOW_SUFFIX_TMP;
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
      _headInMem.reset();
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
      _headInMem.reset();
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

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
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

      r = TRUE;
         
   done:
      return r;
   }

   INT32 storageFile::initCommonHead(const vesselFileName &fn,
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
      head->spaceID = fn.getSpaceID();
      head->spaceType = fn.getSpaceType();
      head->fileType = fn.getFileType();
      head->sequence = fn.getSequence();
      head->pageSize = options.args.pageSize;
      head->maxSegmentCountPerFile = options.args.maxSegmentCountPerFile;
      head->maxPageCountPerSeg = options.args.maxPageCountPerSeg;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::extendFileAndMMap(UINT32 len, ossValuePtr *ptr)
   {
      INT32 rc = SDB_OK;
      UINT64 originalFileSize = 0;
      void *mmapAddr = NULL;
      BOOLEAN needTruncate = FALSE;
   
      if (OSS_UNLIKELY(0 == len))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossMmapFile::size(originalFileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size：%s, %d", _fileName, rc);
         goto error;
      }
      needTruncate = TRUE;

      if (VESSEL_FILE_GLOBAL_OPTIONS::isSparseExtending())
      {
#if defined( _LINUX )
         rc = ossFallocate(&_file, 0, originalFileSize, len);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend file with fallocate: %s, %d, %d",
                   _fileName, len, rc);
            goto error;
         }
#else
         rc = ossExtentBySparse(&_file, len);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend file by sparse:%d", rc);
            goto error;
         }
#endif//#if defined( _LINUX )
      }
      else
      {
         rc = ossExtendFile(&_file, len);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extent file: %s, %d, %d", _fileName, len, rc);
            goto error;
         }
      }

      rc = ossMmapFile::map(originalFileSize, len, &mmapAddr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mmap: %s, %d", _fileName, rc);
         goto error;
      }
      needTruncate = FALSE;

      if (NULL != ptr)
      {
         *ptr = (ossValuePtr)mmapAddr;
      }
   done:
      return rc;
   error:
      if (needTruncate)
      {
         INT32 trc = ossTruncateFile(&_file, originalFileSize);
         if (SDB_OK != trc)
         {
            PD_LOG(PDSEVERE, "failed to rollback file to orignal size:%s, %lld, %d", _fileName, originalFileSize, rc);
            ossPanic();
         }
      }
      goto done;
   }

   INT32 storageFile::allocateNewSegment(ossValuePtr *out)
   {
      INT32 rc = SDB_OK;
      UINT32 extendLen = 0;
      ossValuePtr ptr = 0;

      if (NULL != out)
      {
         *out = 0;
      }

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
      rc = extendFileAndMMap(extendLen, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file:%d", rc);
         goto error;
      }

      ++_dataSegmentCount;
      if (NULL != out)
      {
         *out = ptr;
      }
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
         rc = SDB_INVALIDARG;
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

   INT32 storageFile::validateHead(const void *head, const vesselFileName &fn)const
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
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      rc = createChecksum((ossValuePtr)head, checksum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checksum of head:%d", rc);
         goto error;
      }

      if (suHead->headChecksum != checksum)
      {
         PD_LOG(PDERROR, "invalid checksum, in file:%d, current:%d",
                suHead->headChecksum, checksum);
         rc = SDB_VESSEL_FILE_HEAD_CRASHED;
         goto error;
      }

      if (STORAGE_FILE_HEAD_VERSION != suHead->version)
      {
         PD_LOG(PDERROR, "invalid su version:%d", suHead->version);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (0 != ossStrcmp(fn.getFileName(), suHead->name))
      {
         PD_LOG(PDERROR, "file name not match:%s,%s", fn.getFileName(), suHead->name);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (fn.getSpaceID() != suHead->spaceID)
      {
         PD_LOG(PDERROR, "space id not match:%d,%d", fn.getSpaceID(), suHead->spaceID);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (fn.getSpaceType() != suHead->spaceType)
      {
         PD_LOG(PDERROR, "space type not match :%d, %d",
                fn.getSpaceType(), suHead->spaceType);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (suHead->fileType != fn.getFileType())
      {
         PD_LOG(PDERROR, "file type not match:%d, %d",
                fn.getFileType(), suHead->fileType);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (suHead->sequence != fn.getSequence())
      {
         PD_LOG(PDERROR, "file sequence not match:%lld, %lld",
                fn.getSequence(), suHead->sequence);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      args.pageSize = suHead->pageSize;
      args.maxPageCountPerSeg = suHead->maxPageCountPerSeg;
      args.maxSegmentCountPerFile = suHead->maxSegmentCountPerFile;
      if (!args.isValid())
      {
         PD_LOG(PDERROR, "invalid storage core args in head");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::createChecksum(ossValuePtr headPtr, UINT32 &checksum)const
   {
      SDB_ASSERT(0 != headPtr, "can not be null");
      const void *buf = (const void *)((ossValuePtr)headPtr + 8); /// skip some fields in head.
      return utilCRC32(buf, SOTRAGE_FILE_TOTAL_HEAD_SIZE - 8, checksum);
   }

   INT32 storageFile::removeShadowSuffix()
   {
      INT32 rc = SDB_OK;
      ossPoolString fullPath;
      std::size_t pos = std::string::npos;
      BOOLEAN reset = FALSE;
      vesselFileName fn;

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
      else if (h.isEmpty() ||
               STORAGE_FILE_USER_DEFINED_HEAD_SIZE < h.getSize())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getUserDefinedHeadPtr(ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get head ptr:%d", rc);
         goto error;
      }

      rc = getCommonHeadPtr(commonPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get common ptr:%d", rc);
         goto error;
      }

      ossMemset((void *)ptr, 0x00, STORAGE_FILE_USER_DEFINED_HEAD_SIZE);
      ossMemcpy((void *)ptr, h.data(), h.getSize());

      createChecksum(commonPtr, checksum);
      ((storageFileHead *)commonPtr)->headChecksum = checksum;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFile::copySemgmentsTo(storageFile *file)const
   {
      INT32 rc = SDB_OK;
      UINT32 segmentSize = 0;

      if (OSS_UNLIKELY(NULL == file || !file->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_headInMem.compareCoreArgs(file->_headInMem))
      {
         PD_LOG(PDERROR, "different args found");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      

      rc = file->ensureSegmentCount(getSegmentCount());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend target file:%d", rc);
         goto error;
      }

      segmentSize = _headInMem.getSegmentSize();
      for (UINT32 i = 0; i < getSegmentCount(); ++i)
      {
         ossValuePtr src, dst = 0;
         rc = getSegmentPtr(i, src);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get src ptr:%d", rc);
            goto error;
         }
         rc = file->getSegmentPtr(i, dst);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get dst ptr:%d", rc);
            goto error;
         }

         ossMemcpy((void *)dst, (const void *)src, segmentSize);
      }
   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine