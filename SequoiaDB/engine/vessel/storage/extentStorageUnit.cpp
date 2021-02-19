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

   Source File Name = extentStorageUnit.cpp

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

#include "vessel/extentStorageUnit.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/storageFileName.h"
#include "vessel/dataExtentIDMapFile.h"
#include "vessel/dataExtentFile.h"
#include "utilStr.hpp"
#include "vessel/storageFileUtil.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/smpAccessor.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/csgpAccessor.h"
#include "vessel/impAccessor.h"
#include "vessel/idMapPage.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/lpidContext.h"


#include <boost/filesystem.hpp>

namespace fs = boost::filesystem ;

namespace engine
{
namespace vessel
{
   
   extentStorageUnit::extentStorageUnit()
   :_status(CLOSED),
   _idMapCapacity(0),
   _capacityOfCLRecordPage(0),
    _meta(NULL),
    _idxMeta(NULL)
   {
      ossMemset(_dirName, 0, sizeof(_dirName));
   }

   extentStorageUnit::~extentStorageUnit()
   {
      close();
   }
   

   extentStorageUnit::extentStorageUnit(const extentStorageUnit &o)
   {
      SDB_ASSERT(FALSE, "impossible");
   }

   extentStorageUnit &extentStorageUnit::operator=(const extentStorageUnit &o)
   {
      SDB_ASSERT(FALSE, "impossible");
      return *this;
   }

