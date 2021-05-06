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

   Source File Name = mainDataSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/mainDataSpace.h"
#include "ossLikely.hpp"
#include "vessel/impAccessor.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageUnit.h"
#include "vessel/smpAccessor.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/csgpAccessor.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"

namespace engine
{
namespace vessel
{
   mainDataSpace::mainDataSpace()
   {}

   mainDataSpace::~mainDataSpace()
   {}

   BOOLEAN mainDataSpace::isOpen()const
   {
      return logicalPageSpace::isOpen();
   }

   INT32 mainDataSpace::create(requestContext *context,
                               storageUnit *su,
                               const strSlice &csName,
                               UINT32 logicalID,
                               const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      if (NULL == context ||
          NULL == su ||
          !su->isOpen() ||
          csName.empty() ||
          DMS_INVALID_LOGICCSID == logicalID)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = initNecessaryPagesWhenCreating(context, su, csName,
                                          options.uniqueID,
                                          logicalID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init necessary pages:%d", rc);
         goto error;
      }

      rc = logicalPageSpace::open(context, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open logical page space:%d", rc);
         goto error;
      }

      rc = updateStatusToOnlineWhenCreating(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update status on meta page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::getLpidOfCollectionRecord(CL_MB_ID mbID,
                                                  PAGE_ID &lpid)const
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = 0;
      UINT32 capacity = 0;
      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = logicalPageSpace::getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      rc = getCapacityOfCLRecordPage(pageSize, capacity);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      lpid = mbID / capacity;
   done:
      return rc;
   error:
      goto done;
   }

   PAGE_ID mainDataSpace::getSystemImpPid()const
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 count = 0;
      UINT32 pageSize = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      rc = logicalPageSpace::getMetaPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto done;
      }

      if (!getSMPCapacityOrCount(pageSize, NULL, &count))
      {
         goto done;
      }

      pid = SMP_PAGE_ID + count + getSystemPageCount();
   done:
      return pid;
   }

