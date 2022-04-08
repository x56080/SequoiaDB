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
#include "vessel/storageFileMaintainer.h"
#include "ossLatchGuard.hpp"
#include "vessel/threadContext.h"
#include "vessel/instanceEnv.h"

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

   INT32 dataStorageFileCluster::open(SPACE_ID sid,
                                      SPACE_TYPE type,
                                      UINT32 secretValue,
                                      const storageFileLoader *loader, 
                                      const storageCoreArgs &args,
                                      const options &o)
   {
      INT32 rc = SDB_OK;
      close();

      if (INVALID_SPACE_ID == sid ||
          INVALID_SPACE_TYPE == type ||
          !args.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _sid = sid;
      _type = type;
      _secretValue = secretValue;
      _args = args;
      _o = o;
      if (_o.segmentCountAutoExtending < 0)
      {
         _o.segmentCountAutoExtending = 1;
      }

      rc = openFiles(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open files:%d", rc);
         goto error;
      }

      rc = initAllocator();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init allocator:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 dataStorageFileCluster::initAllocator()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(getCoreArgs().isValid(), "can not be invalid");
      inMemBitmap::options bo;
      bo.percentFreeReused = _o.segmentReusedMinFreePercent;

      rc = _allocator.initWithNoLatch(_args.maxPageCountPerSeg, bo);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page allocator:%d", rc);
         goto error;
      }

      if (0 < _segmentsCreatedEver)
      {
         rc = _allocator.allocateNewBitmapPages(_segmentsCreatedEver);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate bitmap pages:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      _allocator.fini();
      goto done;
   }

   void dataStorageFileCluster::close()
   {
      _close();
      _reset();
   }

   void dataStorageFileCluster::_close()
   {
      _allocator.fini();
      closeFiles();
      return;
   }

   INT32 dataStorageFileCluster::allocatePages(UINT32 count,
                                               PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(&_latch, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count ||
                            _args.maxPageCountPerSeg < count ||
                            NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.lock();
      rc = _allocator.allocateBits(count, pids, 0);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_VESSEL_NOT_ENOUGH_FREE_RESOURCE != rc)
      {
         PD_LOG(PDERROR, "failed to allocate from bitmap:%d", rc);
         goto error;
      }
      else if (0 == _o.segmentCountAutoExtending)
      {
         goto error;
      }
      else if (_allocator.getCustomizedPageCount() == _segmentsCreatedEver)
      {                                 
         rc = _createNewSegment(_o.segmentCountAutoExtending);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create new segment on disk:%d", rc);
            goto error;
         }
      }

      SDB_ASSERT(_allocator.getCustomizedPageCount() < _segmentsCreatedEver, "impossible");
      rc = _allocator.allocateNewBitmapPages(_segmentsCreatedEver - 
                                             _allocator.getCustomizedPageCount());
      if (SDB_OK != rc)
      {
         /// No need to do anything to rollback file.
         /// Just wait for the next extending.
         PD_LOG(PDERROR, "failed to map new segment to allocator:%d", rc);
         goto error;
      }

      rc = _allocator.allocateBits(count, pids, 0);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "unexpected failed allocating:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::occupyPages(UINT32 count,
                                             const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(&_latch, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == count || NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.lock();
      rc = _allocator.occupy(count, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy pages:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void dataStorageFileCluster::releasePages(UINT32 count,
                                             const PAGE_ID *pids)
   {
      if (OSS_UNLIKELY(!isOpen()))
      {
         SDB_ASSERT(FALSE, "must be open");
         goto done;
      }
      else if (OSS_UNLIKELY(0 == count ||
                            NULL == pids))
      {
         SDB_ASSERT(FALSE, "invalid args");
         goto done;
      }

      {
         ossXLatchGuard guard(&_latch);
         _allocator.releaseBits(count, pids);
      }
   done:
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

   INT32 dataStorageFileCluster::ensurePidSpace(PAGE_ID pid)
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

      minSegmentCount = pid / _args.maxPageCountPerSeg + 1;
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

   INT32 dataStorageFileCluster::ensureSegmentCount(UINT32 totalSegmentCount)
   {
      INT32 rc = SDB_OK;
      ossXLatchGuard guard(&_latch, FALSE);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (totalSegmentCount <= _allocator.getCustomizedPageCount())
      {
         goto done;
      }

      guard.lock();

      while (_allocator.getCustomizedPageCount() < totalSegmentCount)
      {
         if (_allocator.getCustomizedPageCount() == _segmentsCreatedEver)
         {
            rc = _createNewSegment(1);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to create new segment on disk:%d", rc);
               goto error;
            }
         }

         rc = _allocator.allocateNewBitmapPages(1);
         if (SDB_OK != rc)
         {
            /// No need to do anything to rollback file.
            /// Just wait for the next allocating.
            PD_LOG(PDERROR, "failed to allocate new page in bitmap:%d", rc);
            goto error;
         }
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

   INT32 dataStorageFileCluster::openFiles(const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(0 == _segmentsCreatedEver && 0 == _files.getSize(), "do not reopen");
      const storageCoreArgs &args = getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");
      constexpr UINT32 MAX_FILE_SEQUENCE = 1048575;
      
      UINT32 flags = storageFileCtlFlag::MMAP_DATA_SEGMENT;
      storageFile *file = NULL;
      const STORAGE_FILE_NAME_LIST *fileList = NULL;
      constexpr UINT32 DEFAULT_CAPACITY = 16;

      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _sid);

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
         else if (FILE_TYPE_DATA_STORAGE != fn.getFileType())
         {
            PD_LOG(PDERROR, "invalid file type:%s", fn.getFileName());
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

         rc = sfm.openStorageFile(fn, flags, *file);
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

         sequence = fn.getSequence();
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

         _segmentsCreatedEver = lastFile->getSegmentCount();
         if (1 < _files.getSize())
         {
            _segmentsCreatedEver += (_files.getSize() - 1) * getCoreArgs().maxSegmentCountPerFile;
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
      closeFiles();
      goto done;
   }

   void dataStorageFileCluster::closeFiles()
   {
      _segmentsCreatedEver = 0;
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
   }

   void dataStorageFileCluster::destroyFiles()
   {
      _segmentsCreatedEver = 0;
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
      _files.fini();
   }

   void dataStorageFileCluster::destroy()
   {
      _allocator.fini();
      destroyFiles();
      return;
   }

   INT32 dataStorageFileCluster::_createNewSegment(UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(dataPageCluster::isOpen(), "must be open");
      SDB_ASSERT(_args.isValid(), "must be valid");
      SDB_ASSERT(0 < count, "can not be zero");

      for (UINT32 i = 0; i < count; ++i)
      {
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
               rc = createNewFile();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to create new storage file:%d", rc);
                  goto error;
               }
            }
         }

         ++_segmentsCreatedEver;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataStorageFileCluster::createNewFile()
   {
      INT32 rc = SDB_OK;
      const storageCoreArgs &args = dataPageCluster::getCoreArgs();
      SDB_ASSERT(args.isValid(), "must be valid");

      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _sid);

      createStorageFileOptions o;
      o.flags = storageFileCtlFlag::MMAP_DATA_SEGMENT;
      storageFileName fn;

      storageFile *file = SDB_OSS_NEW storageFile();
      if (OSS_UNLIKELY(NULL == file))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      if (!fn.build(FILE_TYPE_DATA_STORAGE, getSpaceType(), _files.getSize()))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.secretValue = getSecretValue();

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

   UINT32 dataStorageFileCluster::getTotalSegmentCount()
   {
      return _allocator.getCustomizedPageCount();
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

   INT32 dataStorageFileCluster::readPages(PAGE_ID pid,
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

   INT32 dataStorageFileCluster::writePages(PAGE_ID pid,
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
}//namespace vessel
}//namespace engine