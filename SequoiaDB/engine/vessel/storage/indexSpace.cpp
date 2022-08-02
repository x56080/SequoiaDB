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

   Source File Name = indexSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexSpace.h"
#include "vessel/idMapPage.h"
#include "vessel/indexDef.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/instanceEnv.h"
#include "vessel/pageInitializer.h"

namespace engine
{
namespace vessel
{
   INT32 indexSpace::openAccessCtx(requestContext *context,
                                   indexSpaceAccessCtx &ac)
   {
      INT32 rc = SDB_OK;
      ac.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ac.init(&_mutex, context, this);
   done:
      return rc;
   error:
      goto done;
   }

   void indexSpace::abort(indexSpaceAccessCtx &ctx)
   {
      SDB_ASSERT(isOpen() && ctx.isValid(), "can not be invalid");
      SDB_ASSERT(ctx._is == this, "invalid instance ptr");     
      _rollbackPagesReserved(ctx);
      _rollbackPageRemapped(ctx);
      ctx.reset();

      return;
   }

   INT32 indexSpace::getLogicalPageBuffer(indexSpaceAccessCtx &ctx,
                                          PAGE_ID lpid,
                                          BOOLEAN pteEnabled,
                                          logicalPageBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      runtimePageBuffer rpb;
      _logicalPageBufferIniter initer;
      lpageDescriptor desc;

      buffer.fini();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!ctx.isValid() ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = pteEnabled ?
           _getPageMapping().getPtePrior(_mappingCtx, lpid, desc) :
           _getPageMapping().get(lpid, desc);
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

      rc = _getRuntimePageBuffer(ctx._rctx, desc.pid, ossSharedLatchMode(), rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime page buffer:%d", rc);
         goto error;
      }

      if (!pteEnabled)
      {
         _runtimePageBufferIniter().banWrite(rpb);
      }

      initer.init(lpid, ossSharedLatchMode(),
                  ctx._rctx, this,
                  std::move(rpb), desc.psv, buffer);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::makePrivateBuffer(indexSpaceAccessCtx &ctx,
                                       logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID currentPid = INVALID_PAGE_ID;
      _logicalPageBufferIniter initer;
      lpageDescriptor desc;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!ctx.isValid() ||
                            !lpb.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (ctx.isPrivate(lpb.getLogicalPid()))
      { 
         goto done;
      }

      currentPid = lpb.getGlobalPid().getPageId();

      rc = _ppidsToFree.prepare(currentPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare bitmap:%d", rc);
         goto error;
      }

      rc = ctx._remapped.prepare(lpb.getLogicalPid());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare bitmap:%d", rc);
         goto error;
      }

      rc = _getSpaceMgr().reserve(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve new pid:%d", rc);
         goto error;
      }
      
      desc = lpageDescriptor(pid, ctx._rctx->getEnv()->dms.getOnlinePageSnapshotVersion());
      rc = _getPageMapping().set(_mappingCtx, lpb.getLogicalPid(), desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set pte mapping:%d", rc);
         goto error;
      }
      
      rc = _copyPageAndReinitBuffer(ctx._rctx, desc.psv,
                                    pid, initer.getRpbRef(lpb));
      if (SDB_OK != rc)
      {
         _getPageMapping().reset(_mappingCtx, lpb.getLogicalPid());
         PD_LOG(PDERROR, "failed to copy page data:%d", rc);
         goto error;
      }

      _ppidsToFree.set(currentPid);
      ctx._remapped.set(lpb.getLogicalPid());
                           
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _getSpaceMgr().release(pid);
      }
      goto done;
   }

