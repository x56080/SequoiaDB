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

   Source File Name = extentStorageFile.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/extentStorageFile.h"
#include "pdTrace.hpp"
#include "dms.hpp"
#include "vessel/storageFileDef.h"
#include "vessel/vesselDef.h"
#include "ossLikely.hpp"
#include "vessel/extentDef.h"
#include "utilStr.hpp"
#include "utilCRC.hpp"

namespace engine
{
namespace vessel
{
   extentStorageFile::extentStorageFile()
   :_dataSegmentCount(0)
   {}

   extentStorageFile::~extentStorageFile()
   {}

   INT32 extentStorageFile::open(const CHAR *fullPath, const storageFileName &fn)
   {
      INT32 rc = SDB_OK;
 
      if (isOpen())
      {
         PD_LOG(PDERROR, "su has already been open");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (NULL == fullPath || !fn.valid())
      {
         PD_LOG(PDERROR, "invalid path or name");
         rc = SDB_INVALIDARG;
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

      rc = afterHeadOpen();
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
      if (isOpen())
      {
         close();
      }
      goto done;
   }

   INT32 extentStorageFile::openFileHead(const storageFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(fn.valid(), "can not be invalid");
      UINT64 fileSize = 0;
      void *headBuf = NULL;
      const storageFileHead *head = NULL;

      rc = ossMmapFile::size(fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }
      /// crashed when creating file, just waiting for redo
      if (fileSize < STORAGE_FILE_HEAD_SIZE)
      {
         PD_LOG(PDERROR, "[%s]invalid file size:%lld", fn.getName(), fileSize);
         rc = SDB_VESSEL_CRASHED_WHEN_CREATING;
         goto error;
      }

      rc = map(0, STORAGE_FILE_HEAD_SIZE, &headBuf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mmap file head:%s, %d", fn.getName(), rc);
         goto error;
      }

      rc = validateHead(headBuf, fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "validation of su head is not correct: %d", rc);
         /// crashed when file creating
         if (STORAGE_FILE_HEAD_SIZE == fileSize)
         {
            rc = SDB_VESSEL_CRASHED_WHEN_CREATING;
         }
         else
         {
            ///we always guarantee common head fsynced first when creating.
            ///so if file size is larger than common head size, head is broken.
         }   
         goto error;
      }

      head = (const storageFileHead *)headBuf;
      _headInMem = *head;

      if (0 == head->userDefinedHeadLen)
      {
         goto done;
      }

      if (fileSize < (STORAGE_FILE_HEAD_SIZE + head->userDefinedHeadLen))
      {
         rc = SDB_VESSEL_CRASHED_WHEN_CREATING;
         goto error;
      }

      rc = map(STORAGE_FILE_HEAD_SIZE, head->userDefinedHeadLen, &headBuf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to mmap file head:%s, %d", fn.getName(), rc);
         goto error;
      }

      rc = validateUserDefinedHead(headBuf);
      if (SDB_OK != rc)
      {
         if (fileSize == (STORAGE_FILE_HEAD_SIZE + head->userDefinedHeadLen))
         {
            rc = SDB_VESSEL_CRASHED_WHEN_CREATING;
         }
         goto error;
      }

   done:
      return rc;
   error:
      _headInMem = storageFileHead();
      goto done;
   }

   INT32 extentStorageFile::openFileSegments()
   {
      INT32 rc = SDB_OK;
      UINT64 mmapOffset = 0;
      UINT64 fileSize = 0;
      UINT32 segmentSize = 0;
      UINT32 totalHeadSize = 0;

      rc = ossMmapFile::size(fileSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get file size:%d", rc);
         goto error;
      }

      totalHeadSize = STORAGE_FILE_HEAD_SIZE + _headInMem.userDefinedHeadLen;
      mmapOffset = totalHeadSize;
      segmentSize = _headInMem.maxPageCountPerSeg * _headInMem.pageSize;

      while ((mmapOffset + segmentSize) <= fileSize)
      {
         rc = map(mmapOffset, segmentSize, NULL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to mmap file:%d", rc);
            goto error;
         }
         mmapOffset += segmentSize;
      }

      /// crashed when extending file, resize it
      if (0 != ((fileSize - totalHeadSize) % segmentSize))
      {
         PD_LOG(PDWARNING, "crashed when extending, truncate size to:%lld", (mmapOffset - segmentSize));
         rc = ossTruncateFile(&_file, (mmapOffset - segmentSize));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to truncate file:%s, %d", _fileName, rc);
            goto error;
         }
      }

      _dataSegmentCount = ossMmapFile::segmentSize() - getHeadMmapSegCount();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::getFileHeadPtr(ossValuePtr &ptr)
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

   INT32 extentStorageFile::getUserDefinedHeadPtr(ossValuePtr &ptr)
   {
      INT32 rc = SDB_OK;
      const storageFileHead *head = NULL;
      ossValuePtr headPtr = 0;
      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getFileHeadPtr(headPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      head = (const storageFileHead *)headPtr;
      if (0 == head->userDefinedHeadLen)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ptr = getSegmentInfo(1, NULL, NULL);
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

   INT32 extentStorageFile::create(const storageFileOptions &options,
                                   const void *userDefinedOptions)
   {
      INT32 rc = SDB_OK;

      if (isOpen())
      {
         PD_LOG(PDERROR, "file has already been open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = validateOptions(options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = createFileAndInitHead(options, userDefinedOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%d", rc);
         goto error;
      }

      rc = afterHeadOpen();
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      if (isOpen())
      {
         ossMmapFile::unlink();
      }
      goto done;
   }

   INT32 extentStorageFile::createFileAndInitHead(const storageFileOptions &options,
                                                  const void *userDefinedOptions)
   {
      INT32 rc = SDB_OK;
      CHAR headBuf[STORAGE_FILE_HEAD_SIZE] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE+1] = {0};
      ossValuePtr headPtr = 0;

      rc = utilBuildFullPath(options.dir, options.name, OSS_MAX_PATHSIZE,
                             fullPath) ;

      if (SDB_OK != rc)
      {
         PD_LOG ( PDERROR, "Path+filename are too long: %s; %s", options.dir,
                  options.name) ;
         goto error ;
      }

      rc = ossMmapFile::open(fullPath, OSS_CREATEONLY|OSS_READWRITE|OSS_EXCLUSIVE,
                             OSS_RU|OSS_WU|OSS_RG ) ;
      if (SDB_OK != rc)
      {
         PD_LOG ( PDERROR, "Failed to create new file %s, rc=%d", fullPath, rc) ;
         goto error ;
      }

      rc = initFileHead(options, headBuf, hasUserDefinedHead());
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = extendFileAndMMap(FALSE, STORAGE_FILE_HEAD_SIZE, &headPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extent file:%d", rc);
         goto error;
      }

      ossMemcpy((void *)headPtr, headBuf, STORAGE_FILE_HEAD_SIZE);
      rc = ossMmapFile::flush(0, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }

      _headInMem = *((const storageFileHead *)headBuf);

      if (!hasUserDefinedHead())
      {
         goto done;
      }

      rc = initUserDefinedHead(userDefinedOptions, headBuf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init user defined head:%d", rc);
         goto error;
      }

      rc = extendFileAndMMap(FALSE, STORAGE_FILE_HEAD_SIZE, &headPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extent file:%d", rc);
         goto error;
      }

      ossMemcpy((void *)headPtr, headBuf, STORAGE_FILE_HEAD_SIZE);
      rc = ossMmapFile::flush(1, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      _headInMem = storageFileHead();
      goto done;
   }

   BOOLEAN extentStorageFile::isOpen() const
   {
      return ossMmapFile::_file.isOpened();
   }

   INT32 extentStorageFile::destroy()
   {
      if (isOpen())
      {
         _headInMem = storageFileHead();
         _dataSegmentCount = 0;
         ossMmapFile::unlink();   
      }
      return SDB_OK;
   }

   INT32 extentStorageFile::close()
   {
      
      if (isOpen())
      {
         _headInMem = storageFileHead();
         _dataSegmentCount = 0;
         ossMmapFile::close();
      }
     
      return SDB_OK;
   }

   INT32 extentStorageFile::getExtentPtr(PAGE_ID page, ossValuePtr &ptr)
   {
      return getPagePtr(page, ptr);
   }

   INT32 extentStorageFile::getPagePtr(PAGE_ID page, ossValuePtr &ptr)
   {
      INT32 rc = SDB_OK;
      SEGMENT_ID segID = INVALID_SEG_ID;
      ossValuePtr segPtr = 0;
      if (OSS_UNLIKELY(INVALID_PAGE_ID == page))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      segID = page / _headInMem.maxPageCountPerSeg;
      rc = getSegmentPtr(segID, segPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      ptr = segPtr + ((page % _headInMem.maxPageCountPerSeg) * _headInMem.pageSize);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::getSegmentPtr(SEGMENT_ID seg, ossValuePtr &ptr)
   {
      INT32 rc = SDB_OK;
      UINT32 mmapSegID = 0;
      ossValuePtr segPtr = 0;

      if (OSS_UNLIKELY(INVALID_SEG_ID == seg))
      {
         PD_LOG(PDERROR, "invalid page id");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!isOpen()))
      {
         PD_LOG(PDERROR, "file has not been open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_dataSegmentCount <= seg)
      {
         rc = SDB_VESSEL_PAGE_NOT_EXISTS;
         goto error;
      }

      mmapSegID = getMMapSegmentID(seg);
      segPtr = ossMmapFile::getSegmentInfo(mmapSegID, NULL, NULL);
      if (0 == segPtr)
      {
         PD_LOG(PDERROR, "failed to get segment ptr, seg:%d, current seg size:%d",
                seg, ossMmapFile::segmentSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ptr = segPtr;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::fsync(PAGE_ID pid, UINT32 count, BOOLEAN sync)
   {
      INT32 rc = SDB_OK;
      UINT32 lastCount = count;
      UINT32 maxCountPerSeg = _headInMem.maxPageCountPerSeg;
      UINT32 pageSize = _headInMem.pageSize;
      PAGE_ID tmp = pid;
      UINT32 minSegCount = 0;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       0 == count ||
                       !isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      minSegCount = ((pid + count - 1) / maxCountPerSeg) + 1;

      if (OSS_UNLIKELY(_dataSegmentCount < minSegCount))
      {
         PD_LOG(PDERROR, "fsync out of file range");
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      do
      {
         SEGMENT_ID sid = getSegmentIDFromPageID(tmp);
         SDB_ASSERT(INVALID_SEG_ID != sid, "impossible");
         PAGE_ID beginPage = tmp % maxCountPerSeg;
         UINT32 pageCount =  (maxCountPerSeg - beginPage) < lastCount ? (maxCountPerSeg - beginPage) : lastCount;
         rc =  ossMmapFile::flushBlock(getMMapSegmentID(sid),
                                       beginPage * pageSize,
                                       pageCount * pageSize, sync);
         if (SDB_OK != rc)
         {
            goto error;
         }

         lastCount -= pageCount;
         tmp += pageCount;
      } while (0 < lastCount);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::validateOptions(const storageFileOptions &options)
   {
      INT32 rc = SDB_OK;

      UINT32 nameLen = 0;

      if (OSS_UNLIKELY(NULL == options.dir ||
                       NULL == options.name))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      nameLen = ossStrlen(options.name);

      if (0 == nameLen ||
          SU_FILE_NAME_LEN < nameLen)
      {
         PD_LOG(PDERROR, "invalid length of file name:%s");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (options.spaceID == INVALID_SPACE_ID)
      {
         PD_LOG(PDERROR, "invalid space id");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (NULL != options.args)
      {
         if (options.args->pageSize != DMS_PAGE_SIZE16K &&
          options.args->pageSize != DMS_PAGE_SIZE32K &&
          options.args->pageSize != DMS_PAGE_SIZE64K)
         {
            PD_LOG(PDERROR, "invalid page size:%d", options.args->pageSize);
            rc = SDB_INVALIDARG;
            goto error;
         }
         if (0 == options.args->maxPageCountPerSeg)
         {
            PD_LOG(PDERROR, "invalid maxPageCountPerSeg: %d", options.args->maxPageCountPerSeg);
            rc = SDB_INVALIDARG;
            goto error;
         }
         if (0 != options.args->maxPageCountPerSeg % sizeof(UINT32))
         {
            PD_LOG(PDERROR, "invalid maxPageCountPerSeg:%d", options.args->maxPageCountPerSeg);
            rc = SDB_INVALIDARG;
            goto error;
         }
         if (0 == options.args->maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "invalid maxSegmentCount: %d", options.args->maxSegmentCountPerFile);
            rc = SDB_INVALIDARG;
            goto error;
         }
      }
         
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::initFileHead(const storageFileOptions &options,
                                         CHAR *headBuf,
                                         BOOLEAN hasUserDefinedHead)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != headBuf, "can not be null");
      UINT32 checksum = 0;
      ossMemset(headBuf, 0, STORAGE_FILE_HEAD_SIZE);
      storageFileHead *head = (storageFileHead *)headBuf;
      ossMemcpy(head->magicChars, getMagicChars(), sizeof(head->magicChars));
      head->version = STORAGE_FILE_CURRENT_VERSION;
      ossStrcpy(head->name, options.name);
      head->headChecksum = 0;
      head->createTime = ossGetCurrentMilliseconds();
      head->secretValue = options.secretValue;
      head->spaceID = options.spaceID;
      head->spaceType = getSpaceType();
      head->sequence = options.sequence;
      if (NULL == options.args)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      head->pageSize = options.args->pageSize;
      head->maxSegmentCountPerFile = options.args->maxSegmentCountPerFile;
      head->maxPageCountPerSeg = options.args->maxPageCountPerSeg;
      head->userDefinedHeadLen = hasUserDefinedHead ? STORAGE_FILE_HEAD_SIZE : 0;

      rc = createChecksum(headBuf, STORAGE_FILE_HEAD_SIZE, checksum);
      if (SDB_OK != rc)
      {
         goto error;
      }

      head->headChecksum = checksum;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::extendFileAndMMap(BOOLEAN sparse, UINT32 len, ossValuePtr *ptr)
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

      if (sparse)
      {
         rc = ossExtentBySparse(&_file, len);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extent file with sparse: %s, %d, %d", _fileName, len, rc);
            goto error;
         }
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

   INT32 extentStorageFile::allocateNewSegment(BOOLEAN sparse)
   {
      INT32 rc = SDB_OK;
      UINT32 extendLen = 0;
      ossValuePtr ptr = 0;

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
      rc = extendFileAndMMap(sparse, extendLen, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file:%d", rc);
         goto error;
      }

      ++_dataSegmentCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::ensureSegmentCount(UINT32 count, BOOLEAN sparse)
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
         rc = allocateNewSegment(sparse);
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

   INT32 extentStorageFile::validateHead(const void *head, const storageFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != head, "can not be null");
      const storageFileHead *suHead = (const storageFileHead *)head;
      UINT32 checksum = 0;
      CHAR tmpHeadBuf[STORAGE_FILE_HEAD_SIZE] = {0};
      ossMemcpy(tmpHeadBuf, head, STORAGE_FILE_HEAD_SIZE);
      ((storageFileHead *)tmpHeadBuf)->headChecksum = 0;
      rc = createChecksum(tmpHeadBuf, STORAGE_FILE_HEAD_SIZE, checksum);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checksum of head:%d", rc);
         goto error;
      }

      if (suHead->headChecksum != checksum)
      {
         PD_LOG(PDERROR, "invalid checksum, in file:%d, current:%d", suHead->headChecksum, checksum);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (0 != ossMemcmp(getMagicChars(), suHead->magicChars, sizeof(suHead->magicChars)))
      {
         PD_LOG(PDERROR, "invaid magic chars of head");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (STORAGE_FILE_CURRENT_VERSION != suHead->version)
      {
         PD_LOG(PDERROR, "invalid su version:%d", suHead->version);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (0 != ossStrcmp(fn.getName(), suHead->name))
      {
         PD_LOG(PDERROR, "invalid name in head");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (fn.getSpaceID() != suHead->spaceID)
      {
         PD_LOG(PDERROR, "invalid space id");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (suHead->spaceType != fn.getType())
      {
         PD_LOG(PDERROR, "invalid space type:%d, %d", suHead->spaceType, fn.getType());
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (getSpaceType() != suHead->spaceType)
      {
         PD_LOG(PDERROR, "invalid space type:%d, %d", suHead->spaceType, getSpaceType());
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (suHead->sequence != fn.getSequence())
      {
         PD_LOG(PDERROR, "invalid file sequence:%d, %d", suHead->sequence, fn.getSequence());
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (suHead->pageSize != DMS_PAGE_SIZE4K &&
          suHead->pageSize != DMS_PAGE_SIZE8K &&
          suHead->pageSize != DMS_PAGE_SIZE16K &&
          suHead->pageSize != DMS_PAGE_SIZE32K &&
          suHead->pageSize != DMS_PAGE_SIZE64K)
      {
         PD_LOG(PDERROR, "invalid page size:%d", suHead->pageSize);
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      if (0 == suHead->maxSegmentCountPerFile)
      {
         PD_LOG(PDERROR, "invalid maxSegmentCount");
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageFile::createChecksum(const CHAR *headBuf, UINT32 len, UINT32 &checksum)
   {
      INT32 rc = SDB_OK;
      rc = utilCRC32(headBuf, len, checksum);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

} // namespace vessel
} // namespace engine