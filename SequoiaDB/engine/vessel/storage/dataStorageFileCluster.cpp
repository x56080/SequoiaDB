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
#include "vessel/storageUtils.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageUnit.h"

namespace engine
{
namespace vessel
{
   dataStorageFileCluster::dataStorageFileCluster()
   {}

   dataStorageFileCluster::~dataStorageFileCluster()
   {
      _close();
   }

   void dataStorageFileCluster::_close()
   {
      for (UINT32 i = 0; i < _files.getSize(); ++i)
      {
         storageFile *file = NULL;
         _files.get<storageFile>(i, file);
         if (NULL != file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }

      _files.fini();
      return;
   }

   INT32 dataStorageFileCluster::fsyncSegment(UINT32 globalSegmentId)const
   {
      INT32 rc = SDB_OK;
      storageFile *file = NULL;
      UINT32 fileId = 0;
      UINT32 segmentInFile = 0;
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();

      if (OSS_UNLIKELY(!dataPageCluster::isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      fileId = globalSegmentId / args.maxSegmentCountPerFile;
      if (_files.getSize() <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = _files.get<storageFile>(fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "file[%d] does not exist", fileId);
         rc = SDB_FNE;
         goto error;
      }

      segmentInFile = globalSegmentId % args.maxSegmentCountPerFile;
      rc = file->fsyncSegment(segmentInFile, TRUE);
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

   INT32 dataStorageFileCluster::fysncPage(PAGE_ID pid)const
   {
      INT32 rc = SDB_OK;
      UINT32 fileId = 0;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      storageFile *file = NULL;
      
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!dataPageCluster::isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      fileId = getFileIdByGlobalPageId(pid, &pidInFile);
      if (_files.getSize() <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = _files.get<storageFile>(fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "file[%d] does not exist", fileId);
         rc = SDB_FNE;
         goto error;
      }

      rc = file->fsyncPage(pidInFile, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync page[%d], rc:%d", pidInFile, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::getDataPagePtr(PAGE_ID pid, mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      UINT32 fileId = 0;
      storageFile *file = NULL;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      ossValuePtr p = 0;

      if (OSS_UNLIKELY(!dataPageCluster::isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fileId = getFileIdByGlobalPageId(pid, &pidInFile);
      if (_files.getSize() <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = _files.get<storageFile>(fileId);
      if (NULL == file)
      {
         PD_LOG(PDERROR, "file[%d] does not exist", fileId);
         rc = SDB_FNE;
         goto error;
      }

      rc = file->getPagePtr(pidInFile, p);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d,%d] ptr:%d",
                fileId, pidInFile, rc);
         goto error;
      }

      ptr.reset(p);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::openFiles(requestContext *context,
                                           const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be invalid");
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      constexpr UINT64 MAX_FILE_SEQUENCE = 1048575;
      
      storageFile *file = NULL;
      const FILE_NAME_LIST *fileList = NULL;
      constexpr UINT32 DEFAULT_CAPACITY = 16;

      storageUnit *su = context->getEnv()->dms.getStorageUnit(getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");

      if (!args.isValid())
      {
         PD_LOG(PDERROR, "invalid core args");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _files.init(DEFAULT_CAPACITY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init file array:%d", rc);
         goto error;
      }

      if (NULL == loader)
      {
         goto done;
      }

      fileList = loader->getFileList(getSpaceType(), FILE_TYPE_DATA_STORAGE);
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
         else if (fn.getSpaceID() != getSpaceID())
         {
            PD_LOG(PDERROR, "space id does not match creater:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (fn.getSpaceType() != getSpaceType())
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

         rc = su->openStorageFile(fn, file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fn.getFileName(), rc);
            goto error;
         }

         if (file->getCommonHeadInMem().secretValue != getSecretValue())
         {
            PD_LOG(PDERROR, "secret values do not match[%d,%d]",
                   file->getCommonHeadInMem().secretValue, getSecretValue());
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
         rc = _files.set<storageFile>(sequence, file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add file[%lld] to array:%d", sequence, rc);
            goto error;
         }

         file = NULL;
      }

      if (0 < _files.getSize())
      {
         storageFile *lastFile = NULL;
         _files.get<storageFile>(_files.getSize() - 1, lastFile);
         
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
      for (UINT32 i = 0; i < _files.getSize(); ++i)
      {
         storageFile *file = _files.get<storageFile>(i);
         if (NULL != file)
         {
            PD_LOG(PDINFO, "will destroy file:%s", file->getFullPath());
            file->destroy();
            SDB_OSS_DEL file;
         }
      }
      _close();
      return;
   }

   INT32 dataStorageFileCluster::isSparseSegment(UINT32 globalSegmentId,
                                                 BOOLEAN &isSparse)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dataPageCluster::isOpen(), "must be open");
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      storageFile *file = NULL;
      UINT32 minSegmentCount = 0;
      
      UINT32 fileId = globalSegmentId / args.maxSegmentCountPerFile;
      if (_files.getSize() <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      
      file = _files.get<storageFile>(fileId);
      if (NULL == file)
      {
         isSparse = TRUE;
         goto done;
      }

      minSegmentCount = globalSegmentId % args.maxSegmentCountPerFile + 1;
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

   UINT32 dataStorageFileCluster::getTotalSegmentCountAllocated()const
   {
      SDB_ASSERT(dataPageCluster::isOpen(), "must be open");
      SDB_ASSERT(dataPageCluster::getCoreArgs().isValid(), "must be valid");
      UINT32 count = 0;
      UINT32 size = _files.getSize();
      if (0 < size)
      {
         storageFile *file = _files.get<storageFile>(size - 1);
         SDB_ASSERT(NULL != file, "the last file can not be null");
         if (1 < size)
         {
            count = (size - 1) * dataPageCluster::getCoreArgs().maxSegmentCountPerFile;
         }
         count += file->getSegmentCount();
      }
      return count;
   }

   INT32 dataStorageFileCluster::allocateNewSegment(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dataPageCluster::isOpen(), "must be open");
      SDB_ASSERT(dataPageCluster::getCoreArgs().isValid(), "must be valid");

      storageFile *file = NULL;
      if (0 == _files.getSize())
      {
         rc = createNewFile(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new storage file:%d", rc);
            goto error;
         }
         goto done;
      }

      file = _files.get<storageFile>(_files.getSize() - 1);
      SDB_ASSERT(NULL != file, "the last file can not be null");
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
         rc = createNewFile(context);
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

   INT32 dataStorageFileCluster::ensureSegmentNotSparse(requestContext *context,
                                                        UINT32 globalSegmentId)
   {
      INT32 rc = SDB_OK;
      storageFile *file = NULL;
      UINT32 segmentIdInFile = 0;
      UINT32 fileId = getFileIdByGlobalSegmentId(globalSegmentId, &segmentIdInFile);
      if (_files.getSize() <= fileId)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = _files.get<storageFile>(fileId);
      if (NULL == file)
      {
         rc = createFileEverShrinked(context, fileId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create file ever shrinked:%d", rc);
            goto error;
         }
      }

      rc = file->ensureSegmentCount(segmentIdInFile + 1);
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

   INT32 dataStorageFileCluster::createNewFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");

      storageUnit *su = context->getEnv()->dms.getStorageUnit(getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");

      createStorageFileOptions o;
      vesselFileName fn;

      storageFile *file = SDB_OSS_NEW storageFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(getSpaceID(), FILE_TYPE_DATA_STORAGE,
                    getSpaceType(), _files.getSize()))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = getSecretValue();

      rc = su->createStorageFile(fn, o, slice(), file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new file[%d], rc:%d", fn.getFileName(), rc);
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

      rc = file->removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file's shadow suffix:%d", rc);
         goto error;
      }

      rc = _files.pushBack<storageFile>(file);
      if (SDB_OK != rc)
      {
         goto error;
      }
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

   INT32 dataStorageFileCluster::createFileEverShrinked(requestContext *context,
                                                        UINT32 sequence)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();
      SDB_ASSERT(args.isValid(), "can not be invalid");

      storageUnit *su = context->getEnv()->dms.getStorageUnit(getSpaceID());
      SDB_ASSERT(NULL != su, "can not be null");
      storageFile *file = NULL ;
      createStorageFileOptions o;
      vesselFileName fn;

      if (_files.getSize() <= sequence)
      {
         PD_LOG(PDERROR, "invalid sequence[%d] to create, current size[%d]",
                sequence, _files.getSize());
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL != _files.get<storageFile>(sequence))
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

      if (!fn.build(getSpaceID(), FILE_TYPE_DATA_STORAGE,
                    getSpaceType(), _files.getSize()))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = getSecretValue();

      rc = su->createStorageFile(fn, o, slice(), file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new file[%d], rc:%d", fn.getFileName(), rc);
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
         PD_LOG(PDERROR, "failed to fsync file head:%d", rc);
         goto error;
      }

      rc = file->removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file's shadow suffix:%d", rc);
         goto error;
      }

      rc = _files.set(sequence, file);
      if (SDB_OK != rc)
      {
         goto error;
      }
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

   UINT32 dataStorageFileCluster::getFileIdByGlobalSegmentId(UINT32 globalSegment,
                                                             UINT32 *segmentInFile)const
   {
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      UINT32 fileId = globalSegment / args.maxSegmentCountPerFile;
      if (NULL != segmentInFile)
      {
         *segmentInFile = globalSegment % args.maxSegmentCountPerFile;
      }
      return fileId;
   }
         
   UINT32 dataStorageFileCluster::getFileIdByGlobalPageId(PAGE_ID pid,
                                                          PAGE_ID *pidInFile)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      UINT32 pageCountPerFile = args.getMaxPageCountInFile();
      UINT32 fileId = pid / pageCountPerFile;
      if (NULL != pidInFile)
      {
         *pidInFile = (pid & (pageCountPerFile - 1));
      }
      return fileId;
   }
}//namespace vessel
}//namespace engine