   INT32 indexSpace::allocate(indexSpaceAccessCtx &ctx,
                              pageInitializer *initer,
                              PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      lpageDescriptor desc;
      PAGE_SNAPSHOT_VERION psv = INVALID_PAGE_SNAPSHOT_VERSION;
      PAGE_ID pid = INVALID_PAGE_ID;
      runtimePageBuffer rpb;

      lpid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!ctx.isValid() ||
                            nullptr == initer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      psv = ctx._rctx->getEnv()->dms.getOnlinePageSnapshotVersion();

      rc = _getSpaceMgr().reserve(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve new pid:%d", rc);
         goto error;
      }

      rc = _reserveLpids(1, &lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve lpid:%d", rc);
         goto error;
      }

      desc.reset(pid, psv);
      rc = ctx._reserved.prepare(lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare lpid bitmap:%d", rc);
         goto error;
      }

      rc = _getRuntimePageBufferToReset(ctx._rctx, pid, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page buffer to reset:%d", rc);
         goto error;
      }

      rc = initer->initPage(ctx._rctx, lpid, psv, &rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init page:%d", rc);
         goto error;
      }

      rc = _getPageMapping().set(_mappingCtx, lpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set mapping:%d", rc);
         goto error;
      }

      ctx._reserved.set(lpid);
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

   INT32 indexSpace::removePage(indexSpaceAccessCtx &ctx,
                                PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      lpageDescriptor desc;
      
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!ctx.isValid() ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _getPageMapping().getPtePrior(_mappingCtx, lpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read mapping:%d", rc);
         goto error;
      }
      else if (!desc.isValid())
      {
         PD_LOG(PDERROR, "lpid[%d] not mapped", lpid);
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      if (ctx.isReserved(lpid))
      {
         rc = _getPageMapping().reset(_mappingCtx, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reset lpid[%d], rc:%d", lpid, rc);
            goto error;
         }
         _getSpaceMgr().release(desc.pid);
         _freeLpids(1, &lpid);
         ctx._reserved.reset(lpid);
      }
      else if (ctx.isRemapped(lpid))
      {
         rc = _lpidsToFree.prepare(lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare bitmap:%d", rc);
            goto error;
         }

         rc = _getPageMapping().reset(_mappingCtx, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reset lpid[%d], rc:%d", lpid, rc);
            goto error;
         }

         _lpidsToFree.set(lpid);
         _getSpaceMgr().release(desc.pid);
      }
      else
      {
         rc = ctx._remapped.prepare(lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare bitmap:%d", rc);
            goto error;
         }

         rc = _lpidsToFree.prepare(lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare bitmap:%d", rc);
            goto error;
         }

         rc = _ppidsToFree.prepare(lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to prepare bitmap:%d", rc);
            goto error;
         }

         rc = _getPageMapping().reset(_mappingCtx, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reset lpid[%d], rc:%d", lpid, rc);
            goto error;
         }

         ctx._remapped.set(lpid);
         _lpidsToFree.set(lpid);
         _ppidsToFree.set(desc.pid);
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::removePages(indexSpaceAccessCtx &ctx,
                                 UINT32 size,
                                 const PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(0 == size || nullptr == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < size; ++i)
      {
         rc = removePage(ctx, lpids[i]);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove page[%d], rc:%d", lpids[i], rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::_getRuntimePageBuffer(requestContext *context,
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

      if (OSS_UNLIKELY(NULL == context ||
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

   INT32 indexSpace::_getRuntimePageBufferToReset(requestContext *context,
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

   INT32 indexSpace::_copyPageAndReinitBuffer(requestContext *context,
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
   done:
      return rc;
   error:
      goto done;
   }

   void indexSpace::_rollbackPagesReserved(indexSpaceAccessCtx &ctx)
   {
      lpageMapping &lpm = _getPageMapping();
      sparsePidBitmap &bitmap = ctx._reserved;
      sparsePidBitmap::iterator itr = bitmap.begin();
      while (itr.isValid())
      {
         PAGE_ID lpid = itr.getValue();
         bitmap.next(itr);
         lpageDescriptor desc;
         INT32 rc = lpm.reset(_mappingCtx, lpid, &desc);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get mapping info of lpid[%d], rc:%d",
                   lpid, rc);
            continue;
         }
         else if (!desc.isValid())
         {
            PD_LOG(PDERROR, "lpid[%d] not mapped", lpid);
            SDB_ASSERT(FALSE, "impossible");
            continue;
         }

         ///TODO: snapshot
         _freeLpids(1, &lpid);
         _getSpaceMgr().release(desc.pid);
      }

      bitmap.reset();

      return;
   }

   void indexSpace::_rollbackPageRemapped(indexSpaceAccessCtx &ctx)
   {
      lpageMapping &lpm = _getPageMapping();
      sparsePidBitmap &bitmap = ctx._remapped;
      sparsePidBitmap::iterator itr = bitmap.begin();
      while (itr.isValid())
      {
         PAGE_ID lpid = itr.getValue();
         bitmap.next(itr);
         lpageDescriptor pte;
         lpageDescriptor published;
         INT32 rc = lpm.revert(_mappingCtx, lpid, pte, published);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to revert lpid[%d], rc:%d", lpid, rc);
            continue;
         }

         SDB_ASSERT(published.isValid(), "impossible");
         _ppidsToFree.reset(published.pid);
         if (pte.isValid())
         {
            /// page remapped
            _getSpaceMgr().release(pte.pid);
         }
         else
         {
            /// page removed
            _lpidsToFree.reset(lpid);
         }
      }

      bitmap.reset();

      return;
   }
}//namespace vessel
}//namespace engine