   INT32 extentStorageUnit::getCoreArgs(SPACE_TYPE type,
                                       UINT32 *pageSize,
                                       UINT32 *maxPageCountPerSeg,
                                       UINT32 *maxSegCountPerFile)
   {
      INT32 rc = SDB_OK;
      const dataIDMapFileHead *head = NULL;
      if (OSS_UNLIKELY(NULL == _meta))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      SDB_ASSERT(NULL != _meta, "can not be null");
      head = &(_meta->getHeadCache());

      if (SPACE_TYPE_RECORD_M == type)
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
      else if (SPACE_TYPE_RECORD_D == type)
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
      else if (SPACE_TYPE_IDX_D == type)
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

   INT32 extentStorageUnit::open(requestContext *context,
                                 const strSlice &dirName)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      BOOLEAN crashedImpossible = FALSE;
      const storagePathOptions *path = NULL;
      BOOLEAN rollback = FALSE;

      if (!closed())
      {
         PD_LOG(PDERROR, "su must be closed before opening");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;

      if (OSS_UNLIKELY(NULL == context || dirName.empty()))
      {
         PD_LOG(PDERROR, "prt is null");
         rc = SDB_INVALIDARG;
         goto error;
      }

      path = &(context->getEnv()->options.path);

      if (OSS_UNLIKELY(MAX_SU_DIR_LEN < dirName.strLen()))
      {
         PD_LOG(PDERROR, "name is too long:%s", dirName.str());
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossMemcpy(_dirName, dirName.str(), dirName.strLen());

      if (OSS_UNLIKELY(!parseStorageUnitDir(dirName, &sid)))
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

      if (path->indexPath != path->dataPath)
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

      getCapacityOfIMP(_meta->getCommonHeadInMem().pageSize, _idMapCapacity);
      getCapacityOfCLRecordPage(_meta->getCommonHeadInMem().pageSize, _capacityOfCLRecordPage);
      _status = OPEN;
   done:
      return rc;
   error:
      if (rollback)
      {
         close();
      }
      goto done;
   }

   INT32 extentStorageUnit::create(requestContext *context,
                                   const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SPACE_ID space = options.sid;
      BOOLEAN rollbackDir = FALSE;
      const strSlice &csNameSlice = options.csName;
      BOOLEAN rollback = FALSE;
      const storagePathOptions *path = NULL;

      if (OSS_UNLIKELY(!closed()))
      {
         PD_LOG(PDERROR, "can not be open");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;

      if (OSS_UNLIKELY(NULL == context))
      {
         PD_LOG(PDERROR, "invalid ptr");
         rc = SDB_INVALIDARG;
         goto error;
      }

      path = &(context->getEnv()->options.path);
      
      if (OSS_UNLIKELY(csNameSlice.empty() ||
                       DMS_COLLECTION_SPACE_NAME_SZ < csNameSlice.strLen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(INVALID_SPACE_ID == space ||
                       MAX_SPACE_ID < space))
      {
         PD_LOG(PDERROR, "invalid space id");
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossSnprintf(_dirName, MAX_SU_DIR_LEN + 1, "%s%d", SU_FILE_NAME_PREFIX, space);

      rc = testAllDirsBeforeCreating(*path, _dirName);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = createAllDirs(*path, _dirName);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rollbackDir = TRUE;

      rc = createNecessaryFiles(context, options, path->dataPath.c_str());
      if (SDB_OK != rc)
      {
         goto error;
      }

      getCapacityOfIMP(_meta->getCommonHeadInMem().pageSize, _idMapCapacity);
      getCapacityOfCLRecordPage(_meta->getCommonHeadInMem().pageSize, _capacityOfCLRecordPage);
      rc = initInMemBitMapWhenCreating();
      if (SDB_OK != rc)
      {
         goto error;
      }
      _status = OPEN;

   done:
      return rc;
   error:
      if (rollbackDir)
      {
         PD_LOG(PDWARNING, "rollback all dir created:%s", _dirName);
         removeAllDirs(*path, _dirName, FALSE);
      }
      if (rollback)
      {
         close();
      }
      goto done;
   }

   SPACE_ID extentStorageUnit::getSpaceID()const
   {
      if (OSS_LIKELY(!closed()))
      {
         SDB_ASSERT(NULL != _meta, "can not be null");
         return _meta->getCommonHeadInMem().spaceID;
      }
      return INVALID_SPACE_ID;
   }

   INT32 extentStorageUnit::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      CHAR dirName[MAX_SU_DIR_LEN + 1] = {0};
      const storagePathOptions &path = context->getEnv()->options.path;

      if (closed())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ossStrcpy(dirName, _dirName);
   
      close();

      rc = removeAllDirs(path, dirName, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
   
   INT32 extentStorageUnit::close(requestContext *context)
   {
      INT32 rc = SDB_OK;
      if (CLOSED == _status)
      {
         goto done;
      }

      saveBitMapWhenClosing(context);

      close();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::close()
   {
      INT32 rc = SDB_OK;
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

      _idMapCapacity = 0;
      _capacityOfCLRecordPage = 0;
      ossMemset(_dirName, 0, sizeof(_dirName));
      _inMemDataSMP.teardown();
      _status = CLOSED;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::extendMetaFile(requestContext *context,
                                           PAGE_ID *newPidInFile)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(closed()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(context->getSpaceID() != getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(0 < _meta->getSegmentCount(),
                "can not extend meta file unless creating done");
      rc = _meta->allocateNewSegment(newPidInFile);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }



   INT32 extentStorageUnit::getDataFile(UINT32 fileSequence, dataExtentFile **file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != file, "can not be null");
      SDB_ASSERT(!closed(), "can not be closed");
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

   INT32 extentStorageUnit::getPagePtr(SPACE_TYPE type, PAGE_ID page, ossValuePtr &ptr)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_SPACE_TYPE != type, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != page, "can not be invalid");
      SDB_ASSERT(!closed(), "can not be closed");

      if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                       INVALID_PAGE_ID == page ||
                       closed()))
      {
         PD_LOG(PDERROR, "invalid arg or su closed");
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (SPACE_TYPE_RECORD_M == type)
      {
         rc = _meta->getExtentPtr(page, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (SPACE_TYPE_RECORD_D == type)
      {
         const storageCoreArgs &args = _meta->getHeadCache().data;
         UINT32 fileSequence = page / (args.maxPageCountPerSeg * args.maxSegmentCountPerFile);
         PAGE_ID pageInFile = page % (args.maxPageCountPerSeg * args.maxSegmentCountPerFile);
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

         rc = file->getExtentPtr(pageInFile, ptr);
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

   INT32 extentStorageUnit::fsync(SPACE_TYPE type, PAGE_ID pid, UINT32 count, BOOLEAN sync)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(closed()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (SPACE_TYPE_RECORD_M == type)
      {
         rc = _meta->fsync(pid, count, sync);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }
      else if (SPACE_TYPE_RECORD_D == type)
      {

      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::getLpidOfClRecord(CL_MB_ID mbID, PAGE_ID &lpid)const
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(closed()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(0 < _capacityOfCLRecordPage, "can not be zero");
      lpid = mbID / _capacityOfCLRecordPage;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::getDataPhysicalPid(requestContext *context,
                                               PAGE_ID lpid,
                                               PAGE_ID &ppid,
                                               SNAPSHOT_ID &snap)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      impAccessor imp;

      if (OSS_UNLIKELY(closed()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      pid = getDataIMPPid(lpid);
      
      rc = imp.setup(context,
                     SPACE_TYPE_RECORD_M,
                     pid,
                     PAGE_ACCESSOR_FLAG_NONE,
                     this);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.getPid(lpid, &ppid, &snap);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (INVALID_PAGE_ID == ppid)
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      SDB_ASSERT(INVALID_SNAPSHOT_ID != snap, "can not be invalid");

   done:
      imp.teardown();
      return rc;
   error:
      ppid = INVALID_PAGE_ID;
      snap = INVALID_SNAPSHOT_ID;
      goto done;
   }

   INT32 extentStorageUnit::getDataSMPPId(PAGE_ID pid, PAGE_ID &smpID)
   {
      INT32 rc = SDB_OK;
      UINT32 maxPageCountInFile = 0;
      if (OSS_UNLIKELY(closed()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      maxPageCountInFile = _meta->getHeadCache().data.getMaxPageCountInFile();
      smpID = pid / maxPageCountInFile;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::preallocateDataPages(requestContext *context,
                                                 UINT32 count,
                                                 PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      UINT32 pageCount = _inMemDataSMP.getPageCount();
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(0 == count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         rc = _inMemDataSMP.allocateBits(count, pids);
         if (SDB_OK == rc)
         {
            /// pids already sorted.
            PAGE_ID pid = pids[count - 1];
            PAGE_ID pidInFile = pid % _meta->getHeadCache().data.getMaxPageCountInFile();
            UINT32 sequence = pid / _meta->getHeadCache().data.getMaxPageCountInFile();
            rc = ensureDataFileSpace(context, sequence, pidInFile);
            if (SDB_OK != rc)
            {
               _inMemDataSMP.releaseBits(count, pids);
               goto error;
            }
            goto done;
         }
         else if (SDB_VESSEL_SMP_NO_FREE != rc)
         {
            PD_LOG(PDERROR, "failed to allocate page from bitmap:%d", rc);
            goto error;
         }
         else
         {
            _extendingMetaAndDataLatch.get();
            locked = TRUE;
            /// some one extended space before we get latch.
            if (pageCount < _inMemDataSMP.getPageCount())
            {
               pageCount = _inMemDataSMP.getPageCount();
               _extendingMetaAndDataLatch.release();
               locked = FALSE;
               continue;
            }
            else
            {
               rc = createDataFile(context, NULL);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to create new data file:%d", rc);
                  goto error;
               }

               /// 1 page occupied for smp.
               rc = _inMemDataSMP.allocateNewBitPage(1);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to extend in-mem bitmap:%d", rc);
                  removeLastDataFile(context);
                  goto error;
               }

               _extendingMetaAndDataLatch.release();
               locked = FALSE;
               continue;
            }
         }
         
      } while (TRUE);
      
   done:
      if (locked)
      {
         _extendingMetaAndDataLatch.release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::releaseDataPagesPreallocated(requestContext *context,
                                                         UINT32 count,
                                                         const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      rc = _inMemDataSMP.releaseBits(count, pids);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::remapLpid(requestContext *context,
                                      PAGE_ID lpid,
                                      PAGE_ID pid,
                                      const DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!closed(), "can not be closed");
      SDB_ASSERT(INVALID_PAGE_ID != lpid && INVALID_PAGE_ID != pid, "can not be invalid");
      PAGE_ID impPid = INVALID_PAGE_ID;
      snapshotContainer &container = context->getEnv()->snapContainer;
      impAccessor imp;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid || INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->testLpidLockMode(SPACE_TYPE_RECORD_D, lpid, EXCLUSIVE)))
      {
         PD_LOG(PDERROR, "should hold exclusive lock");
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      impPid = getDataIMPPid(lpid);
      if (OSS_UNLIKELY(INVALID_PAGE_ID == impPid))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = imp.setup(context, SPACE_TYPE_RECORD_M, impPid,
                     PAGE_ACCESSOR_FLAG_NON_READONLY, this);
      if (SDB_OK != rc)
      {
         goto error;
      } 

      rc = imp.remap(lpid, pid, container.getOnlineID(), oplist);
      if (SDB_OK != rc)
      {
         goto error;
      }

      imp.teardown();
   done:
      return rc;
   error:
      imp.teardown();
      goto done;
   }

   INT32 extentStorageUnit::removeSUNameFile(const CHAR *dir,
                                             SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      storageFileName fn;

      rc = utilBuildFullPath(dir, fn.getName(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossDelete(fullPath);
      if (SDB_OK == rc)
      {
         goto done;
      }
      else if (SDB_FNE != rc)
      {
         rc = SDB_OK;
         goto done;
      }
      else
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::createSUNameFile(const CHAR *dir,
                                             SPACE_ID sid,
                                             const strSlice &csName)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      SDB_ASSERT(NULL != dir &&
                 INVALID_SPACE_ID != sid, "can not be null");
      SDB_ASSERT(!csName.empty() && csName.strLen() <= DMS_COLLECTION_SPACE_NAME_SZ, "can not be invalid");
                 ;
      storageFileName fn;
      OSSFILE file;
      CHAR buf[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {0};

      fn.build(SPACE_TYPE_NAME, sid, 0);
      rc = utilBuildFullPath(dir, fn.getName(), OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ossOpen(fullPath,
                  OSS_CREATEONLY | OSS_READWRITE | OSS_EXCLUSIVE,
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
      if (file.isOpened())
      {
         ossClose(file);
      }
      return rc;
   error:
      if (file.isOpened())
      {
         ossClose(file);
         ossDelete(fullPath);
      }
      goto done;
   }

   INT32 extentStorageUnit::firstExtendDataFile(requestContext *context,
                                                dataExtentFile *file)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != file, "can not be null");
      SDB_ASSERT(file->isOpen(), "must be open");
      SDB_ASSERT(0 == file->getSegmentCount(), "must be zero");
      ossValuePtr ptr = 0;
      smpAccessor smp;
      PAGE_ID smpPid = INVALID_PAGE_ID;
      const storageFileHead &head = file->getCommonHeadInMem();
      UINT32 sequence = head.sequence;


      rc = file->allocateNewSegment(NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extend file:%d", rc);
         goto error;
      }

      rc = file->getPagePtr(SMP_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      smpPid = sequence * head.maxPageCountPerSeg * head.maxSegmentCountPerFile;

      rc = smp.setup(context, SPACE_TYPE_RECORD_D,
                     smpPid, head.pageSize, ptr,
                     FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.initSMP(head.maxSegmentCountPerFile,
                       head.maxPageCountPerSeg, 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init smp of data:%d, data seq:%d", rc, sequence);
         goto error;
      }

      smp.teardown();

      rc = file->fsync(SMP_PAGE_ID, 1, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::createDataFile(requestContext *context,
                                           UINT32 *sequenceOfNewFile)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(0 != ossStrlen(_dirName), "can not be null");
      SDB_ASSERT(NULL != _meta, "can not be null");
      SDB_ASSERT(!closed(), "can not be closed");
      storageFileName fn;
      storageFileOptions options;
      dataExtentFile *tmpPtr = NULL;
      const storageFileHead &commonHead = _meta->getCommonHeadInMem();
      const dataIDMapFileHead &metaHead = _meta->getHeadCache();
      CHAR fullPath[OSS_MAX_PATHSIZE+1] = {0};
      UINT32 sequence = 0;

      /// createDataFile should always be protected by extending latch.
      /// no one can modify _data.size() now.
      sequence = _data.size();

      rc = utilBuildFullPath(context->getEnv()->options.path.dataPath.c_str(),
                             _dirName, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path:%d", rc);
         goto error;
      }

      fn.build(SPACE_TYPE_RECORD_D, commonHead.spaceID, sequence);

      options.dir = fullPath;
      options.name = fn.getName();
      options.secretValue = commonHead.secretValue;
      options.logicalCS = commonHead.logicalCSID;
      options.spaceID = commonHead.spaceID;
      options.args = &(metaHead.data);
      options.sequence = sequence;

      tmpPtr = SDB_OSS_NEW dataExtentFile();
      if (NULL == tmpPtr)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = tmpPtr->create(options);
      if (SDB_OK != rc)
      {
         LOG_ERR_AND_REPORT(context, rc, "failed to create data file:%s, rc:%d", fn.getName(), rc);
         goto error;
      }

      rc = firstExtendDataFile(context, tmpPtr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _dataFileAccessingMutex.get();
      _data.push_back(tmpPtr);
      _dataFileAccessingMutex.release();

      /// from here, do not goto error.
      /// or you must add code to rollback data sequence.

      if (NULL != sequenceOfNewFile)
      {
         *sequenceOfNewFile = sequence;
      }
      
   done:
      return rc;
   error:
      if (NULL != tmpPtr)
      {
         tmpPtr->destroy();
         SDB_OSS_DEL tmpPtr;
      }
      goto done;
   }

   INT32 extentStorageUnit::initInMemBitMapWhenCreating()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _meta, "can not be null");
      const dataIDMapFileHead  &head = _meta->getHeadCache();
      UINT32 dataPageSize = head.data.pageSize;
      UINT32 maxSegmentCount = head.data.maxSegmentCountPerFile;
      UINT32 pageCount = head.data.maxPageCountPerSeg;
      UINT32 bitcount = 0;

      rc = getSMPCapacity(dataPageSize, maxSegmentCount, pageCount, bitcount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _inMemDataSMP.setup(bitcount, 8);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::saveBitMapWhenClosing(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      const storagePathOptions &path = context->getEnv()->options.path;
      CHAR subPath[SU_FILE_NAME_LEN+MAX_SU_DIR_LEN+2] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      storageFileName fn;

      if (_inMemDataSMP.isSetup())
      {
         fn.build(SPACE_TYPE_SPACE_MAP, getSpaceID(), 0);
         rc = utilBuildFullPath(_dirName, fn.getName(), SU_FILE_NAME_LEN+MAX_SU_DIR_LEN+1, subPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", _dirName, fn.getName(), rc);
            goto error;
         }

         rc = utilBuildFullPath(path.dataPath.c_str(), subPath, OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", path.dataPath.c_str(), subPath, rc);
            goto error;
         }

         _inMemDataSMP.dumpToFile(context, fullPath);
      }
   
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::initInMemBitMapWhenOpening(requestContext *context,
                                                       BOOLEAN rebuild)
   {
      INT32 rc = SDB_OK;
      const storagePathOptions &path = context->getEnv()->options.path;
      CHAR subPath[SU_FILE_NAME_LEN+MAX_SU_DIR_LEN+2] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      storageFileName fn;
      SDB_ASSERT(!rebuild, "todo");

      fn.build(SPACE_TYPE_SPACE_MAP, getSpaceID(), 0);
      rc = utilBuildFullPath(_dirName, fn.getName(), SU_FILE_NAME_LEN+MAX_SU_DIR_LEN+1, subPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", _dirName, fn.getName(), rc);
         goto error;
      }
      
      rc = utilBuildFullPath(path.dataPath.c_str(), subPath, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build dir full path:%s, %s, %d", path.dataPath.c_str(), subPath, rc);
         goto error;
      }

      rc = _inMemDataSMP.loadFromFile(context, fullPath);
      if (SDB_VESSEL_INVALID_VESSEL_FILE == rc)
      {
         ///need to rebuild from disk.
         SDB_ASSERT(FALSE, "todo");
      }
      else if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }


   INT32 extentStorageUnit::ensureDataFileSpace(requestContext *context,
                                                UINT32 sequence,
                                                PAGE_ID pidInFile)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_PAGE_ID != pidInFile, "can not be invalid");
      dataExtentFile *file = NULL;
      UINT32 pageCountPerSeg = 0;
      UINT32 currentSegCount = 0;
      UINT32 expectedSegCount = 0;
      BOOLEAN locked = FALSE;

      rc = getDataFile(sequence, &file);
      if (SDB_OK != rc)
      {
         goto error;
      }

      pageCountPerSeg = file->getCommonHeadInMem().maxPageCountPerSeg;
      expectedSegCount = pidInFile / pageCountPerSeg + 1;
      currentSegCount = file->getSegmentCount();
      SDB_ASSERT(0 < currentSegCount, "first segment should be created when file creating");
      if (expectedSegCount <= currentSegCount)
      {
         goto done;
      }

      _extendingMetaAndDataLatch.get();
      locked = TRUE;
      /// check again.
      currentSegCount = file->getSegmentCount();
      if (expectedSegCount <= currentSegCount)
      {
         goto done;
      }

      for (UINT32 i = currentSegCount; i <= expectedSegCount; ++i)
      {
         rc = file->allocateNewSegment(NULL);
         if (SDB_OK != rc)
         {
            LOG_ERR_AND_REPORT(context, rc, "failed to extend data file:%d", rc);
            goto error;
         }
      }

   done:
      if (locked)
      {
         _extendingMetaAndDataLatch.release();
      }
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::removeLastDataFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      dataExtentFile *file = NULL;
      /// removeLastDataFile should always be protected by extending latch.
      /// no one can modify _data.size() now.
      UINT32 sequence = _data.size();
      if (0 == sequence)
      {
         goto done;
      }

      rc = getDataFile(sequence, &file);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// no one should access this file now coz we 
      rc = file->destroy();
      if (SDB_OK != rc)
      {
         ossPanic();
         PD_LOG(PDERROR, "failed to remove data file when rollback creating:%d", rc);
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

   INT32 extentStorageUnit::firstExtendMetaFile(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _meta, "can not be null");
      SDB_ASSERT(0 == _meta->getSegmentCount(), "must be zero");
      PAGE_ID pid = INVALID_PAGE_ID;
      const storageFileHead &head = _meta->getCommonHeadInMem();
      UINT32 maxSegmentCount = head.maxSegmentCountPerFile;
      UINT32 maxPageCount = head.maxPageCountPerSeg;
      UINT32 pageSize = head.pageSize;
      ossValuePtr ptr = 0;
      smpAccessor smp;

      getCoreArgs(SPACE_TYPE_RECORD_M, &pageSize, &maxPageCount, &maxSegmentCount);
      

      rc = _meta->allocateNewSegment(&pid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _meta->getPagePtr(SMP_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = smp.setup(context,
                     SPACE_TYPE_RECORD_M,
                     pid, pageSize, ptr,
                     FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// first 3 pages are occupied by system pages.
      rc = smp.initSMP(maxSegmentCount, maxPageCount, 3);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      smp.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::createMetaFile(requestContext *context,
                                           const CHAR *dir,
                                           const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != dir, "can not be null");
      SDB_ASSERT(NULL == _meta, "must be null");
      storageFileName fn;
      storageFileOptions fileOptions;
      UINT32 secretValue = ossRand();
      SPACE_ID sid = options.sid;
      

      SDB_ASSERT(NULL == _meta, "must be null");

      fn.build(SPACE_TYPE_RECORD_M, sid, 0);

      fileOptions.dir = dir;
      fileOptions.name = fn.getName();
      fileOptions.secretValue = secretValue;
      fileOptions.logicalCS = options.csOptions->logicalID;
      fileOptions.spaceID = sid;
      fileOptions.args = &(options.metaArgs);
      fileOptions.sequence = 0;

      _meta = SDB_OSS_NEW dataExtentIDMapFile();
      if (NULL == _meta)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = _meta->create(fileOptions, &options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to crate meta file:%d", rc);
         goto error;
      }

      rc = initNecessaryPagesWhenCreating(context, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      if (NULL != _meta)
      {
         _meta->destroy();
      }
      SAFE_OSS_DELETE(_meta);
      goto done;
   }

   INT32 extentStorageUnit::initNecessaryPagesWhenCreating(requestContext *context,
                                                           const createSUOptions &options)
   {
      INT32 rc = SDB_OK;
      rc = firstExtendMetaFile(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initGMP(context, options.csName, *(options.csOptions));
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = initSystemIMP(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _meta->fsync(SMP_PAGE_ID, 3);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::initGMP(requestContext *context,
                                    const strSlice &csName,
                                    const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _meta, "can not be null");
      SDB_ASSERT(NULL != context, "can not be null");
      
      UINT32 pageSize = _meta->getCommonHeadInMem().pageSize;
      csgpAccessor csgp;
      csMetaRecord record;
      ossValuePtr ptr = 0;


      record.version = CMR_VERSION_1;
      record.setOnline();
      record.flags = options.flags;
      record.logicalID = options.logicalID;
      ossMemcpy(record.name, csName.str(), csName.strLen());

      rc = _meta->getPagePtr(CS_GLOBAL_META_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = csgp.setup(context,
                      SPACE_TYPE_RECORD_M,
                      CS_GLOBAL_META_PAGE_ID,
                      pageSize, ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = csgp.initPage(record);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      csgp.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::initSystemIMP(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != _meta, "can not be null");
      
      UINT32 pageSize = _meta->getCommonHeadInMem().pageSize;
      impAccessor imp;
      ossValuePtr ptr = 0;

      rc = _meta->getPagePtr(SYSTEM_MAP_PAGE_ID, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.setup(context,
                     SPACE_TYPE_RECORD_M,
                     SYSTEM_MAP_PAGE_ID,
                     pageSize, ptr, FALSE, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = imp.initPage(0);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      imp.teardown();
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::openMetaFile(const CHAR *storagePath,
                                         const strSlice &dirName,
                                         SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != storagePath && !dirName.empty(), "can not be null");
      CHAR subPath[SU_FILE_NAME_LEN+MAX_SU_DIR_LEN+2] = {0};
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      storageFileName fn;
      fn.build(SPACE_TYPE_RECORD_M, sid, 0);
      rc = utilBuildFullPath(dirName.str(), fn.getName(), SU_FILE_NAME_LEN+MAX_SU_DIR_LEN+1, subPath);
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
   done:
      return rc;
   error:
      goto done;
   }

   INT32 extentStorageUnit::openOtherFilesUnderPath(const CHAR *path,
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
         
         storageFileName fn;

         if (!fs::is_regular_file(dir_iter->status()))
         {
            PD_LOG(PDERROR, "not regular", fileName.c_str());
            continue;
         }

         INT32 r = fn.extract(fileName.c_str(), dirName.str());
         if (SDB_OK != r)
         {
            PD_LOG(PDWARNING, "invalid file name:%s", fileName.c_str());
            continue;
         }

         if (fn.getType() == SPACE_TYPE_RECORD_M)
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

   INT32 extentStorageUnit::openFile(const CHAR *fullPath,
                                     const storageFileName &fn)
   {
      INT32 rc = SDB_OK;
      extentStorageFile *ef = NULL;
      SDB_ASSERT(NULL != fullPath, "can not be null");
      SDB_ASSERT(fn.valid(), "can not be invalid");

      switch (fn.getType())
      {
      case SPACE_TYPE_RECORD_M:
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
         break;
      }
      case SPACE_TYPE_RECORD_D:
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
         break;
      }
      case SPACE_TYPE_NAME:
      {
         break;
      }
      case SPACE_TYPE_SPACE_MAP:
      {
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

   INT32 extentStorageUnit::createNecessaryFiles(requestContext *context,
                                                const createSUOptions &options,
                                                const CHAR *path)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(NULL != path, "can not be null");

      rc = utilBuildFullPath(path, _dirName, OSS_MAX_PATHSIZE, fullPath);
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

      rc = createSUNameFile(fullPath, options.sid, options.csName);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (NULL != _meta)
      {
         _meta->destroy();
      }
      goto done;
   }


   INT32 extentStorageUnit::crossCheckFilesWhenOpenning()
   {
      INT32 rc = SDB_OK;
      const storageFileHead *commonHead = NULL;
      const dataIDMapFileHead *metaHead = NULL;
      SDB_ASSERT(NULL != _meta, "can not be null");
      if (!closed())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      commonHead = &(_meta->getCommonHeadInMem());
      metaHead = &(_meta->getHeadCache());
      for (UINT32 i = 0; i < _data.size(); ++i)
      {
         dataExtentFile *df = _data.at(i);
         const storageFileHead *dataHead = &(df->getCommonHeadInMem());
         if (commonHead->spaceID != dataHead->spaceID)
         {
            PD_LOG(PDERROR, "space id is not same:%d, %d", commonHead->spaceID, dataHead->spaceID);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (commonHead->logicalCSID != dataHead->logicalCSID)
         {
            PD_LOG(PDERROR, "logical id is not same:%d, %d", commonHead->logicalCSID, dataHead->logicalCSID);
            rc = SDB_VESSEL_INVALID_VESSEL_FILE;
            goto error;
         }
         else if (commonHead->secretValue != dataHead->secretValue)
         {
            PD_LOG(PDERROR, "secret value is not same:%d, %d", commonHead->secretValue, dataHead->secretValue);
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

   INT32 extentStorageUnit::testDir(const CHAR *fullPath,
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

   INT32 extentStorageUnit::testAllDirsBeforeOpenning(const storagePathOptions &path,
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

      if (path.indexPath != path.dataPath)
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

      if (path.lobMetaPath != path.dataPath)
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

      if (path.lobPath != path.dataPath)
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

   INT32 extentStorageUnit::testAllDirsBeforeCreating(const storagePathOptions &path,
                                                      const CHAR *dirName)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != dirName, "can not be null");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      rc = utilBuildFullPath(path.dataPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName, rc);
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
      

      if (path.dataPath != path.indexPath)
      {
         rc = utilBuildFullPath(path.indexPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName, rc);
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

   INT32 extentStorageUnit::createAllDirs(const storagePathOptions &path,
                                          const CHAR *dirName)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != dirName, "can not be null");
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      BOOLEAN rollbackDataDir = FALSE;
      BOOLEAN rollbackIndexDir = FALSE;

      rc = utilBuildFullPath(path.dataPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName, rc);
         goto error;
      }

      rc = ossMkdir(fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create dir:%s, %d", fullPath, rc);
         goto error;
      }

      rollbackDataDir = TRUE;

      if (path.indexPath != path.dataPath)
      {
         rc = utilBuildFullPath(path.indexPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to build full path, space:%s, %d", dirName, rc);
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
         if (SDB_OK == utilBuildFullPath(path.indexPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath))
         {
            removeDir(fullPath, TRUE);
         }
      }

      if (rollbackDataDir)
      {
         if(SDB_OK == utilBuildFullPath(path.dataPath.c_str(), dirName, OSS_MAX_PATHSIZE, fullPath))
         {
            removeDir(fullPath, TRUE);
         }
      }
      goto done;
   }

   INT32 extentStorageUnit::removeDir(const CHAR *fullPath, BOOLEAN mustBeEmpty)
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

   INT32 extentStorageUnit::removeAllDirs(const storagePathOptions &path,
                                          const CHAR *dirName,
                                          BOOLEAN mustBeEmpty)
   {
      INT32 rc = SDB_OK;
      CHAR fullPath[OSS_MAX_PATHSIZE + 1] = {0};
      
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

      if (path.dataPath != path.indexPath)
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

   PAGE_ID extentStorageUnit::getDataIMPPid(PAGE_ID lpid)const
   {
      SDB_ASSERT(0 < _idMapCapacity, "impossible");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      if (OSS_LIKELY(0 < _idMapCapacity))
      {
         return (lpid / _idMapCapacity) + SYSTEM_MAP_PAGE_ID;
      }
      return INVALID_PAGE_ID;
   }
}//namespace vessel
}// namespace engine 