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

   Source File Name = dataStorageFileCluster.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dataStorageFileCluster.h"
#include "vessel/storageFile.h"
#include "pdTrace.hpp"
#include "vessel/storageFileLoader.h"
#include "vessel/storageFileCreater.h"
#include "vessel/storageUtils.h"

namespace engine
{
namespace vessel
{
   OSS_INLINE storageFile *getFileFromArray(ossValuePtr *array, UINT32 i)
   {
      return (storageFile *)(array[i]);
   }

   dataStorageFileCluster::dataStorageFileCluster()
   {}

   dataStorageFileCluster::~dataStorageFileCluster()
   {
      _close();
   }

   void dataStorageFileCluster::_close()
   {
      for (UINT32 i = 0; i < _size; ++i)
      {
         storageFile *file = getFileFromArray(_array, i);
         if (NULL != file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }

      if (NULL != _old)
      {
         SDB_THREAD_FREE(_old);
         _old = NULL;
      }
      if (NULL != _array)
      {
         SDB_THREAD_FREE(_array);
         _array = NULL;
      }
      _capacity = 0;
      _size = 0;
      return;
   }

   UINT32 dataStorageFileCluster::getTotalSegmentCountAllocated()const
   {
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      UINT32 count = 0;
      storageFile *file = NULL;
      if (0 == _size)
      {
         goto done;
      }

      file = getFileFromArray(_array, _size - 1);
      SDB_ASSERT(NULL != file, "last file can not be null");
      SDB_ASSERT(0 < file->getSegmentCount(), "can not be zero");
      count = ((_size - 1) * args.maxSegmentCountPerFile) + file->getSegmentCount();
   done:
      return count;
   }

   INT32 dataStorageFileCluster::fsyncSegment(UINT32 globalSegmentId)const
   {
      INT32 rc = SDB_OK;
      storageFile *file = NULL;
      UINT32 fileId = 0;
      UINT32 segmentInFile = 0;
      const storageCoreArgs &args = getCoreArgs();

      if (OSS_UNLIKELY(!dataPageCluster::isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      fileId = globalSegmentId / args.maxSegmentCountPerFile;
      if (_size <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = getFileFromArray(_array, fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "file[%d] does not exist", fileId);
         rc = SDB_FNE;
         goto error;
      }

      segmentInFile = (globalSegmentId & (args.maxSegmentCountPerFile - 1));
      rc = file->fsync(segmentInFile, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync segment[%d,%d], rc:%d",
                fileId, segmentInFile, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::getPagePtr(PAGE_ID pid, ossValuePtr &ptr)const
   {
      INT32 rc = SDB_OK;
      UINT32 fileId = 0;
      storageFile *file = NULL;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      UINT32 maxPageCountInFile = args.getMaxPageCountInFile();

      if (OSS_UNLIKELY(!dataPageCluster::isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      fileId = pid / maxPageCountInFile;
      if (_size <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = getFileFromArray(_array, fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "file[%d] does not exist", fileId);
         rc = SDB_VESSEL_PAGE_NOT_EXISTS;
         goto error;
      }

      pidInFile = (pid & (maxPageCountInFile - 1));
      rc = file->getPagePtr(pidInFile, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d,%d] ptr:%d",
                fileId, pidInFile, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::openFiles(const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      SDB_ASSERT(0 == _capacity, "must be empty");
      const storageFileCreater *creater = dataPageCluster::getCreater();
      SDB_ASSERT(NULL != creater, "can not be null");
      SDB_ASSERT(creater->isValid(), "must be valid");
      constexpr UINT64 MAX_FILE_SEQUENCE = 1048575;

      storageFile *file = NULL;
      const FILE_NAME_LIST *fileList = NULL;
      constexpr UINT32 DEFAULT_CAPACITY = 32;
      UINT32 bufferSize = sizeof(ossValuePtr) * DEFAULT_CAPACITY;
      _array = (ossValuePtr *)SDB_THREAD_ALLOC(bufferSize);
      if (NULL == _array)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemset(_array, 0, bufferSize);
      _capacity = DEFAULT_CAPACITY;

      if (NULL == loader)
      {
         goto done;
      }

      fileList = loader->getFileList(FILE_TYPE_DATA_STORAGE);
      if (NULL == fileList)
      {
         goto done;
      }

      for (FILE_NAME_LIST::const_iterator itr = fileList->begin();
           itr != fileList->end(); ++itr)
      {
         UINT32 sequence = 0;
         const vesselFileName &fn = *itr;
         if (!fn.isValid())
         {
            PD_LOG(PDERROR, "found invalid file name in list");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (FILE_TYPE_DATA_STORAGE != fn.getFileType())
         {
            PD_LOG(PDERROR, "invalid file type:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (fn.getSpaceID() != creater->getSpaceID())
         {
            PD_LOG(PDERROR, "space id does not match creater:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (fn.getSpaceType() != creater->getSpaceType())
         {
            PD_LOG(PDERROR, "space type does not match creater:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (fn.hasShadowSuffix())
         {
            PD_LOG(PDERROR, "found file with shadow suffix:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (OSS_UNLIKELY(MAX_FILE_SEQUENCE <= fn.getSequence()))
         {
            PD_LOG(PDERROR, "found file with oversize sequence:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         file = SDB_OSS_NEW storageFile();
         if (OSS_UNLIKELY(NULL == file))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = file->open(creater->getDirSlice(), fn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fn.getFileName(), rc);
            goto error;
         }

         if (file->getCommonHeadInMem().secretValue != creater->getSecretValue())
         {
            PD_LOG(PDERROR, "secret values do not match[%d,%d]",
                   file->getCommonHeadInMem().secretValue, creater->getSecretValue());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().pageSize != args.pageSize)
         {
            PD_LOG(PDERROR, "page size do not match[%d,%d]",
                   file->getCommonHeadInMem().pageSize, args.pageSize);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().maxPageCountPerSeg != args.maxPageCountPerSeg)
         {
            PD_LOG(PDERROR, "page count of segment do not match[%d,%d]",
                   file->getCommonHeadInMem().maxPageCountPerSeg, args.maxPageCountPerSeg);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().maxSegmentCountPerFile != args.maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "segment count do not match[%d,%d]",
                   file->getCommonHeadInMem().maxSegmentCountPerFile, args.maxSegmentCountPerFile);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }

         sequence = file->getCommonHeadInMem().sequence;

         rc = ensureArrayCapacity(sequence + 1);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure array capacity[%d], rc:%d",
                   sequence + 1, rc);
            goto error;
         }

         _array[sequence] = (ossValuePtr)file;
         if (_size <= sequence)
         {
            _size = sequence + 1;
         }

         file = NULL;
      }

      if (0 < _size)
      {
         storageFile *lastFile = getFileFromArray(_array, _size - 1);
         if (0 == lastFile->getSegmentCount())
         {
            PD_LOG(PDERROR, "last file[%s] should not be empty", file->getFullPath());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
      }
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      _close();
      goto done;
   }

   void dataStorageFileCluster::closeFiles()
   {
      _close();
   }

   void dataStorageFileCluster::destroyFiles()
   {
      for (UINT32 i = 0; i < _size; ++i)
      {
         storageFile *file = getFileFromArray(_array, i);
         if (NULL != file)
         {
            PD_LOG(PDINFO, "will destroy file:%s", file->getFullPath());
            file->destroy();
            SDB_OSS_DEL file;
            _array[i] = 0;
         }
      }
      _size = 0;
      _close();
      return;
   }

   INT32 dataStorageFileCluster::isSparseSegment(UINT32 globalSegmentId,
                                                 BOOLEAN &isSparse)const
   {
      INT32 rc = SDB_OK;
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      storageFile *file = NULL;
      UINT32 minSegmentCount = 0;
      
      UINT32 fileId = globalSegmentId / args.maxSegmentCountPerFile;
      if (_size <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      
      file = getFileFromArray(_array, fileId);
      if (NULL == file)
      {
         isSparse = TRUE;
         goto done;
      }

      minSegmentCount = (globalSegmentId & (args.maxSegmentCountPerFile - 1)) + 1;
      if (file->getSegmentCount() < minSegmentCount)
      {
         isSparse = TRUE;
         goto done;
      }

      isSparse = FALSE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::allocateNewSegment()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dataPageCluster::isOpen(), "must be open");
      SDB_ASSERT(NULL != _array, "impossible");

      storageFile *file = NULL;
      if (0 == _size)
      {
         rc = createNewFile();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new storage file:%d", rc);
            goto error;
         }
      }

      file = getFileFromArray(_array, _size - 1);
      SDB_ASSERT(NULL != file, "impossible");
      if (file->getSegmentCount() < file->getCommonHeadInMem().maxSegmentCountPerFile)
      {
         rc = file->allocateNewSegment();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend storage file[%s]:%d",
                   file->getFullPath(), rc);
            goto error;
         }
      }
      else
      {
         rc = createNewFile();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new storage file:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::ensureSegmentNotSparse(UINT32 globalSegmentId)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _array, "can not be null");
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();
      SDB_ASSERT(args.isValid(), "can not be invalid");
      storageFile *file = NULL;
      UINT32 minSegmentCount = 0;
      UINT32 fileId = globalSegmentId / args.getMaxPageCountInFile();
      if (_size <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = getFileFromArray(_capacity, fileId);
      if (NULL == file)
      {
         rc = createFileEverShrinked(fileId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create file ever shrinked:%d", rc);
            goto error;
         }
      }

      minSegmentCount = (globalSegmentId & (args.maxSegmentCountPerFile - 1)) + 1;
      rc = file->ensureSegmentCount(minSegmentCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure segment count:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::createNewFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _array, "impossible");

      const storageFileCreater *creater = dataPageCluster::getCreater();
      SDB_ASSERT(NULL != creater, "can not be null");
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();

      storageFile *file = NULL ;
      
      rc = ensureArrayCapacity(_size + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure array size[%d], rc:%d",
                _size + 1, rc);
         goto error;
      }

      file = SDB_OSS_NEW storageFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = creater->createTmpFile(FILE_TYPE_DATA_STORAGE,
                                  _size, args, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create tmp file:%d", rc);
         goto error;
      }

      rc = file->allocateNewSegment();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new segment:%d", rc);
         goto error;
      }

      rc = file->fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file:%d", rc);
         goto error;
      }

      rc = renameToFormalAndReopen(creater->getDirSlice(), FALSE,
                                   FILE_SHADOW_SUFFIX_TMP, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename to formal file:%d", rc);
         goto error;
      }

      _array[_size++] = (ossValuePtr)file;
      file = NULL;
   
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->destroy();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 dataStorageFileCluster::createFileEverShrinked(UINT32 sequence)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _array, "impossible");

      const storageFileCreater *creater = dataPageCluster::getCreater();
      SDB_ASSERT(NULL != creater, "can not be null");
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();

      storageFile *file = NULL ;

      if (_size <= sequence)
      {
         PD_LOG(PDERROR, "invalid sequence[%d] to create, current size[%d]",
                sequence, _size);
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL != getFileFromArray(_array, sequence))
      {
         rc = SDB_FE;
         goto error;
      }

      file = SDB_OSS_NEW storageFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = creater->createTmpFile(FILE_TYPE_DATA_STORAGE,
                                  sequence, args, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create tmp file:%d", rc);
         goto error;
      }

      rc = file->allocateNewSegment();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new segment:%d", rc);
         goto error;
      }

      rc = file->fsyncFileHead();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file head:%d", rc);
         goto error;
      }

      rc = renameToFormalAndReopen(creater->getDirSlice(), FALSE,
                                   FILE_SHADOW_SUFFIX_TMP, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to rename to formal file:%d", rc);
         goto error;
      }

      _array[sequence] = (ossValuePtr)file;
      file = NULL;
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->destroy();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 dataStorageFileCluster::ensureArrayCapacity(UINT32 size)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 != _capacity, "can not be zero");
      SDB_ASSERT(NULL != _array, "can not be null");
      UINT32 capacity = _capacity;
      ossValuePtr *buffer = NULL;
      UINT32 bufferSize = 0;

      if (size <= capacity)
      {
         goto done;
      }

      do
      {
         capacity = capacity << 1;
      } while (capacity < size);
      

      bufferSize = (capacity << 3); /// capacity * sizeof(ossValuePtr)
      buffer = (ossValuePtr *)SDB_THREAD_ALLOC(bufferSize);
      if (NULL == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ossMemset(buffer, 0, bufferSize);
      ossMemcpy(buffer, _array, (_size << 3));
      if (NULL != _old)
      {
         SDB_THREAD_FREE(_old);
      }
      _old = _array;
      _array = buffer;
      _capacity = capacity;
      buffer = NULL;
   done:
      return rc;
   error:
      if (NULL != buffer)
      {
         SDB_THREAD_FREE(buffer);
      }
      goto done;
   }



}//namespace vessel
}//namespace engine