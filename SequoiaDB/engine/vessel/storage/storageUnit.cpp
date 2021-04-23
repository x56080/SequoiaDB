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
#include "vessel/idxDataFile.h"
#include "vessel/idxIDMapFile.h"
#include "vessel/deltaLogFile.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   storageUnit::storageUnit():
   _isOpen(FALSE),
   _meta(NULL),
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

   UINT32 storageUnit::getLogicalID()const
   {
      if (isOpen())
      {
         SDB_ASSERT(NULL != _meta, "can not be null");
         return _meta->getCommonHeadInMem().logicalID;
      }
      return DMS_INVALID_LOGICCSID;
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
      const storageCoreArgs *args = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      SDB_ASSERT(NULL != _meta, "can not be null");
      head = &(_meta->getHeadCache());
      
      switch (type)
      {
      case FILE_TYPE_DM:
         args = &(head->meta);
         break;
      case FILE_TYPE_DD:
         args = &(head->data);
         break;
      case FILE_TYPE_IDX_M:
         args = &(head->indexMeta);
         break;
      case FILE_TYPE_IDX_D:
         args = &(head->index);
         break;
      default:
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      SDB_ASSERT(NULL != args, "can not be null");
      if (NULL != pageSize)
      {
         *pageSize = args->pageSize;
      }
      if (NULL != maxPageCountPerSeg)
      {
         *maxPageCountPerSeg = args->maxPageCountPerSeg;
      }
      if (NULL != maxSegCountPerFile)
      {
         *maxSegCountPerFile = args->maxSegmentCountPerFile;
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

      rc = createNecessaryFiles(context, options);
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

   void storageUnit::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      const storagePathOptions *path = NULL;
      SDB_ASSERT(NULL != context, "can not be null");

      for (_DELTA_LIST::iterator itr = _delta.begin();
           itr != _delta.end(); ++itr)
      {
         deltaLogFile *file = *itr;
         if (NULL != file)
         {
            rc = file->destroy();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destory delta file:%d", rc);
            }
            SDB_OSS_DEL file;
         }
      }
      _delta.clear();

      for (_INDEX_DATA_VEC::iterator itr = _idxDataVec.begin();
           itr != _idxDataVec.end(); ++itr)
      {
         idxDataFile *file = *itr;
         if (NULL != file)
         {
            rc = file->destroy();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destory idx data file:%d", rc);
            }
            SDB_OSS_DEL file;
         }
      }
      _idxDataVec.clear();

      for (_INDEX_META_LIST::iterator itr = _idxMetaList.begin();
           itr != _idxMetaList.end(); ++itr)
      {
         idxIDMapFile *file = *itr;
         if (NULL != file)
         {
            rc = file->destroy();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to destory idx meta file:%d", rc);
            }
            SDB_OSS_DEL file;
         }
      }
      _idxMetaList.clear();

      if (NULL != _fsm)
      {
         rc = _fsm->destroy();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to unlink fsm file in dir[%s], rc:%d", _dirName, rc);
         }
         SDB_OSS_DEL _fsm;
         _fsm = NULL;
      }

      for (UINT32 i = 0; i < _data.size(); ++i)
      {
         dataExtentFile *file = _data[i];
         if (NULL != file)
         {
            rc = file->destroy();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to unlink data file in dir[%s], rc:%d", _dirName, rc);
            }
            SDB_OSS_DEL file;
         }
      }
      _data.clear();

      path = &(context->getEnv()->options.path);
      rc = removeCSNameFile(strSlice(path->dataPath.c_str(), path->dataPath.size()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to unlink su name file:%d", rc);
         rc = SDB_OK;
      }

      if (NULL != _meta)
      {
         rc = _meta->destroy();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to unlink meta file in dir[%s], rc:%d", _dirName, rc);
         }
         SDB_OSS_DEL _meta;
         _meta = NULL;
      }
   
      rc = removeAllDirs(*path, _dirName, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to unlink dirs [%s], rc:%d", _dirName, rc);
      }

      ossMemset(_dirName, 0, sizeof(_dirName));
      _isOpen = FALSE;
   done:
      return;
   }

   INT32 storageUnit::open(requestContext *context,
                           const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      const storagePathOptions *path = NULL;
      SPACE_ID sid = INVALID_SPACE_ID;
      BOOLEAN crashedImpossible = FALSE;
      vesselFileName fn;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      strSlice pathSlice;

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

      if (!fn.build(sid, FILE_TYPE_DM, 0))
      {
         PD_LOG(PDERROR, "failed to build filename");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = testAllDirsBeforeOpenning(*path, dirName, crashedImpossible);
      if (SDB_OK != rc)
      {
         goto error;
      }

      pathSlice.reset(path->dataPath.c_str(), path->dataPath.size());
      rc = buildFileFullPath(pathSlice, dirName, fn,
                             OSS_MAX_PATHSIZE + 1, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path of file:%s, rc:%d",
                fn.getName(), rc);
         goto error;
      }

      pathSlice.reset(fullPath);

      rc = openMetaFile(pathSlice, fn);
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
      BOOLEAN sparse = FALSE;
      ossScopedLock lock(&_extendingDDAndDMLatch);
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
      
      sparse = context->getEnv()->options.extendFileWithSparse;
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
      dataExtentFile *file = NULL;
      ossScopedLock lock(&_extendingDDAndDMLatch);

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

      ///no one can update _data now, coz we are holding extending latch
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

      file->destroy();
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
      ossScopedLock lock(&_extendingDDAndDMLatch);

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

   INT32 storageUnit::createNewDataFile(requestContext *context,
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
      BOOLEAN sparse = FALSE;

      ossScopedLock lock(&_extendingDDAndDMLatch);

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      /// no one can update file vec now.
      /// no need to lock.
      sequence = _data.size();
      sparse = context->getEnv()->options.extendFileWithSparse;

      if (sequence < _data.size())
      {
         goto done;
      }

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
      for (_DELTA_LIST::iterator itr = _delta.begin();
           itr != _delta.end(); ++itr)
      {
         deltaLogFile *file = *itr;
         if (NULL != file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }
      _delta.clear();

      for (_INDEX_DATA_VEC::iterator itr = _idxDataVec.begin();
           itr != _idxDataVec.end(); ++itr)
      {
         idxDataFile *file = *itr;
         if (NULL != file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }
      _idxDataVec.clear();

      for (_INDEX_META_LIST::iterator itr = _idxMetaList.begin();
           itr != _idxMetaList.end(); ++itr)
      {
         idxIDMapFile *file = NULL;
         if (NULL == file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }
      _idxMetaList.clear();

      if (NULL != _fsm)
      {
         _fsm->close();
         SDB_OSS_DEL _fsm;
         _fsm = NULL;
      }
      
      for (_DATA_VEC::iterator itr = _data.begin();
           itr != _data.end(); ++itr)
      {
         dataExtentFile *file = *itr;
         if (NULL != file)
         {
            file->close();
            SDB_OSS_DEL file;
         }
      }
      _data.clear();

      if (NULL != _meta)
      {
         _meta->close();
         SDB_OSS_DEL _meta;
         _meta = NULL;
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
                                           const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");

      rc = createMetaFile(context, options);
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
                                     const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL == _meta, "must be null");
      vesselFileName fn;
      storageFileOptions fileOptions;
      UINT32 secretValue = ossRand();
      SPACE_ID sid = options.sid;
      dataExtentIDMapFile *file = NULL;
      CHAR dirName[MAX_SPACE_DIR_LEN + 1] = {0};
      CHAR *fullDirBuf = NULL;
      UINT32 fullDirBufSize = 0;
      strSlice pathSlice;
      const storagePathOptions &path = context->getEnv()->options.path;
      pathSlice.reset(path.dataPath.c_str(), path.dataPath.size());

      if (!vesselFileName::buildDirName(options.sid, MAX_SPACE_DIR_LEN + 1,
                                        dirName))
      {
         PD_LOG(PDERROR, "failed to build dirname");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      fullDirBufSize = pathSlice.strLen() + MAX_SPACE_DIR_LEN + 8;
      fullDirBuf = context->allocateBuffer(fullDirBufSize);
      if (NULL == fullDirBuf)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = utilBuildFullPath(pathSlice.str(), dirName, fullDirBufSize, fullDirBuf);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full dir:%d", rc);
         goto error;
      }

      fn.build(sid, FILE_TYPE_DM, 0);
      fileOptions.dir = fullDirBuf;
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
      file = NULL;
   done:
      if (NULL != fullDirBuf)
      {
         context->releaseBuffer(fullDirBuf, fullDirBufSize);
      }
      return rc;
   error:
      if (NULL != file)
      {
         file->destroy();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 storageUnit::openMetaFile(const strSlice &fullPath,
                                   const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be empty");
      SDB_ASSERT(FILE_TYPE_DM == fn.getType(), "must be dm");
      dataExtentIDMapFile *file = NULL;

      if (NULL != _meta)
      {
         PD_LOG(PDERROR, "meta file has already been open");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      file = SDB_OSS_NEW dataExtentIDMapFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         goto error;
      }

      rc = file->open(fullPath.str(), fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fullPath.str(), rc);
         goto error;
      }

      rc = file->cacheHead();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to cache head in mem:%d", rc);
         goto error;
      }

      _meta = file;
      file = NULL;
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 storageUnit::openDataFile(const strSlice &fullPath,
                                   const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be empty");
      SDB_ASSERT(FILE_TYPE_DD == fn.getType(), "must be dm");

      dataExtentFile *file = NULL;
      if (_data.size() <= fn.getSequence())
      {
         _data.resize(fn.getSequence() + 1);
      }
      else if (NULL != _data.at(fn.getSequence()))
      {
         PD_LOG(PDERROR, "duplicated data file:%s", fullPath.str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      file = SDB_OSS_NEW dataExtentFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->open(fullPath.str(), fn);
      if (SDB_OK != rc)
      {
         /// null ptr leaved in _data. it will be recreate if necessary.
         PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fullPath.str(), rc);
         goto error;
      }

      _data[fn.getSequence()] = file;
      file = NULL;

   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 storageUnit::openIdxMFile(const strSlice &fullPath,
                                   const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be empty");
      SDB_ASSERT(FILE_TYPE_IDX_M == fn.getType(), "must be dm");
      idxIDMapFile *file = NULL;
      _INDEX_META_LIST::iterator itr = _idxMetaList.begin();

      file = SDB_OSS_NEW idxIDMapFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->open(fullPath.str(), fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fullPath.str(), rc);
         goto error;
      }

      for (; itr != _idxMetaList.end(); ++itr)
      {
         const storageFileHead &head = file->getCommonHeadInMem();
         const storageFileHead &itrHead = (*itr)->getCommonHeadInMem();
         if (head.sequence <= itrHead.sequence)
         {
            break;
         }
      }
      _idxMetaList.insert(itr, file);
      file = NULL;
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 storageUnit::openIdxDFile(const strSlice &fullPath,
                                   const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be empty");
      SDB_ASSERT(FILE_TYPE_IDX_D == fn.getType(), "must be dm");
      idxDataFile *file = NULL;

      if (_idxDataVec.size() <= fn.getSequence())
      {
         _idxDataVec.resize(fn.getSequence(), NULL);
      }
      else if (NULL != _idxDataVec.at(fn.getSequence()))
      {
         PD_LOG(PDERROR, "duplicated idx data file:%s", fullPath.str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      file = SDB_OSS_NEW idxDataFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->open(fullPath.str(), fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fullPath.str(), rc);
         goto error;
      }

      _idxDataVec[fn.getSequence()] = file;
      file = NULL;

   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 storageUnit::openDeltaFile(const strSlice &fullPath,
                                    const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be empty");
      SDB_ASSERT(FILE_TYPE_DELTA == fn.getType(), "must be delta");
      deltaLogFile *file = NULL;
      _DELTA_LIST::iterator itr = _delta.begin();

      file = SDB_OSS_NEW deltaLogFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->open(fullPath.str(), fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fullPath.str(), rc);
         goto error;
      }

      for (; itr != _delta.end(); ++itr)
      {
         const storageFileHead &head = file->getCommonHeadInMem();
         const storageFileHead &itrHead = (*itr)->getCommonHeadInMem();
         if (head.sequence <= itrHead.sequence)
         {
            break;
         }
      }
      _delta.insert(itr, file);
      file = NULL;
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 storageUnit::openFSMFile(const strSlice &fullPath,
                                  const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be empty");
      SDB_ASSERT(FILE_TYPE_FSM == fn.getType(), "must be delta");
      fsmFile *file = NULL;

      if (NULL != _fsm)
      {
         PD_LOG(PDERROR, "duplicated fms file");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      file = SDB_OSS_NEW fsmFile();
      if (NULL == file)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = file->open(fullPath.str(), fn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fullPath.str(), rc);
         goto error;
      }

      _fsm = file;
      file = NULL;
   done:
      return rc;
   error:
      if (NULL != file)
      {
         file->close();
         SDB_OSS_DEL file;
      }
      goto done;
   }

   INT32 storageUnit::openOtherFilesUnderPath(const CHAR *path,
                                              const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != path && !dirName.empty(), "can not be null");
      CHAR dirToScan[OSS_MAX_PATHSIZE + 1] = {0};
      SPACE_ID dirSpaceID = INVALID_SPACE_ID;
      if (!vesselFileName::parseDirName(dirName, &dirSpaceID))
      {
         PD_LOG(PDERROR, "failed to parse dirname[%s]", dirName.str());
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = utilBuildFullPath(path, dirName.str(), OSS_MAX_PATHSIZE + 1, dirToScan);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", path, dirName.str(), rc);
         goto error;
      }

      {
      fs::directory_iterator end_iter ;
      fs::path dataDir(dirToScan);
      if (!fs::exists(dataDir) || !fs::is_directory(dataDir))
      {
         PD_LOG(PDERROR, "invalid data path:%s", dirToScan);
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string fileName = dir_iter->path().filename().string();
         std::string fullPath = dir_iter->path().string();
         PD_LOG(PDDEBUG, "found file:[%s]", fileName.c_str());
         strSlice fullPathSlice(fullPath.c_str(), fullPath.size());
         vesselFileName fn;

         if (!fs::is_regular_file(dir_iter->status()))
         {
            PD_LOG(PDERROR, "not regular", fileName.c_str());
            continue;
         }

         if (!fn.extract(strSlice(fileName.c_str(), fileName.size()), dirSpaceID))
         {
            PD_LOG(PDWARNING, "invalid file name:%s", fileName.c_str());
            continue;
         }

         if (fn.getType() == FILE_TYPE_DM)
         {
            continue;
         }

         rc = openFile(fullPathSlice, fn);
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

   INT32 storageUnit::openFile(const strSlice &fullPath,
                               const vesselFileName &fn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!fullPath.empty(), "can not be null");
      SDB_ASSERT(fn.isValid(), "can not be invalid");

      switch (fn.getType())
      {
      case FILE_TYPE_DM:
         rc = openMetaFile(fullPath, fn);
         break;
      case FILE_TYPE_DD:
         rc = openDataFile(fullPath, fn);
         break;
      case FILE_TYPE_IDX_M:
         rc = openIdxMFile(fullPath, fn);
         break;
      case FILE_TYPE_IDX_D:
         rc = openIdxDFile(fullPath, fn);
         break;
      case FILE_TYPE_FSM:
         rc = openFSMFile(fullPath, fn);
         break;
      case FILE_TYPE_CS_NAME:
         break;
      case FILE_TYPE_CONTROL:
         break;
      case FILE_TYPE_DELTA:
         rc = openDeltaFile(fullPath, fn);
         break;
      default:
         PD_LOG(PDERROR, "unknown file type:%d", fn.getType());
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file:%s, rc:%d", fn.getName(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 storageUnit::ensureCSNameFile(requestContext *context,
                                       const strSlice &csName)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      CHAR nameBuf[DMS_COLLECTION_SPACE_NAME_SZ+1] = {0};
      vesselFileName fn;
      OSSFILE file;
      strSlice pathSlice;
      strSlice dirNameSlice;

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

      fn.build(getSpaceID(), FILE_TYPE_CS_NAME);
      pathSlice.reset(context->getEnv()->options.path.dataPath.c_str(),
                      context->getEnv()->options.path.dataPath.size());
      dirNameSlice.reset(_dirName);
      rc = buildFileFullPath(pathSlice, dirNameSlice, fn,
                             OSS_MAX_PATHSIZE + 1, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build file path:%d", rc);
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

      ossMemcpy(nameBuf, csName.str(), csName.strLen());
      nameBuf[csName.strLen()] = '\n';

      rc = ossWriteN(&file, nameBuf, csName.strLen() + 1);
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

   INT32 storageUnit::removeCSNameFile(const strSlice &dataPath)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      vesselFileName fn;
      strSlice dirSlice(_dirName);
      OSSFILE file;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (dataPath.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!fn.build(getSpaceID(), FILE_TYPE_CS_NAME, 0))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      rc = buildFileFullPath(dataPath, dirSlice, fn,
                             OSS_MAX_PATHSIZE + 1, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build file path:%d", rc);
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

   INT32 storageUnit::ensureFsmFile(requestContext *context, fsmFile **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      vesselFileName fn;
      storageFileOptions options;
      storageCoreArgs args(FSM_PAGE_SIZE,
                           FSM_PAGE_COUNT_PER_SEG,
                           FSM_MAX_SEG_COUNT);
      fsmFile *file = NULL;
      CHAR *fullDirPathBuffer = NULL;
      UINT32 fullDirPathBufferSize = 0;

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

      if (NULL != _fsm && _fsm->isReadyToWork())
      {
         if (NULL != out)
         {
            *out = _fsm;
         }
         goto done;
      }

      {
      ossScopedLock guard(&_fsmLatch);
      if (NULL != _fsm && _fsm->isReadyToWork())
      {
         if (NULL != out)
         {
            *out = _fsm;
         }
         goto done;
      }

      if (NULL == _fsm)
      {
         if (OSS_UNLIKELY(!fn.build(getSpaceID(), FILE_TYPE_FSM)))
         {
            PD_LOG(PDERROR, "failed to build fsm file name");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         fullDirPathBufferSize = context->getEnv()->options.path.dataPath.size() +
                                 MAX_SPACE_DIR_LEN + 8;
         fullDirPathBuffer = context->allocateBuffer(fullDirPathBufferSize);
         if (NULL == fullDirPathBuffer)
         {
            PD_LOG(PDERROR, "failed to allocate mem");
            rc = SDB_OOM;
            goto error;
         }

         rc = utilBuildFullPath(context->getEnv()->options.path.dataPath.c_str(),
                              _dirName, fullDirPathBufferSize, fullDirPathBuffer);
         if (SDB_OK != rc)
         {
            goto error;
         }

         options.dir = fullDirPathBuffer;
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
      }

      rc = _fsm->initToWork(&_fsmLatch,
                            context->getEnv()->options.extendFileWithSparse);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init fsm file:%d", rc);
         goto error;
      }

      if (NULL != out)
      {
         *out = _fsm;
      }
      }

   done:
      if (NULL != fullDirPathBuffer)
      {
         context->releaseBuffer(fullDirPathBuffer, fullDirPathBufferSize);
      }
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

   INT32 storageUnit::buildFileFullPath(const strSlice &path,
                                        const strSlice &dir,
                                        const vesselFileName &fn,
                                        UINT32 bufferSize,
                                        CHAR *buffer)
   {
      SDB_ASSERT(!path.empty() && !dir.empty() && fn.isValid(), "can not be invalid");
      SDB_ASSERT(NULL != buffer, "can not be null");
      static const UINT32 _TMP_BUF_SIZE = MAX_SPACE_DIR_LEN + MAX_FILE_NAME_LEN + 4;
      CHAR tmp[_TMP_BUF_SIZE] = {0};
      INT32 rc = SDB_OK;
      if (MAX_SPACE_DIR_LEN < dir.strLen())
      {
         rc = SDB_INVALIDARG;
         goto done;
      }

      rc = utilBuildFullPath(dir.str(), fn.getName(), _TMP_BUF_SIZE, tmp);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%d", rc);
         goto error;
      }

      rc = utilBuildFullPath(path.str(), tmp, bufferSize, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:   
      goto done;
   }
}//namespace vessel
}//namespace engine