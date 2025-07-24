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

   Source File Name = logicalPageSpacePte.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/logicalPageSpacePte.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/dataManagementService.h"
#include "vessel/pageInitializer.h"

namespace engine
{
namespace vessel
{
   logicalPageSpacePte::logicalPageSpacePte(const storageUnitManifest *manifest):
   logicalPageSpace(manifest)
   {

   }

   INT32 logicalPageSpacePte::getPublicPageBuffer(requestContext *context,
                                                  PAGE_ID lpid,
                                                  logicalPageBufferPte &buffer)
   {
      INT32 rc = SDB_OK;
      buffer.fini();
      
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         runtimePageBuffer rpb;
         _logicalPageBufferIniter initer;
         lpageDescriptor desc;

         rc = _getPageMapping().get(lpid, desc);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid mapping info:%d", rc);
            goto error;
         }
         else if (!desc.isValid())
         {
            rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
            goto error;
         }

         rc = _getRuntimePageBuffer(context, desc.pid, ossSharedLatchMode(), rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get runtime page buffer:%d", rc);
            goto error;
         }

         initer.init(lpid, ossSharedLatchMode(),
                     context, this,
                     std::move(rpb), desc.psv, buffer);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::getPageBuffer(requestContext *context,
                                            spacePteAccessCtx *actx,
                                            PAGE_ID lpid,
                                            logicalPageBufferPte &buffer)
   {
      INT32 rc = SDB_OK;
      lpageDescriptor desc;
      buffer.fini();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == actx ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         runtimePageBuffer rpb;
         _logicalPageBufferIniter initer;
         lpageDescriptor desc;
         BOOLEAN isPrivate = FALSE;
         rc = _getPtePrior(context, actx, lpid, desc, isPrivate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid[%d] mapping info:%d", lpid, rc);
            goto error;
         }
         else if (!desc.isValid())
         {
            rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
            goto error;
         }

         rc = _getRuntimePageBuffer(context, desc.pid, ossSharedLatchMode(), rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get runtime page buffer:%d", rc);
            goto error;
         }

         //SDB_ASSERT(lpid == rpb.getPageHead()->lpid, "must be same");

         if (isPrivate)
         {
            rc = rpb.prepareToWrite(context);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDERROR, "faield to get rpb ready to write:%d", rc);
               SDB_ASSERT(FALSE, "impossible");
               goto error;
            }
         }

