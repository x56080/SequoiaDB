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

   Source File Name = storageFileCluster.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/storageFileCluster.h"
#include "vessel/storageFile.h"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileMaintainer.h"

namespace engine
{
namespace vessel
{
   storageFileCluster::storageFileCluster()
   {

   }

   storageFileCluster::~storageFileCluster()
   {

   }

   INT32 storageFileCluster::open(const storageFileManifest &manifest,
                                  UINT32 ctlFlags,
                                  const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      static constexpr UINT32 _DEFAULT_FILE_ARRAY_CAPACITY = 8;
      close();

      if (!manifest.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _ctl = ctlFlags;
      _manifest = manifest;
      rc = _files.init(_DEFAULT_FILE_ARRAY_CAPACITY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init file array:%d", rc);
         goto error;
      }

      if (nullptr != loader)
      {
         rc = loadFiles(loader);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to load storage files:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   void storageFileCluster::close()
   {
      _ctl = 0;
      _manifest.reset();
      for (UINT32 i = 0; i < _files.getSize(); ++i)
      {
         storageFile *file = NULL;
         _files.get<storageFile>(i, file);
         if (NULL != file)
         {
            file->close();
            releaseFilePtr(file);
         }
      }

      _files.fini();
   }

   void storageFileCluster::destroy()
   {
      _ctl = 0;
      _manifest.reset();
      for (UINT32 i = 0; i < _files.getSize(); ++i)
      {
         storageFile *file = NULL;
         _files.get<storageFile>(i, file);
         if (NULL != file)
         {
            file->destroy();
            releaseFilePtr(file);
         }
      }

      _files.fini();
   }

   INT32 storageFileCluster::loadFiles(const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != loader, "can not be null");
      SDB_ASSERT(_manifest.isValid(), "can not be invalid");

      constexpr UINT32 MAX_FILE_SEQUENCE = 1048575;

      const storageCoreArgs &args = _manifest.args;
      storageFile *file = nullptr;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest.sid);
      const STORAGE_FILE_NAME_LIST *fileList = loader->getFileList(_manifest.stype,
                                                                   _manifest.ftype);
      if (nullptr == fileList)
      {
         goto done;
      }

      for (STORAGE_FILE_NAME_LIST::const_iterator itr = fileList->begin();
           itr != fileList->end(); ++itr)
      {
         UINT32 sequence = 0;
         const storageFileName &fn = *itr;
         if (!fn.isValid())
         {
            PD_LOG(PDERROR, "found invalid file name in list");
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (fn.getFileType() != _manifest.ftype)
         {
            PD_LOG(PDERROR, "invalid file type:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }
         else if (fn.getSpaceType() != _manifest.stype)
         {
            PD_LOG(PDERROR, "invalid space type:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }
         else if (fn.hasShadowSuffix())
         {
            PD_LOG(PDERROR, "found file with shadow suffix:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }
         else if (OSS_UNLIKELY(MAX_FILE_SEQUENCE <= fn.getSequence()))
         {
            PD_LOG(PDERROR, "found file with oversize sequence:%s", fn.getFileName());
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }

         file = createFilePtr();
         if (OSS_UNLIKELY(NULL == file))
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = sfm.openStorageFile(fn, _ctl, *file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fn.getFileName(), rc);
            goto error;
         }

         if (file->getCommonHeadInMem().secretValue != _manifest.secretValue)
         {
            PD_LOG(PDERROR, "secret values do not match[%d,%d]",
                   file->getCommonHeadInMem().secretValue, _manifest.secretValue);
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().pageSize != args.pageSize)
         {
            PD_LOG(PDERROR, "page size do not match[%d,%d]",
                   file->getCommonHeadInMem().pageSize, args.pageSize);
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().maxPageCountPerSeg != args.maxPageCountPerSeg)
         {
            PD_LOG(PDERROR, "page count of segment do not match[%d,%d]",
                   file->getCommonHeadInMem().maxPageCountPerSeg, args.maxPageCountPerSeg);
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }

         if (file->getCommonHeadInMem().maxSegmentCountPerFile != args.maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "segment count do not match[%d,%d]",
                   file->getCommonHeadInMem().maxSegmentCountPerFile, args.maxSegmentCountPerFile);
            rc = SDB_VESSEL_INVALID_FILE;
            goto error;
         }

         sequence = fn.getSequence();
         rc = _files.set<storageFile>(sequence, file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add file[%d] to array:%d", sequence, rc);
            goto error;
         }

         file = nullptr;
      }

      for (UINT32 i = 0; i < _files.getSize(); ++i)
      {
         storageFile *file = _files.get<storageFile>(i);
         if (nullptr == file)
         {
            PD_LOG(PDERROR, "file [%d] not found when load[%d, %d]",
                   i, _manifest.sid, _manifest.stype);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      return rc;
   error:
      if (nullptr != file)
      {
         file->close();
         releaseFilePtr(file);
      }
      goto done;
   }

   UINT32 storageFileCluster::getTotalSegmentCount()const
   {
      UINT32 count = 0;
      if (0 < _files.getSize())
      {
         count = (_files.getSize() - 1) * _manifest.args.maxSegmentCountPerFile;
         count += _files.getBack<storageFile>()->getSegmentCount();
      }
      
      return count;
   }

   UINT32 storageFileCluster::getFileCount()const
   {
      return _files.getSize();
   }

   BOOLEAN storageFileCluster::isOutOfSpace(PAGE_ID pid)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(isOpen(), "can not be invalid");
      UINT32 totalPageCount = getCoreArgs().maxPageCountPerSeg * getTotalSegmentCount();
      return totalPageCount <= pid;
   }

   INT32 storageFileCluster::getFileSpaceId(PAGE_ID pid)const
   {
      if (OSS_LIKELY(INVALID_PAGE_ID != pid && isOpen()))
      {
         return pid / getCoreArgs().getMaxPageCountInFile();
      }
      else
      {
         SDB_ASSERT(INVALID_PAGE_ID != pid, "invalid pid");
         SDB_ASSERT(isOpen(), "invalid file cluster");
         return -1;
      }
   }

   INT32 storageFileCluster::read(UINT64 offset, UINT64 size, void *buf)
   {
      INT32 rc = SDB_OK;
      UINT64 maxSize = 0;
      UINT64 totalReadSize = 0;
      UINT64 maxFileSize = 0;

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
      
      maxSize = getCoreArgs().getSegmentSize() * getTotalSegmentCount();
      if (maxSize < (offset + size))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      maxFileSize = getCoreArgs().getMaxFileBodySize();
      do
      {
         UINT64 readOffset = offset + totalReadSize;
         UINT32 fileId = readOffset / maxFileSize;
         UINT64 fileOffset = readOffset % maxFileSize;
         UINT64 readSize = maxFileSize - fileOffset;
         if ((size - totalReadSize) < readSize)
         {
            readSize = size - totalReadSize;
         }

         storageFile *file = _files.get<storageFile>(fileId);
         if (OSS_UNLIKELY(nullptr == file))
         {
            PD_LOG(PDERROR, "failed to get file ptr[%d]", fileId);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = file->readData(fileOffset, readSize, (CHAR *)buf + totalReadSize);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read [%lld, %lld] from file4[%d], rc:%d",
                   fileOffset, readSize, fileId, rc);
            goto error;
         }

         totalReadSize += readSize;
      } while (totalReadSize < size);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileCluster::readPages(PAGE_ID pid,
                                       UINT32 pcnt,
                                       CHAR *data)
   {
      INT32 rc = SDB_OK;
      UINT32 totalSegments = 0;
      UINT32 maxSegment = 0;
      UINT32 pageCountPerFile = 0;
      UINT32 rcnt = 0;

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

      totalSegments = getTotalSegmentCount();
      maxSegment = (pid + pcnt - 1) / getCoreArgs().maxPageCountPerSeg;
      if (totalSegments <= maxSegment)
      {
         SDB_ASSERT(FALSE, "out of bound");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      pageCountPerFile = getCoreArgs().getMaxPageCountInFile();

      do
      {
         PAGE_ID p = pid + rcnt;
         UINT32 count = 1;
         UINT32 fileId = p / pageCountPerFile;
         for (UINT32 i = rcnt + 1; i < pcnt; ++i)
         {
            UINT32 nextFileId = (p + count) / pageCountPerFile;
            if (nextFileId == fileId)
            {
               ++count;
            }
            else
            {
               break;
            }
         }

         storageFile *file = _files.get<storageFile>(fileId);
         if (OSS_UNLIKELY(nullptr == file))
         {
            PD_LOG(PDERROR, "failed to get file[%d]", fileId);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         rc = file->readPages(p % pageCountPerFile, count,
                              data + (rcnt * getCoreArgs().pageSize));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file pages:%d", rc);
            goto error;
         }

         rcnt += count;

      } while(rcnt < pcnt);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileCluster::writePages(PAGE_ID pid,
                                        UINT32 pcnt,
                                        const CHAR *data)
   {
      INT32 rc = SDB_OK;
      UINT32 totalSegments = 0;
      UINT32 maxSegment = 0;
      UINT32 pageCountPerFile = 0;
      UINT32 wcnt = 0;

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

      totalSegments = getTotalSegmentCount();
      maxSegment = (pid + pcnt - 1) / getCoreArgs().maxPageCountPerSeg;
      if (totalSegments <= maxSegment)
      {
         SDB_ASSERT(FALSE, "out of bound");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      pageCountPerFile = getCoreArgs().getMaxPageCountInFile();

      do
      {
         PAGE_ID p = pid + wcnt;
         UINT32 count = 1;
         UINT32 fileId = p / pageCountPerFile;
         for (UINT32 i = wcnt + 1; i < pcnt; ++i)
         {
            UINT32 nextFileId = (p + count) / pageCountPerFile;
            if (nextFileId == fileId)
            {
               ++count;
            }
            else
            {
               break;
            }
         }

         storageFile *file = _files.get<storageFile>(fileId);
         if (OSS_UNLIKELY(nullptr == file))
         {
            PD_LOG(PDERROR, "failed to get file[%d]", fileId);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         rc = file->writePages(p % pageCountPerFile, count,
                               data + (wcnt * getCoreArgs().pageSize));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to read file pages:%d", rc);
            goto error;
         }

         wcnt += count;

      } while(wcnt < pcnt);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileCluster::ensureSegmentCount(UINT32 minSegmentCount)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      while (getTotalSegmentCount() < minSegmentCount)
      {
         rc = extendNewSegment();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new segment:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileCluster::allocateNewSegment(UINT32 count)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         rc = extendNewSegment();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new segment:%d", rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileCluster::ensurePage(PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      UINT32 minSegmentCount = 0;

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

      minSegmentCount = (pid / getCoreArgs().maxPageCountPerSeg) + 1;
      rc = ensureSegmentCount(minSegmentCount);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   storageFile *storageFileCluster::createFilePtr()
   {
      return SDB_OSS_NEW storageFile();
   }

   void storageFileCluster::releaseFilePtr(storageFile *ptr)
   {
      if (nullptr != ptr)
      {
         SDB_OSS_DEL ptr;
      }
   }

   INT32 storageFileCluster::extendNewSegment()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");

      if (0 == _files.getSize())
      {
         rc = createNewFile();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new storage file:%d", rc);
            goto error;
         }
      }
      else
      {
         storageFile *file = _files.get<storageFile>(_files.getSize() - 1);
         SDB_ASSERT(nullptr != file, "the last file can not be null");
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

      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageFileCluster::createNewFile()
   {
      INT32 rc = SDB_OK;
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");

      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest.sid);

      createStorageFileOptions o;
      storageFileName fn;

      storageFile *file = createFilePtr();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(_manifest.ftype, _manifest.stype, _files.getSize()))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = _manifest.secretValue;
      o.flags = _ctl;

      rc = sfm.createStorageFile(fn, o, *file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new file[%s], rc:%d", fn.getFileName(), rc);
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

   INT32 storageFileCluster::getPageMmapPtr(PAGE_ID pid,
                                            mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      UINT32 fileId = 0;
      storageFile *file = nullptr;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      ossValuePtr p = 0;
      ptr.reset();

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

      fileId = getFileId(pid, &pidInFile);
      if (_files.getSize() <= fileId)
      {
         SDB_ASSERT(FALSE, "out of bound");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = _files.get<storageFile>(fileId);
      if (nullptr == file)
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

   ossValuePtr storageFileCluster::getPageMmapPtr(PAGE_ID pid)const
   {
      UINT32 fileId = 0;
      storageFile *file = nullptr;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      ossValuePtr p = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         goto error;
      }

      fileId = getFileId(pid, &pidInFile);
      if (_files.getSize() <= fileId)
      {
         SDB_ASSERT(FALSE, "out of bound");
         goto error;
      }

      file = _files.get<storageFile>(fileId);
      if (nullptr == file)
      {
         PD_LOG(PDERROR, "file[%d] does not exist", fileId);
         goto error;
      }

      file->getPagePtr(pidInFile, p);
   done:
      return p;
   error:
      goto done;
   }

   INT32 storageFileCluster::fsyncSegment(UINT32 globalSegmentId, BOOLEAN sync)const
   {
      INT32 rc = SDB_OK;
      storageFile *file = nullptr;
      UINT32 fileId = 0;
      UINT32 segmentInFile = 0;
      const storageCoreArgs &args = getCoreArgs();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      fileId = globalSegmentId / args.maxSegmentCountPerFile;
      if (_files.getSize() <= fileId)
      {
         SDB_ASSERT(FALSE, "out of bound");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = _files.get<storageFile>(fileId);
      if (nullptr == file)
      {
         PD_LOG(PDERROR, "file[%d] does not exist", fileId);
         rc = SDB_FNE;
         goto error;
      }

      segmentInFile = globalSegmentId % args.maxSegmentCountPerFile;
      rc = file->fsyncSegment(segmentInFile, sync);
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

   INT32 storageFileCluster::fysncPage(PAGE_ID pid, BOOLEAN sync)const
   {
      INT32 rc = SDB_OK;
      UINT32 fileId = 0;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      storageFile *file = nullptr;
      
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      fileId = getFileId(pid, &pidInFile);
      if (_files.getSize() <= fileId)
      {
         SDB_ASSERT(FALSE, "out of bound");
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

      rc = file->fsyncPage(pidInFile, sync);
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

   UINT32 storageFileCluster::getFileId(PAGE_ID pid, PAGE_ID *pidInFile)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      UINT32 pageCountPerFile = args.getMaxPageCountInFile();
      UINT32 fileId = pid / pageCountPerFile;
      if (nullptr != pidInFile)
      {
         *pidInFile = pid % pageCountPerFile;
      }
      return fileId;
   }

   INT32 storageFileCluster::fsyncFile(UINT32 fileId)const
   {
      INT32 rc = SDB_OK;
      storageFile *file = nullptr;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(_files.getSize() < fileId))
      {
         SDB_ASSERT(FALSE, "out of bound");
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      file = _files.get<storageFile>(fileId);

      rc = file->fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file[%s], rc:%d",
                file->getFullPath(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
