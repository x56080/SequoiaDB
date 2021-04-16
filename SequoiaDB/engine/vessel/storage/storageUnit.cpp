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

   Source File Name = storageUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/storageUnit.h"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/dataExtentIDMapFile.h"
#include "vessel/dataExtentFile.h"
#include "utilStr.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "vessel/fsmFile.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   storageUnit::storageUnit():
   _isOpen(FALSE),
   _meta(NULL),
   _idxMeta(NULL),
   _fsm(NULL)
   {
      ossMemset(_dirName, 0, sizeof(_dirName));
   }

   storageUnit::~storageUnit()
   {
      close();
   }

   SPACE_ID storageUnit::getSpaceID()const
   {
      if (isOpen())
      {
         SDB_ASSERT(NULL != _meta, "can not be null");
         return _meta->getCommonHeadInMem().spaceID;
      }
      return INVALID_SPACE_ID;
   }

   UINT32 storageUnit::getMetaSegmentCount()const
   {
      if (OSS_UNLIKELY(NULL != _meta))
      {
         return _meta->getSegmentCount();
      }
      return 0;
   }

   INT32 storageUnit::getPagePtr(FILE_TYPE type,
                                 PAGE_ID pid,
                                 ossValuePtr &ptr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_FILE_TYPE != type, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(isOpen(), "can not be closed");
      if (OSS_UNLIKELY(INVALID_FILE_TYPE == type ||
                       INVALID_PAGE_ID == pid ||
                       !isOpen()))
      {
         PD_LOG(PDERROR, "invalid arg or su closed");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (FILE_TYPE_DM == type)
      {
         rc = _meta->getPagePtr(pid, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (FILE_TYPE_DD == type)
      {
         const storageCoreArgs &args = _meta->getHeadCache().data;
         UINT32 fileSequence = pid / (args.maxPageCountPerSeg * args.maxSegmentCountPerFile);
         PAGE_ID pageInFile = pid % (args.maxPageCountPerSeg * args.maxSegmentCountPerFile);
         dataExtentFile *file = NULL;

         rc = getDataFile(fileSequence, &file);
         if (SDB_FNE == rc)
         {
            rc = SDB_VESSEL_PAGE_NOT_EXISTS;
            goto error;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }

         rc = file->getPagePtr(pageInFile, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
         
      }
      else
      {
         PD_LOG(PDERROR, "invalid space type:%d", type);
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::getMaxPageCountInFile(UINT32 &count)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      count = _meta->getHeadCache().data.getMaxPageCountInFile();
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 storageUnit::getDataFileCount()
   {
      SDB_ASSERT(isOpen(), "must be open");
      ossScopedLock(&_dataFileAccessingMutex, SHARED);
      return _data.size();
   }

   void storageUnit::dumpIDMapFileHead(dataIDMapFileHead &head)
   {
      if (NULL != _meta)
      {
         const dataIDMapFileHead &h = _meta->getHeadCache();
         head.version = h.version;
         head.data = h.data;
         head.meta = h.meta;
         head.index = h.index;
         head.indexMeta = h.indexMeta;
      }
      return;
   }

   INT32 storageUnit::getCoreArgs(FILE_TYPE type,
                                  UINT32 *pageSize,
                                  UINT32 *maxPageCountPerSeg,
                                  UINT32 *maxSegCountPerFile)
   {
      INT32 rc = SDB_OK;
      const dataIDMapFileHead *head = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      SDB_ASSERT(NULL != _meta, "can not be null");
      head = &(_meta->getHeadCache());

      if (FILE_TYPE_DM == type)
      {
         if (NULL != pageSize)
         {
            *pageSize = head->meta.pageSize;
         }
         if (NULL != maxPageCountPerSeg)
         {
            *maxPageCountPerSeg = head->meta.maxPageCountPerSeg;
         }
         if (NULL != maxSegCountPerFile)
         {
            *maxSegCountPerFile = head->meta.maxSegmentCountPerFile;
         }
         
      }
      else if (FILE_TYPE_DD == type)
      {
         if (NULL != pageSize)
         {
            *pageSize = head->data.pageSize;
         }
         if (NULL != maxPageCountPerSeg)
         {
            *maxPageCountPerSeg = head->data.maxPageCountPerSeg;
         }
         if (NULL != maxSegCountPerFile)
         {
            *maxSegCountPerFile = head->data.maxSegmentCountPerFile;
         }
      }
      else if (FILE_TYPE_IDX_D == type)
      {
         if (NULL != pageSize)
         {
            *pageSize = head->index.pageSize;
         }
         if (NULL != maxPageCountPerSeg)
         {
            *maxPageCountPerSeg = head->index.maxPageCountPerSeg;
         }
         if (NULL != maxSegCountPerFile)
         {
            *maxSegCountPerFile = head->index.maxSegmentCountPerFile;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "todo");
         rc = SDB_INVALIDARG;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::fsync(FILE_TYPE type,
                            PAGE_ID pid,
                            UINT32 count,
                            BOOLEAN sync)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid ||
                       0 == count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (FILE_TYPE_DM == type)
      {
         rc = _meta->fsync(pid, count, sync);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (FILE_TYPE_DD == type)
      {
         rc = fsyncDataPages(pid, count);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }


   INT32 storageUnit::create(requestContext *context,
                             const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "must be closed");
      BOOLEAN rollbackDir = FALSE;
      const storagePathOptions *path = NULL;
      strSlice dirSlice;

      if (OSS_UNLIKELY(NULL == context))
      {
         PD_LOG(PDERROR, "invalid ptr");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked() ||
                             options.sid != context->getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!validateSUOptions(options))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!vesselFileName::buildDirName(options.sid, sizeof(_dirName), _dirName))
      {
         PD_LOG(PDERROR, "failed to build dir name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      dirSlice.reset(_dirName);
      path = &(context->getEnv()->options.path);

      rc = testAllDirsBeforeCreating(*path, dirSlice);
      if (SDB_OK != rc)
      {
         goto error;
      }
   
      rc = createAllDirs(*path, dirSlice);
      if (SDB_OK != rc)
      {
         goto error;
      }
      rollbackDir = TRUE;

      rc = createNecessaryFiles(context, options, *path);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _isOpen = TRUE;
   done:
      return rc;
   error:
      if (rollbackDir)
      {
         PD_LOG(PDWARNING, "rollback all files created:%s", _dirName);
         destroy(context);
      }
      close();
      goto done;
   }

   INT32 storageUnit::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      INT32 everRc = SDB_OK;
      const storagePathOptions *path = NULL;
      SDB_ASSERT(NULL != context, "can not be null");

      if (!isOpen())
      {
         goto done;
      }

      for (UINT32 i = 0; i < _idx.size(); ++i)
      {
         extentStorageFile *file = _idx[i];
         if (NULL != file)
         {
            rc = file->unlink();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to unlink idx file in dir[%s], rc:%d", _dirName, rc);
               everRc = rc;
               rc = SDB_OK;
            }
            SDB_OSS_DEL file;
         }
      }
      _idx.clear();

      if (NULL != _idxMeta)
      {
         rc = _idxMeta->unlink();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to unlink idx meta file in dir[%s], rc:%d", _dirName, rc);
            everRc = rc;
            rc = SDB_OK;
         }
         SDB_OSS_DEL _idxMeta;
         _idxMeta = NULL;
      }

      for (UINT32 i = 0; i < _data.size(); ++i)
      {
         dataExtentFile *file = _data[i];
         if (NULL != file)
         {
            rc = file->unlink();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to unlink data file in dir[%s], rc:%d", _dirName, rc);
               everRc = rc;
               rc = SDB_OK;
            }
            SDB_OSS_DEL file;
         }
      }
      _data.clear();

      if (NULL != _fsm)
      {
         rc = _fsm->unlink();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to unlink fsm file in dir[%s], rc:%d", _dirName, rc);
            everRc = rc;
            rc = SDB_OK;
         }
         SDB_OSS_DEL _fsm;
         _fsm = NULL;
      }

      path = &(context->getEnv()->options.path);
      rc = removeSUNameFile(path->dataPath.c_str(), getSpaceID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to unlink su name file:%d", rc);
         everRc = rc;
         rc = SDB_OK;
      }

      if (NULL != _meta)
      {
         rc = _meta->unlink();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to unlink meta file in dir[%s], rc:%d", _dirName, rc);
            everRc = rc;
            rc = SDB_OK;
         }
         SDB_OSS_DEL _meta;
         _meta = NULL;
      }
   
      rc = removeAllDirs(*path, _dirName, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to unlink dirs [%s], rc:%d", _dirName, rc);
         everRc = rc;
         rc = SDB_OK;
      }

      ossMemset(_dirName, 0, sizeof(_dirName));
      _isOpen = FALSE;
      rc = everRc;
   done:
      return rc;
   }

   void storageUnit::close(requestContext *context)
   {
      if (NULL != _fsm && _fsm->isOpen())
      {
         _fsm->ossMmapFile::flushAll(TRUE);
      }
      return close();
   }

   INT32 storageUnit::open(requestContext *context,
                           const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      const storagePathOptions *path = NULL;
      SPACE_ID sid = INVALID_SPACE_ID;
      BOOLEAN crashedImpossible = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       dirName.empty() ||
                       MAX_SPACE_DIR_LEN < dirName.strLen()))
      {
         PD_LOG(PDERROR, "prt is null");
         rc = SDB_INVALIDARG;
         goto error;
      }

      path = &(context->getEnv()->options.path);

      if (OSS_UNLIKELY(!vesselFileName::parseDirName(dirName, &sid)))
      {
         PD_LOG(PDERROR, "invalid su file arg:%s", dirName.str());
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }

      rc = testAllDirsBeforeOpenning(*path, dirName, crashedImpossible);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = openMetaFile(path->dataPath.c_str(), dirName, sid);
      if (SDB_VESSEL_CRASHED_WHEN_CREATING == rc)
      {
         if (crashedImpossible)
         {
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         }
         goto error;
      }
      else if (SDB_OK != rc)
      {
         goto error;
      }

      rc = openOtherFilesUnderPath(path->dataPath.c_str(), dirName);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!path->indexPath.empty() && path->indexPath != path->dataPath)
      {
         rc = openOtherFilesUnderPath(path->indexPath.c_str(), dirName);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = crossCheckFilesWhenOpenning();
      if (SDB_OK != rc)
      {
         goto error;
      }

      ossMemcpy(_dirName, dirName.str(), dirName.strLen());
      _isOpen = TRUE;

   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 storageUnit::extendMetaFile(requestContext *context,
                                     const UINT32 *segmentCount)
   {
      INT32 rc = SDB_OK;
      UINT32 segCount = 0;
      BOOLEAN sparse = context->getEnv()->options.extendFileWithSparse;
      ossScopedLock lock(&_extendingMetaLatch);
      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      segCount = NULL == segmentCount ?
                 _meta->getSegmentCount() + 1 : *segmentCount;
      if (_meta->getCommonHeadInMem().maxSegmentCountPerFile < segCount)
      {
         rc = SDB_VESSEL_FS_UPPER_LIMIT;
         goto error;
      }

      while (_meta->getSegmentCount() < segCount)
      {
         rc = _meta->allocateNewSegment(sparse);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::ensureDataFileSpace(requestContext *context,
                                          PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      UINT32 minSegCount = 0;
      UINT32 sequence = 0;
      PAGE_ID pidInFile = INVALID_PAGE_ID;
      dataExtentFile *file = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sequence = pid / _meta->getHeadCache().data.getMaxPageCountInFile();
      pidInFile = pid % _meta->getHeadCache().data.getMaxPageCountInFile();
      minSegCount = pidInFile / _meta->getHeadCache().data.maxPageCountPerSeg + 1;

      rc = getDataFile(sequence, &file);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (minSegCount <= file->getSegmentCount())
      {
         goto done;
      }

      rc = extendDataFile(context, sequence, minSegCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::removeLastDataFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      UINT32 sequence = 0;
      dataExtentFile *file = NULL;
      ossScopedLock lock(&_extendingDataLatch);

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ///no one can update _data now, coz we are holding _extendingDataLatch
      if (_data.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getDataFile(_data.size() - 1, &file);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = file->destroy();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to destroy data file[%d], space id[%d], rc:%d",
                sequence, getSpaceID(), rc);
         goto error;
      }

      _dataFileAccessingMutex.get();
      _data.pop_back();
      _dataFileAccessingMutex.release();
      SDB_OSS_DEL file;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::extendDataFile(requestContext *context,
                                     UINT32 sequence,
                                     UINT32 minSegCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      dataExtentFile *file = NULL;
      UINT32 count = 0;
      BOOLEAN sparse = context->getEnv()->options.extendFileWithSparse;
      ossScopedLock lock(&_extendingDataLatch);

      rc = getDataFile(sequence, &file);
      if (SDB_OK != rc)
      {
         goto error;
      }

      count = file->getSegmentCount();
      while (count < minSegCount)
      {
         rc = file->allocateNewSegment(sparse);
         if (SDB_OK != rc)
         {
            goto error;
         }
         ++count;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::fsyncDataPages(PAGE_ID pid, UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != _meta, "can not be null");
      UINT32 maxPageCount = _meta->getHeadCache().data.getMaxPageCountInFile();
      UINT32 lastCount = count;
      PAGE_ID currentPid = pid;
      dataExtentFile *file = NULL;

      do
      {
         UINT32 sequence = currentPid / maxPageCount;
         UINT32 pidInFile = currentPid % maxPageCount;
         UINT32 syncCount = 0;
         if ((pidInFile + lastCount) <= maxPageCount)
         {
            syncCount = lastCount;
         }
         else
         {
            syncCount = maxPageCount - pidInFile;
         }

         rc = getDataFile(sequence, &file);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get data file[%d], rc:%d", sequence, rc);
            goto error;
         }

         rc = file->fsync(pidInFile, syncCount, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fsync file[%d], rc:%d", sequence, rc);
            goto error;
         }

         lastCount -= syncCount;
         currentPid += syncCount;
      } while (0 < lastCount);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createDataFile(requestContext *context,
                                     UINT32 *sequenceOfNewFile)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 != ossStrlen(_dirName), "can not be null");
      SDB_ASSERT(NULL != _meta, "can not be null");
      SDB_ASSERT(isOpen(), "can not be closed");
      vesselFileName fn;
      storageFileOptions options;
      dataExtentFile *file = NULL;
      const storageFileHead &commonHead = _meta->getCommonHeadInMem();
      const dataIDMapFileHead &metaHead = _meta->getHeadCache();
      CHAR fullPath[OSS_MAX_PATHSIZE+1] = {0};
      UINT32 sequence = 0;
      BOOLEAN sparse = context->getEnv()->options.extendFileWithSparse;

      ossScopedLock lock(&_extendingDataLatch);

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      /// no one can update file vec now.
      /// no need to lock.
      sequence = _data.size();

      rc = utilBuildFullPath(context->getEnv()->options.path.dataPath.c_str(),
                             _dirName, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%d", rc);
         goto error;
      }

      fn.build(commonHead.spaceID, FILE_TYPE_DD, sequence);

      options.dir = fullPath;
      options.name = fn.getName();
      options.secretValue = commonHead.secretValue;
      options.spaceID = commonHead.spaceID;
      options.args = &(metaHead.data);
      options.sequence = sequence;
      options.logicalID = commonHead.logicalID;

      file = SDB_OSS_NEW dataExtentFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->create(options);
      if (SDB_OK != rc)
      {
         LOG_ERR_AND_REPORT(context, rc, "failed to create data file:%s, rc:%d", fn.getName(), rc);
         goto error;
      }

      rc = file->allocateNewSegment(sparse);
      if (SDB_OK != rc)
      {
         goto error;
      }

      {
      ossScopedLock(&_dataFileAccessingMutex, EXCLUSIVE);
      _data.push_back(file);
      }

      /// from here, do not goto error.
      /// or you must add code to rollback data file sequence.

      if (NULL != sequenceOfNewFile)
      {
         *sequenceOfNewFile = sequence;
      }
      
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

   void storageUnit::close()
   {
      if (!isOpen())
      {
         goto done;
      }

      if (NULL != _meta)
      {
         _meta->close();
         SDB_OSS_DEL _meta;
         _meta = NULL;
      }
      for (UINT32 i = 0; i < _data.size(); ++i)
      {
         dataExtentFile *file = _data[i];
         if (NULL != file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }
      _data.clear();

      if (NULL != _idxMeta)
      {
         _idxMeta->close();
         SDB_OSS_DEL _idxMeta;
         _idxMeta = NULL;
      }
      for (UINT32 i = 0; i < _idx.size(); ++i)
      {
         extentStorageFile *file = _idx[i];
         if (NULL != file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }
      _idx.clear();

      if (NULL != _fsm)
      {
         _fsm->close();
      }

      ossMemset(_dirName, 0, sizeof(_dirName));
      _isOpen = FALSE;
   done:
      return;
   }

   INT32 storageUnit::testAllDirsBeforeOpenning(const storagePathOptions &path,
                                                const strSlice &dirName,
                                                BOOLEAN &impossibleCrashed)
   {
      INT32 rc = SDB_OK;
      UINT32 primaryDirSubFileCount = 0;
      UINT32 otherDirsSubFileCount = 0;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      UINT32 dirNotExists = 0;

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
         goto error;
      }

      rc = testDir(fullPath, primaryDirSubFileCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!path.indexPath.empty() && path.indexPath != path.dataPath)
      {
         UINT32 subFileCount = 0;
         rc = utilBuildFullPath(path.indexPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = testDir(fullPath, subFileCount);
         if (SDB_FNE == rc)
         {
            rc = SDB_OK;
            ++dirNotExists;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         else
         {
            otherDirsSubFileCount += subFileCount;
         }
      }

      if (!path.lobMetaPath.empty() && path.lobMetaPath != path.dataPath)
      {
         UINT32 subFileCount = 0;
         rc = utilBuildFullPath(path.lobMetaPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = testDir(fullPath, subFileCount);
         if (SDB_FNE == rc)
         {
            rc = SDB_OK;
            ++dirNotExists;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         else
         {
            otherDirsSubFileCount += subFileCount;
         }
      }

      if (!path.lobPath.empty() && path.lobPath != path.dataPath)
      {
         UINT32 subFileCount = 0;
         rc = utilBuildFullPath(path.lobPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = testDir(fullPath, subFileCount);
         if (SDB_FNE == rc)
         {
            rc = SDB_OK;
            ++dirNotExists;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         else
         {
            otherDirsSubFileCount += subFileCount;
         }
      }

      if (0 < dirNotExists)
      {
         if (0 < primaryDirSubFileCount || 0 < otherDirsSubFileCount)
         {
            /// we always create all dirs before creating files.
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
            
         }
         else
         {
            rc = SDB_VESSEL_CRASHED_WHEN_CREATING;
            goto error;
         }
      }
      else if (0 == otherDirsSubFileCount)
      {
         if (0 == primaryDirSubFileCount)
         {
            rc = SDB_VESSEL_CRASHED_WHEN_CREATING;
            goto error;
         }
         impossibleCrashed = 1 < primaryDirSubFileCount;
      }
      else if (0 == primaryDirSubFileCount)
      {
         /// we always create files under primary dir.
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         goto error;
      }
      else
      {
         impossibleCrashed = 1 < primaryDirSubFileCount;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::testAllDirsBeforeCreating(const storagePathOptions &path,
                                                const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

      if (path.dataPath.empty() || dirName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
         goto error;
      }

      rc = ossAccess(fullPath);
      if (SDB_OK == rc)
      {
         rc = SDB_FE;
         PD_LOG(PDERROR, "dir already exists:%s", fullPath);
         goto error;
      }
      else if (SDB_FNE == rc)
      {
         rc = SDB_OK;
      }
      else
      {
         goto error;
      }
      
      if (!path.indexPath.empty() && path.dataPath != path.indexPath)
      {
         rc = utilBuildFullPath(path.indexPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossAccess(fullPath);
         if (SDB_OK == rc)
         {
            rc = SDB_FE;
            PD_LOG(PDERROR, "dir already exists:%s", fullPath);
            goto error;
         }
         else if (SDB_FNE == rc)
         {
            rc = SDB_OK;
         }
         else
         {
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::testDir(const CHAR *fullPath,
                             UINT32 &subCount)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != fullPath, "can not be null");
      UINT32 subFileCount = 0;
      fs::directory_iterator end_iter ;
      fs::path dataDir(fullPath);
      if (!fs::exists(dataDir))
      {
         rc = SDB_FNE;
         goto error;
      }
      else if (!fs::is_directory(dataDir))
      {
         rc = SDB_VESSEL_INVALID_VESSEL_FILE;
         PD_LOG(PDERROR, "%s is not a dir", fullPath);
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string fileName = dir_iter->path().filename().string();
         std::string fullPath = dir_iter->path().string();
         if (0 == fileName.compare(".") ||
             0 == fileName.compare(".."))
         {
            continue;
         }

         ++subFileCount;
      }
      subCount = subFileCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createAllDirs(const storagePathOptions &path,
                                    const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      BOOLEAN rollbackDataDir = FALSE;
      BOOLEAN rollbackIndexDir = FALSE;

      if (dirName.empty() || path.dataPath.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
         goto error;
      }

      rc = ossMkdir(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create dir:%s, %d", fullPath, rc);
         goto error;
      }

      rollbackDataDir = TRUE;

      if (!path.indexPath.empty() && path.indexPath != path.dataPath)
      {
         rc = utilBuildFullPath(path.indexPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName.str(), rc);
            goto error;
         }

         rc = ossMkdir(fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create dir:%s, %d", fullPath, rc);
            goto error;
         }
         rollbackIndexDir = TRUE;
      }

   done:
      return rc;
   error:
      if (rollbackIndexDir)
      {
         if (SDB_OK == utilBuildFullPath(path.indexPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath))
         {
            removeDir(fullPath, TRUE);
         }
      }

      if (rollbackDataDir)
      {
         if(SDB_OK == utilBuildFullPath(path.dataPath.c_str(), dirName.str(), OSS_MAX_PATHSIZE, fullPath))
         {
            removeDir(fullPath, TRUE);
         }
      }
      goto done;
   }

   INT32 storageUnit::removeAllDirs(const storagePathOptions &path,
                                    const CHAR *dirName,
                                    BOOLEAN mustBeEmpty)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != dirName, "can not be null");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};

      if (NULL == dirName || ossStrlen(dirName) == 0)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      rc = utilBuildFullPath(path.dataPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName, rc);
         goto error;
      }

      rc = ossAccess(fullPath);
      if (SDB_FNE == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to access dir:%s, %d", dirName, rc);
         goto error;
      }
      else
      {
         rc = removeDir(fullPath, mustBeEmpty);
         if (SDB_OK != rc)
         {
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
      }

      if (!path.indexPath.empty() && path.dataPath != path.indexPath)
      {
         rc = utilBuildFullPath(path.indexPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName, rc);
            goto error;
         }

         rc = ossAccess(fullPath);
         if (SDB_FNE == rc)
         {
            rc = SDB_OK;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to access dir:%s, %d", dirName, rc);
            goto error;
         }
         else
         {
            rc = removeDir(fullPath, mustBeEmpty);
            if (SDB_OK != rc)
            {
               rc = SDB_VESSEL_INVALID_VESSEL_FILE;
               goto error;
            }
         }  
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::removeDir(const CHAR *fullPath, BOOLEAN mustBeEmpty)
   {
      INT32 rc = SDB_OK;
      UINT32 subFileCount = 0;
      fs::directory_iterator end_iter ;
      fs::path dataDir(fullPath);
      if (!fs::exists(dataDir))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!fs::is_directory(dataDir))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "%s is not a dir", fullPath);
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string fileName = dir_iter->path().filename().string();
         std::string fullPath = dir_iter->path().string();
         if (0 == fileName.compare(".") ||
             0 == fileName.compare(".."))
         {
            continue;
         }

         ++subFileCount;
      }

      if (mustBeEmpty && 0 < subFileCount)
      {
         PD_LOG(PDERROR, "dir is not empty:%s", fullPath);
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossDelete(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove dir:%s, %d", fullPath, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createNecessaryFiles(requestContext *context,
                                           const createSUOptions &options,
                                           const storagePathOptions &path)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      SDB_ASSERT(NULL != context, "can not be null");

      rc = utilBuildFullPath(path.dataPath.c_str(), _dirName, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", _dirName, rc);
         goto error;
      }

      rc = createMetaFile(context, fullPath, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create the meta file:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createMetaFile(requestContext *context,
                                     const CHAR *dir,
                                     const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != dir, "can not be null");
      SDB_ASSERT(NULL == _meta, "must be null");
      vesselFileName fn;
      storageFileOptions fileOptions;
      UINT32 secretValue = ossRand();
      SPACE_ID sid = options.sid;
      dataExtentIDMapFile *file = NULL;

      fn.build(sid, FILE_TYPE_DM, 0);
      fileOptions.dir = dir;
      fileOptions.name = fn.getName();
      fileOptions.secretValue = secretValue;
      fileOptions.spaceID = sid;
      fileOptions.logicalID = options.logicalID;
      fileOptions.args = &(options.metaArgs);
      fileOptions.sequence = 0;

      file = SDB_OSS_NEW dataExtentIDMapFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->create(fileOptions, &options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to crate meta file:%d", rc);
         goto error;
      }

      rc = file->cacheHead();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache head in mem:%d", rc);
         goto error;
      }

      _meta = file;
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

   INT32 storageUnit::openMetaFile(const CHAR *storagePath,
                                   const strSlice &dirName,
                                   SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != storagePath && !dirName.empty(), "can not be null");
      CHAR subPath[MAX_FILE_NAME_LEN+MAX_SPACE_DIR_LEN+2] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      vesselFileName fn;
      fn.build(sid, FILE_TYPE_DM, 0);
      rc = utilBuildFullPath(dirName.str(), fn.getName(), MAX_FILE_NAME_LEN+MAX_SPACE_DIR_LEN+2, subPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", dirName.str(), fn.getName(), rc);
         goto error;
      }

      rc = utilBuildFullPath(storagePath, subPath, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", storagePath, subPath, rc);
         goto error;
      }

      rc = openFile(fullPath, fn);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _meta->cacheHead();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache head in mem:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::openOtherFilesUnderPath(const CHAR *path,
                                              const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != path && !dirName.empty(), "can not be null");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      rc = utilBuildFullPath(path, dirName.str(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", path, dirName.str(), rc);
         goto error;
      }

      {
      fs::directory_iterator end_iter ;
      fs::path dataDir(fullPath);
      if (!fs::exists(dataDir) || !fs::is_directory(dataDir))
      {
         PD_LOG(PDERROR, "invalid data path:%s", fullPath);
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string fileName = dir_iter->path().filename().string();
         std::string fullPath = dir_iter->path().string();
         PD_LOG(PDDEBUG, "found file:[%s]", fileName.c_str());
         
         vesselFileName fn;

         if (!fs::is_regular_file(dir_iter->status()))
         {
            PD_LOG(PDERROR, "not regular", fileName.c_str());
            continue;
         }

         if (!fn.extract(strSlice(fileName.c_str(), fileName.size()), NULL))
         {
            PD_LOG(PDWARNING, "invalid file name:%s", fileName.c_str());
            continue;
         }

         if (fn.getType() == FILE_TYPE_DM)
         {
            continue;
         }

         rc = openFile(fullPath.c_str(), fn);
         if (SDB_VESSEL_CRASHED_WHEN_CREATING == rc)
         {
            PD_LOG(PDWARNING, "found su file which crashed when creating:%s", fullPath.c_str());
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open file:%s", fileName.c_str());
            goto error;
         }
      }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::openFile(const CHAR *fullPath,
                               const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      extentStorageFile *ef = NULL;
      SDB_ASSERT(NULL != fullPath, "can not be null");
      SDB_ASSERT(fn.isValid(), "can not be invalid");

      switch (fn.getType())
      {
      case FILE_TYPE_DM:
      {
         if (NULL != _meta)
         {
            PD_LOG(PDERROR, "duplicated meta file:%s", fullPath);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         ef = SDB_OSS_NEW dataExtentIDMapFile();
         if (NULL == ef)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = ef->open(fullPath, fn);
         if (SDB_OK != rc)
         {
            goto error;
         }

         _meta = (dataExtentIDMapFile *)ef;
         ef = NULL;
         break;
      }
      case FILE_TYPE_DD:
      {
         _data.resize(fn.getSequence() + 1, NULL);
         if (NULL != _data.at(fn.getSequence()))
         {
            PD_LOG(PDERROR, "duplicated data file:%s", fullPath);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         ef = SDB_OSS_NEW dataExtentFile();
         if (NULL == ef)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = ef->open(fullPath, fn);
         if (SDB_OK != rc)
         {
            /// null ptr leaved in _data. it will be recreate if necessary.
            goto error;
         }
         _data[fn.getSequence()] = (dataExtentFile *)ef;
         ef = NULL;
         break;
      }
      case FILE_TYPE_CS_NAME:
      {
         break;
      }
      case FILE_TYPE_FSM:
      {
         ef = SDB_OSS_NEW fsmFile();
         if (NULL == ef)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = ef->open(fullPath, fn);
         if (SDB_OK != rc)
         {
            goto error;
         }

         _fsm = (fsmFile *)ef;
         ef = NULL;
         break;   
      }
      default:
      {
         PD_LOG(PDERROR, "unknown file type:%d", fn.getType());
         rc = SDB_INVALIDARG;
         goto error;
      }
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(ef);
      goto done;
   }

   INT32 storageUnit::ensureSUNameFile(requestContext *context,
                                       const strSlice &csName)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      SDB_ASSERT(!csName.empty() && csName.strLen() <= DMS_COLLECTION_SPACE_NAME_SZ, "can not be invalid");
      CHAR filePath[MAX_SPACE_DIR_LEN + MAX_FILE_NAME_LEN + 2] = {0};
      vesselFileName fn;
      OSSFILE file;
      CHAR buf[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {0};
      const CHAR *path = NULL;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if(NULL == context || csName.empty() ||
              DMS_COLLECTION_SPACE_NAME_SZ < csName.strLen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fn.build(getSpaceID(), FILE_TYPE_CS_NAME, 0);
      rc = utilBuildFullPath(_dirName, fn.getName(), MAX_SPACE_DIR_LEN + MAX_FILE_NAME_LEN + 2, filePath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build file path:%d", rc);
         goto error;
      }

      path = context->getEnv()->options.path.dataPath.c_str();
      rc = utilBuildFullPath(path, filePath, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossOpen(fullPath,
                  OSS_REPLACE | OSS_READWRITE | OSS_EXCLUSIVE,
                  OSS_DEFAULTFILE,
                  file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file:%s, %d", fullPath, rc);
         goto error;
      }

      ossMemcpy(buf, csName.str(), csName.strLen());
      buf[csName.strLen()] = '\n';

      rc = ossWriteN(&file, buf, csName.strLen() + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write name file:%s, %d", fullPath, rc);
         goto error;
      }

      rc = ossFsync(&file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync name file:%s, %d", fullPath, rc);
         goto error;
      }

      ossClose(file);
      ossChmod(fullPath, OSS_RU);

   done:
      return rc;
   error:
      if (file.isOpened())
      {
         ossClose(file);
         ossDelete(fullPath);
      }
      goto done;
   }

   INT32 storageUnit::removeSUNameFile(const CHAR *dataPath,
                                       SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      CHAR filePath[MAX_SPACE_DIR_LEN + MAX_FILE_NAME_LEN + 2] = {0};
      SDB_ASSERT(NULL != dataPath &&
                 INVALID_SPACE_ID != sid, "can not be null");
      vesselFileName fn;
      OSSFILE file;

      fn.build(sid, FILE_TYPE_CS_NAME, 0);
      rc = utilBuildFullPath(_dirName, fn.getName(), MAX_SPACE_DIR_LEN + MAX_FILE_NAME_LEN + 2, filePath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build file path:%d", rc);
         goto error;
      }
      rc = utilBuildFullPath(dataPath, filePath, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossDelete(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to delete file:%s", fullPath);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::createFsmFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL == _fsm, "do not recreate");
      vesselFileName fn;
      storageFileOptions options;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      const CHAR *dataPath = NULL;
      storageCoreArgs args(FSM_PAGE_SIZE,
                           FSM_PAGE_COUNT_PER_SEG,
                           FSM_MAX_SEG_COUNT);
      fsmFile *file = NULL;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fn.build(getSpaceID(), FILE_TYPE_FSM, 0);

      dataPath = context->getEnv()->options.path.dataPath.c_str();
      rc = utilBuildFullPath(dataPath, _dirName, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         goto error;
      }

      options.dir = fullPath;
      options.name = fn.getName();
      options.secretValue = _meta->getCommonHeadInMem().secretValue;
      options.spaceID = getSpaceID();
      options.sequence = 0;
      options.args = &args;
      options.logicalID = _meta->getCommonHeadInMem().logicalID;
      options.replaceWhenCreate = TRUE;

      file = SDB_OSS_NEW fsmFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->create(options, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create free space map file:%d", rc);
         goto error;
      }

      _fsm = file;
      file = NULL;

   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->destroy();
      }
      SAFE_OSS_DELETE(file);
      goto done;
   }

   INT32 storageUnit::getDataFile(UINT32 fileSequence, dataExtentFile **file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != file, "can not be null");
      SDB_ASSERT(isOpen(), "can not be closed");
      ossScopedLock lock(&_dataFileAccessingMutex, SHARED);
      if (_data.size() <= fileSequence)
      {
         PD_LOG(PDDEBUG, "current file count:%d, file to access:%d", _data.size(), fileSequence);
         rc = SDB_FNE;
         goto error;
      }
      
      *file = _data[fileSequence];
      if (NULL == file)
      {
         PD_LOG(PDERROR, "file has not been open yet:%d", fileSequence);
         rc = SDB_FNE;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::crossCheckFilesWhenOpenning()
   {
      INT32 rc = SDB_OK;
      const storageFileHead *commonHead = NULL;
      const dataIDMapFileHead *metaHead = NULL;
      SDB_ASSERT(NULL != _meta, "can not be null");

      commonHead = &(_meta->getCommonHeadInMem());
      metaHead = &(_meta->getHeadCache());
      for (UINT32 i = 0; i < _data.size(); ++i)
      {
         dataExtentFile *df = _data.at(i);
         if (NULL == df)
         {
            continue;
         }

         const storageFileHead *dataHead = &(df->getCommonHeadInMem());
         if (commonHead->spaceID != dataHead->spaceID)
         {
            PD_LOG(PDERROR, "space id is not same:%d, %d", commonHead->spaceID, dataHead->spaceID);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (commonHead->secretValue != dataHead->secretValue)
         {
            PD_LOG(PDERROR, "secret value is not same:%d, %d", commonHead->secretValue, dataHead->secretValue);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (commonHead->logicalID != dataHead->logicalID)
         {
            PD_LOG(PDERROR, "logical id value is not same:%d, %d", commonHead->logicalID, dataHead->logicalID);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (metaHead->data.pageSize != dataHead->pageSize)
         {
            PD_LOG(PDERROR, "page size is not same:%d, %d", metaHead->data.pageSize, dataHead->pageSize);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (metaHead->data.maxSegmentCountPerFile != dataHead->maxSegmentCountPerFile)
         {
            PD_LOG(PDERROR, "maxSegmentCountPerFile is not same:%d, %d", metaHead->data.maxSegmentCountPerFile, dataHead->maxSegmentCountPerFile);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (metaHead->data.maxPageCountPerSeg != dataHead->maxPageCountPerSeg)
         {
            PD_LOG(PDERROR, "maxPageCountPerSeg is not same:%d, %d", metaHead->data.maxPageCountPerSeg, dataHead->maxPageCountPerSeg);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN storageUnit::validateSUOptions(const createSUOptions &options)
   {
      BOOLEAN r = FALSE;
      if (INVALID_SPACE_ID == options.sid ||
          MAX_SPACE_ID < options.sid)
      {
         goto done;
      }
      else if (DMS_INVALID_LOGICCSID == options.logicalID)
      {
         goto done;
      }
      else if (options.csName.empty())
      {
         goto done;
      }
      else if (!options.dataArgs.isValid())
      {
         goto done;
      }
      else if (!options.metaArgs.isValid())
      {
         goto done;
      }
      else if (!options.idxArgs.isValid())
      {
         goto done;
      }
      else if (!options.idxMetaArgs.isValid())
      {
         goto done;
      }

      r = TRUE;
   done:
      return r;
   }
}//namespace vessel
}//namespace engine