         initer.init(lpid, ossSharedLatchMode(),
                     context, this,
                     std::move(rpb), desc.psv, buffer);
         buffer._ac = actx;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::allocatePtePage(requestContext *context,
                                              spacePteAccessCtx *actx,
                                              pageInitializer *initer,
                                              PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      runtimePageBuffer rpb;
      lpid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == actx ||
                            nullptr == initer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
         rc = _getSpaceMgr().reserve(pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve new pid:%d", rc);
            goto error;
         }

         rc = _reserveLpids(1, &lpid, FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve lpid:%d", rc);
            goto error;
         }

         rc = _getRuntimePageBufferToReset(context, pid, rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page buffer to reset:%d", rc);
            goto error;
         }

         rc = initer->initPage(context, lpid, psv, &rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init page:%d", rc);
            goto error;
         }

         actx->_pmap.insert(std::make_pair(lpid, pid));
      }
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _getSpaceMgr().release(pid);
      }
      if (INVALID_PAGE_ID != lpid)
      {
         _freeLpids(1, &lpid);
         lpid = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 logicalPageSpacePte::makePrivateBuffer(requestContext *context,
                                                spacePteAccessCtx *actx,
                                                logicalPageBufferPte &buffer)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID currentPid = INVALID_PAGE_ID;
      _logicalPageBufferIniter initer;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      PAGE_ID lpid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == actx ||
                            !buffer.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (actx->isPrivate(buffer.getLogicalPid()))
      {
         goto done;
      }

      lpid = buffer.getLogicalPid();
      currentPid = buffer.getGlobalPid().getPageId();

      rc = _getSpaceMgr().reserve(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve new pid:%d", rc);
         goto error;
      }

      psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
      rc = _copyPageAndReinitBuffer(context, psv,
                                    pid, initer.getRpbRef(buffer));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy page data:%d", rc);
         goto error;
      }

      actx->_obsoletePids.set(currentPid);
      actx->_pmap.insert(std::make_pair(lpid, pid));

   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _getSpaceMgr().release(pid);
      }
      goto done;
   }

   INT32 logicalPageSpacePte::removePage(requestContext *context,
                                         spacePteAccessCtx *actx,
                                         PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == actx ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         lpageDescriptor desc;
         BOOLEAN isPrivate = FALSE;
         rc = _getPtePrior(context, actx, lpid, desc, isPrivate);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lpid[%d] mapping info:%d", lpid, rc);
            goto error;
         }
         else if (!desc.isValid())
         {
            PD_LOG(PDERROR, "lpid[%d] not mapped", lpid);
            rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
            goto error;
         }
         else if (isPrivate)
         {
            lpageDescriptor publicDesc;
            rc = _getPageMapping().get(lpid, publicDesc);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get public page mapping:%d", rc);
               goto error;
            }

            if (publicDesc.isValid())
            {
               actx->_pmap[lpid] = INVALID_PAGE_ID;
               actx->_obsoleteLpids.set(lpid);
               actx->_obsoletePids.set(publicDesc.pid);
            }
            else
            {
               /// brand new lpid, free it at once.
               actx->erase(lpid);
               _freeLpid(lpid);
            }

            _getSpaceMgr().release(desc.pid);
         }
         else
         {
            actx->_obsoleteLpids.set(lpid);
            actx->_obsoletePids.set(desc.pid);
            actx->_pmap[lpid] = INVALID_PAGE_ID;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::removePages(requestContext *context,
                                          spacePteAccessCtx *actx,
                                          UINT32 size,
                                          const PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == actx ||
                       0 == size ||
                       nullptr == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         for (UINT32 i = 0; i < size; ++i)
         {
            rc = removePage(context, actx, lpids[i]);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to remove lpid[%d], rc:%d", lpids[i], rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::initViewer(BOOLEAN readonly, lpsPteViewer &v)
   {
      INT32 rc = SDB_OK;
      v.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
         if (!readonly)
         {
            mode.setUpgrade();
         }

         _publishingLocker.lockWith(mode);
         v = std::move(lpsPteViewer(&_publishingLocker, mode, getPSN()));
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::initWriteBatch(LPS_PTE_WRITE_BATCH &batch)
   {
      INT32 rc = SDB_OK;
      batch.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
         _publishingLocker.lockWith(mode);
         lpsPteViewer viewer(&_publishingLocker, mode, getPSN());
         LPS_PTE_WRITE_BATCH b(SDB_OSS_NEW lpsPteWriteBatch(std::move(viewer)));
         if (!b)
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         batch = std::move(b);
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::precommit(requestContext *context,
                                        lpsPteWriteBatch &batch,
                                        PTE_ACCESS_CTX_PTR &&ac)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(nullptr == context ||
                       !batch.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (ac->isEmpty())
      {
         ac.reset();
      }
      else
      {
         spacePteAccessCtx *obj = ac.get();
         batch.precommit(std::move(ac));

         PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
         SDB_ASSERT(getPSN() == batch.getPSN(), "must be same");
         lpageMapping &mapping = _getPageMapping();
         rc = _fsyncPrivatePages(obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fsync files:%d", rc);
            goto error;
         }

         for (auto itr = obj->_pmap.cbegin(); itr != obj->_pmap.cend(); ++itr)
         {
            SDB_ASSERT(INVALID_PAGE_ID != itr->first, "can not be invalid");
            if (INVALID_PAGE_ID != itr->second)
            {
               lpageDescriptor desc(itr->second, psv);
               rc = mapping.set(batch._mctx, itr->first, desc);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to set lpage mapping:%d", rc);
                  goto error;
               }
            }
            else
            {
               rc = mapping.reset(batch._mctx, itr->first);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reset lpage mapping:%d", rc);
                  goto error;
               }
            }
         }
         
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::commit(LPS_PTE_WRITE_BATCH &batch)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!batch || !batch->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         SDB_ASSERT(batch->getPSN() == getPSN(), "must be same");
         rc = _commit(*batch);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to commit batch to lpm:%d", rc);
            goto error;
         }

         _freeObsoleteResources(*batch);
         PD_LOG(PDDEBUG, "pte batch committed, task num[%d], current psn[%d]",
                batch->_committing.size(), getPSN());
         batch.reset();
      }
   done:
      return rc;
   error:
      SDB_ASSERT(FALSE, "impossible");
      goto done;
   }

   void logicalPageSpacePte::abort(LPS_PTE_WRITE_BATCH &batch)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(nullptr != batch && batch->isValid(), "can not be invalid");

      _getPageMapping().abort(batch->_mctx);

      for (auto itr = batch->_committing.begin();
           itr != batch->_committing.end(); ++itr)
      {
         abort(itr->second);
      }

      batch.reset();
      return;
   }

   void logicalPageSpacePte::abort(PTE_ACCESS_CTX_PTR &ac)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");

      const spacePteAccessCtx *ctx = ac.get();
      SDB_ASSERT(nullptr != ctx, "can not be invalid");
      for (auto itr = ctx->_pmap.cbegin();
           itr != ctx->_pmap.cend(); ++itr)
      {
         if (INVALID_PAGE_ID == itr->second)
         {
            /// it is a removing operation, do not to abort.
            continue;
         }
         else
         {
            lpageDescriptor desc;
            INT32 rc = _getPageMapping().get(itr->first, desc);
            if (OSS_UNLIKELY(SDB_OK != rc))
            {
               PD_LOG(PDSEVERE, "failed to get desc of lpid[%d], rc:%d",
                      itr->first, rc);
               ossPanic();
            }

            _getSpaceMgr().release(itr->second);
            if (!desc.isValid())
            {
               /// it is a brand new lpid
               _freeLpid(itr->first);
            }
         }
      }

      ac.reset();
      return;
   }

   INT32 logicalPageSpacePte::_commit(lpsPteWriteBatch &batch)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(batch.isValid(), "can not be invalid");

      if (!batch.hasPteMapping())
      {
         goto done;
      }

      // rc = _fsyncDirtyClusterFiles(batch);
      // if (OSS_UNLIKELY(SDB_OK != rc))
      // {
      //    PD_LOG(PDERROR, "failed to fsync dirty files:%d", rc);
      //    goto error;
      // }

      rc = _getMetaFile().fsync();
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to fsync meta data file:%d", rc);
         goto error;
      }

      batch._viewer._transferToExclusiveLock();

      _getPageMapping().publish(batch._mctx);
      _psn.fetch_add(1, std::memory_order_relaxed);

      batch._viewer._transferToUpgradeLock();

      rc = _updateUberBlockOnDisk(TRUE);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDSEVERE, "failed to update uber block on disk:%d", rc);
         ossPanic();
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageSpacePte::_freeObsoleteResources(lpsPteWriteBatch &batch)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(batch.isValid(), "can not be invalid");
      for (auto itr = batch._committing.cbegin();
           itr != batch._committing.cend(); ++itr)
      {
         _getSpaceMgr().releaseBatch(itr->second->_obsoletePids);
         _freeLpids(itr->second->_obsoleteLpids);
      }

      _getPageMapping().freeOboleteSetAfterPublish(batch._mctx);
   }

   INT32 logicalPageSpacePte::_getRuntimePageBuffer(requestContext *context,
                                                    PAGE_ID pid,
                                                    const ossSharedLatchMode &mode,
                                                    runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      storageFileCluster *fcluster = nullptr;
      logicalPageSpace::_runtimePageBufferIniter initer;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = 0;
      mmapPagePointer ptr;

      if (OSS_UNLIKELY(nullptr == context ||
                       INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fcluster = getFileCluster();
      SDB_ASSERT(nullptr != fcluster, "can not be null");

      rc = fcluster->getPageMmapPtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 FILE_TYPE_DATA_STORAGE,
                 pid);
      pageSize = getFileCluster()->getCoreArgs().pageSize;

      initer.initWithMmap(gpid, pageSize, ptr, rpb);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::_getRuntimePageBufferToReset(requestContext *context,
                                                           PAGE_ID pid,
                                                           runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      rc = this->_getRuntimePageBuffer(context, pid,
                                       mode, /// usless actually
                                       rpb);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get rpb ready to write:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      rpb.fini();
      goto done;
   }

   INT32 logicalPageSpacePte::_copyPageAndReinitBuffer(requestContext *context,
                                                       PAGE_SNAPSHOT_VERION psv,
                                                       PAGE_ID newPid,
                                                       runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = getFileCluster()->getCoreArgs().pageSize;
      mmapPagePointer ptr;
      GLOBAL_PAGE_ID gpid;
      logicalPageSpace::_runtimePageBufferIniter initer;
      slice rs;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       INVALID_PAGE_ID == newPid ||
                       !rpb.isValid() ||
                       rpb.isCacheBuffer()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      rs = rpb.getSlice();
      if (isPageCrashed((ossValuePtr)(rs.data()), pageSize))
      {
         PD_LOG(PDERROR, "page[%s] may be crashed", rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      rc = getFileCluster()->getPageMmapPtr(newPid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get new page[%d] ptr:%d", newPid, rc);
         goto error;
      }

      ossMemcpy((void *)(ptr.get()), rs.data(), pageSize);
      ((pageHead *)(ptr.get()))->pid = newPid;
      ((pageHead *)(ptr.get()))->psv = psv;

      rpb.fini();

      gpid.reset(getSpaceID(),
                 getSpaceType(),
                 FILE_TYPE_DATA_STORAGE,
                 newPid);

      initer.initWithMmap(gpid, pageSize, ptr, rpb);
      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         rpb.fini();
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         SDB_ASSERT(FALSE, "impossible");
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::_getPtePrior(requestContext *context,
                                           spacePteAccessCtx *actx,
                                           PAGE_ID lpid,
                                           lpageDescriptor &desc,
                                           BOOLEAN &isPrivate)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(nullptr != actx, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      PAGE_ID pid = INVALID_PAGE_ID;
      desc.reset();

      if (actx->get(lpid, pid))
      {
         if (INVALID_PAGE_ID != pid)
         {
            PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();
            desc.reset(pid, psv);
            isPrivate = TRUE;
         }
      }
      else
      {
         rc = _getPageMapping().get(lpid, desc);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page descriptor:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::_fsyncPrivatePages(spacePteAccessCtx *ctx)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != ctx, "can not be invalid");
      sparseBitmap32 pids;
      ctx->exportDirtyPids(pids);
      PD_LOG(PDDEBUG, "begin to flush pte pages[%d]", pids.getTotalNum());
      sparseBitmap32::iterator itr;
      while (pids.next(itr))
      {
         INT32 rc = getFileCluster()->fysncPage(itr.get(), FALSE);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to fsync file segment[%d], rc:%d", itr.get(), rc);
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpacePte::_fsyncDirtyClusterFiles(const lpsPteWriteBatch &batch)
   {
      INT32 rc = SDB_OK;
      const storageFileManifest &manifest = getFileCluster()->getManifest();
      ossPoolSet<UINT32> files = batch.exportDirtyFiles(manifest.args);
      PD_LOG(PDDEBUG, "[%d] dirty files exported", files.size());
      for (auto i = files.cbegin(); i != files.cend(); ++i)
      {
         INT32 rc = getFileCluster()->fsyncFile(*i);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to fsync file[%d], rc:%d", *i, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