   INT32 mainDataSpace::readGlobalMetaData(requestContext *context,
                                           BOOLEAN cacheMode,            
                                           csMetaRecord &record)
   {
      INT32 rc = SDB_OK;
      csgpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = cacheMode;
      record.reset();
      PAGE_ID pid = INVALID_PAGE_ID;

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
   
      pid = getGlobalMetaPid();
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         PD_LOG(PDERROR, "failed to get pid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = accessor.initWithOptions(context,
                                    FILE_TYPE_DM,
                                    pid,
                                    o, getSU());
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = accessor.readMetaRecord(record);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read meta record:%d", rc);
         goto error;
      }
   done:
      accessor.fini(context);
      return rc;
   error:
      record.reset();
      goto done;
   }

   INT32 mainDataSpace::updateStatusToOnlineWhenCreating(requestContext *context)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      csgpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;
      dataIDMapFileHead h;

      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      pid = getGlobalMetaPid();
      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         PD_LOG(PDERROR, "failed to get pid");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      getSU()->dumpIDMapFileHead(h);
      rc = accessor.initWithOptions(context, FILE_TYPE_DM,
                                    pid, o, getSU());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init csgp accessor:%d", rc);
         goto error;
      }

      rc = accessor.setOnlineWhenCreating(context, h);
      if (SDB_OK != rc)
      {
         goto error;
      }

      accessor.fini(context);
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 mainDataSpace::allocatePages(requestContext *context,
                                      PAGE_TYPE pageType,
                                      UINT32 count,
                                      const PAGE_ID *lpids,
                                      const PAGE_ID *pids,
                                      const slice &args,
                                      DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      PAGE_ID smpPid = INVALID_PAGE_ID;
      PAGE_ID impPid = INVALID_PAGE_ID;
      BOOLEAN rollbackSMP = FALSE;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_TYPE == pageType ||
                       0 == count ||
                       NULL == lpids ||
                       NULL == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (OSS_UNLIKELY(INVALID_PAGE_ID == pids[0]))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      smpPid = getDataSmpPid(pids[0]);
      if (INVALID_PAGE_ID == smpPid)
      {
         PD_LOG(PDERROR, "failed to get smp pid of pid[%d]", pids[0]);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpids[0]))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      impPid = logicalPageSpace::getImpPidOfLpid(lpids[0]);
      if (INVALID_PAGE_ID == impPid)
      {
         PD_LOG(PDERROR, "failed to get imp pid of lpid[%d]", lpids[0]);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 1; i < count; ++i)
      {
         PAGE_ID nextIMPPid = INVALID_PAGE_ID;
         PAGE_ID nextSMPPid = INVALID_PAGE_ID;

         if (OSS_UNLIKELY(INVALID_PAGE_ID == pids[i] ||
                          INVALID_PAGE_ID == lpids[i]))
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         
         nextSMPPid = getDataSmpPid(pids[i]);
         if (INVALID_PAGE_ID == nextSMPPid)
         {
            PD_LOG(PDERROR, "failed to get smp pid of pid[%d]", pids[i]);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         if (smpPid != nextSMPPid)
         {
            PD_LOG(PDERROR, "allocating must be excuted on same smp[%d,%d]",
                   pids[0], pids[i]);
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
         nextIMPPid = logicalPageSpace::getImpPidOfLpid(lpids[i]);
         if (INVALID_PAGE_ID == impPid)
         {
            PD_LOG(PDERROR, "failed to get imp pid of lpid[%d]", lpids[i]);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         if (impPid != nextIMPPid)
         {
            PD_LOG(PDERROR, "allocating must be excuted on same imp[%d,%d]",
                   lpids[0], lpids[i]);
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }

      rc = allocateNewDataPagesOnSMP(context, smpPid, pageType,
                                     count, lpids, pids, args,
                                     &lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate pages on smp:%d", rc);
         goto error;
      }
      rollbackSMP = TRUE;

      rc = mapNewPagesToIdMap(context, impPid, count, lpids,
                              pids, lsn, NULL == oplist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to map pages to idmap:%d", rc);
         goto error;
      }

      if (NULL != oplist)
      {
         *oplist = lsn;
      }

   done:
      return rc;
   error:
      if (rollbackSMP)
      {
         PD_LOG(PDWARNING, "begin to rollback allocating on smp[%d]", smpPid);
         if (SDB_OK != releaseDataPagesOnSMP(context, count, pids, lsn, TRUE))
         {
            PD_LOG(PDSEVERE, "failed to rollback allocating on smp[%d]:%d", smpPid, rc);
            IRedoLogger *logger = context->getOuterResource()->logger;
            logger->abortOplist(context->getSession(), lsn);
         }
      }
      goto done;
   }

   INT32 mainDataSpace::releasePages(requestContext *context,
                                     UINT32 count,
                                     const PAGE_ID *lpids,
                                     DPS_LSN_OFFSET oplist)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 mainDataSpace::allocateNewDataPagesOnSMP(requestContext *context,
                                                  PAGE_ID smpPid,
                                                  PAGE_TYPE type,
                                                  UINT32 count,
                                                  const PAGE_ID *lpids,
                                                  const PAGE_ID *pids,
                                                  const slice &args,
                                                  DPS_LSN_OFFSET *oplist)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != smpPid, "can not be invalid");
      SDB_ASSERT(0 != count, "can not be zero");
      SDB_ASSERT(NULL != lpids, "can not be null");
      SDB_ASSERT(NULL != pids, "can not be null");
      SDB_ASSERT(INVALID_PAGE_TYPE != type, "can not be invalid");
      smpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;
      o.oplistHead = (NULL != oplist);

      rc = accessor.initWithOptions(context, FILE_TYPE_DD,
                                    smpPid, o, getSU());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init smp[%d] accessor:%d", smpPid, rc);
         goto error;
      }

      rc = accessor.allocatePages(context, type, count,
                                  lpids, pids, args);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate pages on smp:%d", rc);
         goto error;
      }

      if (NULL != oplist)
      {
         *oplist = accessor.getOplist();
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != *oplist, "impossible");
      }
      accessor.fini(context);
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }

   INT32 mainDataSpace::releaseDataPagesOnSMP(requestContext *context,
                                              UINT32 count,
                                              const PAGE_ID *pids,
                                              DPS_LSN_OFFSET oplist,
                                              BOOLEAN oplistTail)
   {
      SDB_ASSERT(FALSE, "TODO");
      return SDB_OK;
   }

   INT32 mainDataSpace::mapNewPagesToIdMap(requestContext *context,
                                           PAGE_ID imp,
                                           UINT32 count,
                                           const PAGE_ID *lpids,
                                           const PAGE_ID *pids,
                                           DPS_LSN_OFFSET oplist,
                                           BOOLEAN oplistTail)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != imp, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(NULL != pids && NULL != lpids, "can not be null");
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(!oplistTail || DPS_INVALID_LSN_OFFSET != oplist, "impossible");

      impAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;
      o.oplistTail = oplistTail;
      const snapshotContainer &sc = context->getEnv()->snapContainer;
      
      rc = accessor.initWithOptions(context, FILE_TYPE_DM,
                                    imp, o, getSU(), oplist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init imp[%d] accessor:%d", imp, rc);
         goto error;
      }

      rc = accessor.map(context, count, lpids, pids, sc.getOnlineID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to map new pages to idmap:%d", rc);
         goto error;
      }
   done:
      accessor.fini(context);
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::getPhysicalPid(requestContext *context,
                                       PAGE_ID lpid,
                                       PAGE_ID &pid,
                                       SNAPSHOT_ID *snap)
   {
      INT32 rc = SDB_OK;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      impAccessor imp;
      PAGE_ID impPid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!context->testLpidLocked(FILE_TYPE_DD, lpid))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      impPid = getImpPidOfLpid(lpid);
      if (INVALID_PAGE_ID == impPid)
      {
         PD_LOG(PDERROR, "failed to get imp pid of lpid[%d]", lpid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = imp.initWithOptions(context, FILE_TYPE_DM,
                               impPid, o, getSU());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init imp accessor:%d", rc);
         goto error;
      }

      rc = imp.getPidByLpid(lpid, &pid, snap);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (INVALID_PAGE_ID == pid)
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }
   done:
      imp.fini(context);
      return rc;
   error:
      pid = INVALID_PAGE_ID;
      if (NULL != snap)
      {
         *snap = INVALID_SNAPSHOT_ID;
      }
      goto done;
   }

   INT32 mainDataSpace::getPhysicalPidToWrite(requestContext *context,
                                              PAGE_ID lpid,
                                              PAGE_ID &pid,
                                              SNAPSHOT_ID *snap)
   {
      INT32 rc = SDB_OK;
      SNAPSHOT_ID snapshot = INVALID_SNAPSHOT_ID;
      snapshotContainer *sc = NULL;
      PAGE_ID currentPid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!context->testLpidLockMode(FILE_TYPE_DD, lpid, EXCLUSIVE))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = getPhysicalPid(context, lpid, currentPid, &snapshot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      sc = &(context->getEnv()->snapContainer);
      if (sc->contains(snapshot, getSpaceID()))
      {
         rc = copyOnWirte(context, lpid, currentPid, pid, snap);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to copy on write page[%d], rc:%d", lpid, rc);
            goto error;
         }
      }

      pid = currentPid;
      if (NULL != snap)
      {
         *snap = snapshot;
      }
   done:
      return rc;
   error:
      pid = INVALID_PAGE_ID;
      if (NULL != snap)
      {
         *snap = INVALID_SNAPSHOT_ID;
      }
      goto done;
   }

   INT32 mainDataSpace::copyOnWirte(requestContext *context,
                                    PAGE_ID lpid,
                                    PAGE_ID oldPid,
                                    PAGE_ID &newPid,
                                    SNAPSHOT_ID *snap)
   {
      SDB_ASSERT(FALSE, "TODO");
      return 0;
   }

   INT32 mainDataSpace::allocateIdMapPagesOnDisk(requestContext *context,
                                                 PAGE_ID first,
                                                 UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(INVALID_PAGE_ID != first, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");
      PAGE_ID maxPid = first + count - 1;
      UINT32 pageCountInSeg = 0;
      UINT32 currentSegCount = 0;
      UINT32 targetSegCount = 0;
      UINT32 pageSize = 0;
      PAGE_ID smpPid = INVALID_PAGE_ID;
      
      if (OSS_UNLIKELY(INVALID_PAGE_ID == first))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getSU()->getCoreArgs(FILE_TYPE_DM, &pageSize, &pageCountInSeg);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      smpPid = logicalPageSpace::getSMPPIdOfImp(first);
      if (INVALID_PAGE_ID == smpPid)
      {
         PD_LOG(PDERROR, "failed to get smp pid of pid[%d]", first);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 1; i < count; ++i)
      {
         PAGE_ID nextSMP = logicalPageSpace::getSMPPIdOfImp(first + i);
         if (INVALID_PAGE_ID == nextSMP)
         {
            PD_LOG(PDERROR, "failed to get smp pid of pid[%d]", first + i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         if (smpPid != nextSMP)
         {
            PD_LOG(PDERROR, "allocating must be in one smp");
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }
      }

      currentSegCount = getSU()->getMetaSegmentCount();
      SDB_ASSERT(64 == pageCountInSeg, "must be 64");
      targetSegCount = (maxPid >> 6) + 1;
      if (currentSegCount < targetSegCount)
      {
         rc = getSU()->extendMetaFile(context, &targetSegCount);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to extend meta file to seg count[%d], rc:%d",
                   targetSegCount, rc);
            goto error;
         }
      }

      for (UINT32 i = 0; i < count; ++i)
      {
         ossValuePtr ptr = 0;
         PAGE_ID pid = first + i;
         rc = getSU()->getPagePtr(FILE_TYPE_DM, pid, ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get ptr of page[%d], rc:%d", pid, rc);
            goto error;
         }

         if (!initIdMapPage(pageSize, pid, (void *)ptr))
         {
            PD_LOG(PDERROR, "failed to init imp[%d]", pid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      rc = allocateNewIMPInSMP(context, smpPid, first, count);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate imp in smp:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::allocateNewIMPInSMP(requestContext *context,
                                            PAGE_ID smp,
                                            PAGE_ID pid,
                                            UINT32 count)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != smp, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(0 < count, "can not be zero");
      SDB_ASSERT(count <= 4, "lazy to add new interface of smpAccessor");
      PAGE_ID pids[4];
      smpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = TRUE;
      o.readOnly = FALSE;

      for (UINT32 i = 0; i < count; ++i)
      {
         pids[i] = pid + i;
      }

      rc = accessor.initWithOptions(context, FILE_TYPE_DM,
                                     smp, o, getSU());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init smp accessor of pid[%d], rc:%d",
                smp, rc);
         goto error;
      }

      rc = accessor.allocatePages(context, PAGE_TYPE_ID_MAP,
                                  count, NULL, pids, slice());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate pids[%d, %d] on smp:%d", pid, count, rc);
         goto error;
      }

   done:
      accessor.fini(context);
      return rc;
   error:
      goto done;
   }

   UINT32 mainDataSpace::getDataFileCount()
   {
      SDB_ASSERT(isOpen(), "must be open");
      return getSU()->getDataFileCount();
   }

   INT32 mainDataSpace::getDataSMPOfFile(UINT32 sequence, UINT32 i, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      UINT32 capacity = 0;
      UINT32 count = 0;
      UINT32 pageSize = 0;
      UINT32 pageCountInSeg = 0;
      UINT32 segCountInFile = 0;
      
      if (getSU()->getDataFileCount() < sequence)
      {
         rc = SDB_FNE;
         goto error;
      }

      rc = getSU()->getCoreArgs(FILE_TYPE_DD, &pageSize,
                                &pageCountInSeg, &segCountInFile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get core args");
         goto error;
      }

      if (!getSMPCapacityOrCount(pageSize, &capacity, &count))
      {
         PD_LOG(PDERROR, "failed to get capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (count <= i)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      pid = pageCountInSeg * segCountInFile * sequence + SMP_PAGE_ID + i;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 mainDataSpace::createDataFile(requestContext *context,
                                       UINT64 sequence)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != context, "can not be null");
      storageUnit *su = getSU();
      UINT32 maxPageCount = 0;
      UINT32 pageSize = 0;
      UINT32 capacity = 0;
      UINT32 smpCount = 0;
      PAGE_ID firstPid = INVALID_PAGE_ID;
      ossValuePtr ptr = 0;
      BOOLEAN rollbackFile = FALSE;

      rc = su->getMaxPageCountInDDFile(maxPageCount);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = logicalPageSpace::getDataPageSize(pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!getSMPCapacityOrCount(pageSize, &capacity, &smpCount))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = su->createNewDataFile(context, sequence);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create new data file[%lld], rc:%d", sequence, rc);
         goto error;
      }
      rollbackFile = TRUE;

      firstPid = sequence * maxPageCount;
      rc = su->getPagePtr(FILE_TYPE_DD, firstPid, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!initSmp(pageSize, firstPid, smpCount, (CHAR *)ptr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 1; i < smpCount; ++i)
      {
         rc = su->getPagePtr(FILE_TYPE_DD, firstPid + i, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (!initSmp(pageSize, firstPid + i, 0, (CHAR *)ptr))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      rc = su->fsync(FILE_TYPE_DD, firstPid, smpCount, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync file, pid[%d], count[%d], rc:%d",
                firstPid, smpCount, rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (rollbackFile)
      {
         su->removeLastDataFile(context);
      }
      goto done;
   }

   PAGE_ID mainDataSpace::getGlobalMetaPid()const
   {
      PAGE_ID pid = INVALID_PAGE_ID;
      UINT32 pageSize = 0;
      UINT32 count = 0;
      INT32 rc = logicalPageSpace::getMetaPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto done;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(pageSize, NULL, &count)))
      {
         goto done;
      }

      return SMP_PAGE_ID + count;
   done:
      return pid;
   }

   PAGE_ID mainDataSpace::getDataSmpPid(PAGE_ID pid)const
   {
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      INT32 rc = SDB_OK;
      PAGE_ID smp = INVALID_PAGE_ID;
      UINT32 capacity = 0;
      UINT32 pageSize = 0;
      UINT32 maxPageCountPerFile = 0;
      PAGE_ID firstSMPOfThisFile = INVALID_PAGE_ID;
      UINT32 delta = 0;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == pid))
      {
         goto done;
      }

      rc = logicalPageSpace::getDataPageSize(pageSize);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto done;
      }

      rc = getSU()->getMaxPageCountInDDFile(maxPageCountPerFile);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto done;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(pageSize, &capacity, NULL)))
      {
         goto done;
      }

      firstSMPOfThisFile = (pid & (~(maxPageCountPerFile - 1)));

      SDB_ASSERT(ossIsPowerOf2(maxPageCountPerFile), "must be power of 2");
      delta = ((pid / capacity) & (maxPageCountPerFile - 1));
      smp = firstSMPOfThisFile + delta;
   done:
      return smp;
   }

   INT32 mainDataSpace::initNecessaryPagesWhenCreating(requestContext *context,
                                                       storageUnit *su,
                                                       const strSlice &csName,
                                                       UINT32 uniqueID,
                                                       UINT32 logicalID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context && NULL != su, "can not be null");
      SDB_ASSERT(su->isOpen(), "must be open");
      SDB_ASSERT(0 == su->getMetaSegmentCount(), "must be zero");
      SDB_ASSERT(!csName.empty(), "can not be empty");
      UINT32 segCount = 1;
      UINT32 capacity = 0;
      UINT32 count = 0;
      UINT32 pageSize = 0;
      UINT32 occupied = 0;
      UINT32 pageCountInSeg = 0;
      ossValuePtr ptr = 0;

      rc = su->getCoreArgs(FILE_TYPE_DM, &pageSize, &pageCountInSeg);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         goto error;
      }

      if (OSS_UNLIKELY(!getSMPCapacityOrCount(pageSize, &capacity, &count)))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// extend file and init smps
      rc = su->extendMetaFile(context, &segCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to extent id map file:%d", rc);
         goto error;
      }

      occupied = count + getSystemPageCount() + getReservedImpCount();
      SDB_ASSERT(occupied < pageCountInSeg, "impossible");

      for (UINT32 i = 0; i < count; ++i)
      {
         PAGE_ID pid = SMP_PAGE_ID + i;
         rc = su->getPagePtr(FILE_TYPE_DM, pid, ptr);
         if (SDB_OK != rc)
         {
            goto error;
         }

         if (!initSmp(pageSize, pid, occupied, (CHAR *)ptr))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
         occupied = 0;
      }

      /// init cs
      rc = su->getPagePtr(FILE_TYPE_DM, count, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!initGmp(pageSize, count, csName, uniqueID, logicalID, (CHAR *)ptr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      /// init system imp
      rc = su->getPagePtr(FILE_TYPE_DM, count + 1, ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!initIdMapPage(pageSize, count + 1, (void *)ptr))
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = su->fsync(FILE_TYPE_DM, 0,
                     count + getSystemPageCount() + getReservedImpCount());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync pages:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}
}