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

   Source File Name = logicalPageSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/logicalPageSpace.h"
#include "vessel/requestContext.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/threadContext.h"
#include "vessel/vesselOptions.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileMaintainer.h"
#include "dpsWriteReqBuilder.hpp"
#include "vessel/containerUtils.h"
#include "vessel/pageInitializer.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   constexpr UINT32 UBER_BLOCK_PID = 0;

///////////////logicalPageSpace::_runtimePageBufferIniter
   void logicalPageSpace::
         _runtimePageBufferIniter::
         initWithBuffer(const GLOBAL_PAGE_ID &gpid,
                                   UINT32 pageSize,
                                   liteIOBuffer &iob,
                                   runtimePageBuffer &rpb)
   {
      rpb.initWithBuffer(gpid, pageSize, iob);
   }

   void logicalPageSpace::
         _runtimePageBufferIniter::
         initWithMmap(const GLOBAL_PAGE_ID &gpid,
                      UINT32 pageSize,
                      const mmapPagePointer &ptr,
                      runtimePageBuffer &rpb)
   {
      rpb.initWithMmap(gpid, pageSize, ptr);
   }

///////////////logicalPageSpace::_runtimePageBufferIniter end

///////////////logicalPageSpace::_logicalPageBufferIniter
   void logicalPageSpace::
        _logicalPageBufferIniter::init(PAGE_ID lpid,
                                       ossSharedLatchMode mode,
                                       requestContext *context,
                                       logicalPageSpace *lps,
                                       runtimePageBuffer &&rpb,
                                       PAGE_SNAPSHOT_VERION psv,
                                       logicalPageBuffer &lpb)
   {
      lpb.fini();
      lpb._lpid = lpid;
      lpb._mode = mode;
      lpb._context = context;
      lpb._lps = lps;
      lpb._rpb = std::move(rpb);
      lpb._psv = psv;
      return;
   }

///////////////logicalPageSpace::_logicalPageBufferIniter end

   logicalPageSpace::logicalPageSpace(const storageUnitManifest *manifest):
   _manifest(manifest)
   {
      SDB_ASSERT(nullptr != _manifest, "can not be invalid");
   }

   logicalPageSpace::~logicalPageSpace()
   {

   }

   INT32 logicalPageSpace::create()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not init");

      if (OSS_UNLIKELY(nullptr == _manifest ||
                       !_manifest->isValid()))
      {
         PD_LOG(PDERROR, "can not create lps with invalid manifest");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _onCreationStarted();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start creation:%d", rc);
         goto error;
      }

      rc = _createMetaFile();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create meta data file:%d", rc);
         goto error;
      }

      rc = _createUberBlock();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init uber block:%d", rc);
         goto error;
      }

      rc = _initPageMapping(TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpage mapping:%d", rc);
         goto error;
      }

      rc = _initLpidAllocator();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid allocator:%d", rc);
         goto error;
      }

      rc = _openFileCluster(nullptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file cluster:%d", rc);
         goto error;
      }

      rc = _initSpaceManager();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init space manager:%d", rc);
         goto error;
      }

      rc = _onCreationFinished();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to finish creation:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      destroy();
      goto done;
   }

   INT32 logicalPageSpace::open(const storageFileLoader &loader)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not init");

      if (OSS_UNLIKELY(nullptr == _manifest ||
                       !_manifest->isValid()))
      {
         PD_LOG(PDERROR, "can not create lps with invalid manifest");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _onOpenStarted(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start open:%d", rc);
         goto error;
      }

      rc = _openMetaFile();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to meta file:%d", rc);
         goto error;
      }

      rc = _initPageMapping(FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpage mapping:%d", rc);
         goto error;
      }

      rc = _initLpidAllocator();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpid allocator:%d", rc);
         goto error;
      }

      rc = _openFileCluster(&loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file cluster:%d", rc);
         goto error;
      }

      rc = _initSpaceManager();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init space manager:%d", rc);
         goto error;
      }

      rc = _onOpenFinished(loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to finish open lps:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void logicalPageSpace::close()
   {
      if (isOpen())
      {
         _onClosingStarted();

         INT32 rc = _updateUberBlockOnDisk(FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDSEVERE, "failed to update uber block on disk:%d", rc);
            ossPanic();
         }

         _smgr.fini();
         _fcluster.close();
         _lpm.fini();
         _mfile.fsync();
         _mfile.close();
         _allocator.fini();
         _onClosingFinished();
      }
      return;
   }

   void logicalPageSpace::destroy()
   {
      if (isOpen())
      {
         _onDestroyStarted();
         _smgr.fini();
         _fcluster.destroy();
         _lpm.fini();
         _mfile.destroy();
         _allocator.fini();
         _onDestroyFinished();
      }

      return;
   }

   INT32 logicalPageSpace::getLogicalPageBuffer(requestContext *context,
                                                PAGE_ID lpid,
                                                const ossSharedLatchMode &mode,
                                                logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      lpageDescriptor desc;
      BOOLEAN locked = FALSE;

      lpb.fini();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            INVALID_PAGE_ID == lpid ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->lockLpid(getSpaceType(), lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      locked = TRUE;

      rc = _lpm.get(lpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in mapping:%d", lpid, rc);
         goto error;
      }
      else if (!desc.isValid())
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      rc = _getRuntimePageBuffer(context, desc.pid,
                                 mode, lpb._rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime buffer of pid[%d], rc:%d", desc.pid, rc);
         goto error;
      }

      lpb._lpid = lpid;
      lpb._mode = mode;
      lpb._context = context;
      lpb._lps = this;
      lpb._psv = desc.psv;
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockLpid(getSpaceType(), lpid);
      }
      goto done;
   }

   INT32 logicalPageSpace::tryToGetLogicalPageBuffer(requestContext *context,
                                                     PAGE_ID lpid,
                                                     const ossSharedLatchMode &mode,
                                                     logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!lpb.isValid(), "impossible");
      BOOLEAN locked = FALSE;
      lpageDescriptor desc;

      lpb.fini();
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            INVALID_PAGE_ID == lpid ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->tryLockLpid(getSpaceType(), lpid, mode, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      else if (!locked)
      {
         goto done;
      }

      rc = _lpm.get(lpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }
      else if (!desc.isValid())
      {
         rc = SDB_VESSEL_LOGICAL_PAGE_UNMAPPED;
         goto error;
      }

      rc = _getRuntimePageBuffer(context, desc.pid,
                                 mode, lpb._rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get runtime buffer of pid[%d], rc:%d", desc.pid, rc);
         goto error;
      }

      lpb._lpid = lpid;
      lpb._mode = mode;
      lpb._context = context;
      lpb._lps = this;
      lpb._psv = desc.psv;
   done:
      return rc;
   error:
      if (lpb._rpb.isValid())
      {
         lpb._rpb.fini();
      }
      if (locked)
      {
         context->unlockLpid(getSpaceType(), lpid);
      }
      goto done;
   }

   INT32 logicalPageSpace::testLogicalPageMapping(PAGE_ID lpid,
                                                  lpageDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      desc.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_lpm.isOutOfMaxBound(lpid)))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = _lpm.get(lpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to find lpid[%d] in cache:%d", lpid, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::makeBufferWritable(logicalPageBuffer &lpb)
   {
      INT32 rc = SDB_OK;
      BOOLEAN snapshotEffective = FALSE;
      runtimePageBuffer &rpb = lpb._rpb;
      requestContext *context = lpb._context;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!lpb.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(lpb._lps != this))
      {
         SDB_ASSERT(FALSE, "lpb not allocated by this space");
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!lpb._mode.isExclusiveOrUpgrade()))
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else if (rpb.isWritingPrepared())
      {
         goto done;
      }

      if (lpb._mode.isUpgrade())
      {
         lpb.lockExclusiveFromUpgrade();
      }

      rc = context->getEnv()->dms.isSnapshotEffective(getSpaceID(),
                                                      lpb.getPsv(),
                                                      snapshotEffective);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to check if snapshot effective:%d", rc);
         goto error;
      }

      if (snapshotEffective)
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      
      SDB_ASSERT(rpb.isValid(), "must be valid");
      if (!rpb.isWritingPrepared())
      {
         rc = rpb.prepareToWrite(context);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get rpb writable:%d", rc);
            goto error;
         }
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::ensureReservedPageMapped(requestContext *context,
                                                    PAGE_ID lpid,
                                                    pageInitializer *initer)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode;
      PAGE_ID pid = INVALID_PAGE_ID;
      lpageDescriptor desc;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            INVALID_PAGE_ID == lpid ||
                            nullptr == initer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!_isReservedLpid(lpid))
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (!context->testLpidLocked(getSpaceType(), lpid, &mode) ||
               !mode.isExclusive())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }

      rc = _lpm.get(lpid, desc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get desc from mapping:%d", rc);
         goto error;
      }
      else if (desc.isValid())
      {
         goto done;
      }

      rc = _smgr.reserve(pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate page from file cluster:%d", rc);
         goto error;
      }

      rc = _initAndCreateMapping(context, initer, 1, &lpid, &pid);
      if (SDB_OK != rc)
      {
         _smgr.release(pid);
         PD_LOG(PDERROR, "failed to init page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::allocatePages(requestContext *context,
                                         pageInitializer *initer,
                                         UINT32 count,
                                         PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      PID_ARRAY pids;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID *pidPtr = nullptr;
      BOOLEAN lpidAllocated = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      } 
      else if (OSS_UNLIKELY(nullptr == context ||
                            nullptr == initer ||
                            0 == count ||
                            nullptr == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(_fcluster.getCoreArgs().maxPageCountPerSeg < count))
      {
         PD_LOG(PDERROR, "batch count[%d] out of segment size", count);
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = _reserveLpids(count, lpids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve lpids:%d", rc);
         goto error;
      }
      lpidAllocated = TRUE; 

      rc = _smgr.reserveExtent(count, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve data extent:%d", rc);
         goto error;
      }

      if (1 < count)
      {
         pids = context->allocateArray<PAGE_ID>(count);
         if (OSS_UNLIKELY(!pids.isValid()))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         for (UINT32 i = 0; i < count; ++i)
         {
            pids[i] = pid + i;
         }

         pidPtr = pids.data();
      }
      else
      {
         pidPtr = &pid;
      }

      rc = _initAndCreateMapping(context, initer, count,
                                 lpids, pidPtr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init and create mapping:%d", rc);
         goto error;
      }

   done:
      if (pids.isValid())
      {
         context->releaseBuffer(pids.data());
      }
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _smgr.releaseExtent(pid, count);
      }
      if (lpidAllocated)
      {
         _freeLpids(count, lpids);
      }
      for (UINT32 i = 0; i < count; ++i)
      {
         lpids[i] = INVALID_PAGE_ID;
      }
      goto done;
   }

   INT32 logicalPageSpace::releasePages(requestContext *context,
                                        UINT32 count,
                                        const PAGE_ID *lpids)
   {
      INT32 rc = SDB_OK;
      dataManagementService *dms = nullptr;
      ossPoolVector<PAGE_ID> pidsToFree;
      ossPoolVector<PAGE_ID> lpidsToFree;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      } 
      else if (OSS_UNLIKELY(nullptr == context ||
                            0 == count ||
                            nullptr == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      lpidsToFree.reserve(count);
      dms = &(context->getEnv()->dms);
      for (UINT32 i = 0; i < count; ++i)
      {
         BOOLEAN snapshotEffective = FALSE;
         PAGE_ID lpid = lpids[i];
         lpageDescriptor desc;

         if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
         {
            continue;
         }

         rc = _lpm.get(lpid, desc);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] from cache, rc:%d", lpid, rc);
            rc = SDB_OK;
         }
         else if (!desc.isValid())
         {
            PD_LOG(PDERROR, "lpid[%d] not mapped", lpid);
            continue;
         }

         rc = dms->isSnapshotEffective(getSpaceID(), desc.psv,
                                       snapshotEffective);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get snapshot status:%d", rc);
            goto error;
         }
         else if (!snapshotEffective)
         {
            pidsToFree.push_back(desc.pid);
         }

         lpidsToFree.push_back(lpid);
      }

      if (!lpidsToFree.empty())
      {
         _lpm.resetBatch(lpidsToFree.size(),
                         lpidsToFree.data());
         _freeLpids(lpidsToFree.size(),
                    lpidsToFree.data());
      }

      if (!pidsToFree.empty())
      {
         /// sort pids first to avoid random IO(sme) and
         /// reduce locking times.
         if (1 < pidsToFree.size())
         {
            std::sort(pidsToFree.begin(), pidsToFree.end());
         }
         _smgr.releaseBatch(pidsToFree.size(), pidsToFree.data());
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::_initPageMapping(BOOLEAN creating)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      rc = _mfile.makeReadableBuffer(UBER_BLOCK_PID, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get uber block buffer:%d", rc);
         goto error;
      }

      rc = _lpm.init(&_mfile, buffer.getReadableObjPtr<lpmUberBlock>(0));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init lpage mapping:%d", rc);
         goto error;
      }

      if (creating)
      {
         for (UINT32 i = 0; i < _getReservedLpidUnits(); ++i)
         {
            rc = _lpm.ensureUnitSpace(i);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure mapping unit:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::_createUberBlock()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_mfile.isOpen(), "can not be invalid");
      mmapPagePointer ptr;
      strictBuffer buffer;
      PAGE_ID pid = INVALID_PAGE_ID;
      lpmUberBlock *block = nullptr;
      PAGE_ID smePid = INVALID_PAGE_ID;
      strictBuffer smeBuffer;

      rc = _mfile.reservePid(pid, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve block pid:%d", rc);
         goto error;
      }
      else if (UBER_BLOCK_PID != pid)
      {
         PD_LOG(PDERROR, "invalid pid[%d] of uber block reserved", pid);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _mfile.reservePid(smePid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve sme pid:%d", rc);
         goto error;
      }

      rc = _mfile.makeWritableBuffer(smePid, smeBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make buffer of pid[%d], rc:%d", rc);
         goto error;
      }

      smeBuffer.setBuffer(0xFF);

      buffer.makeWritable(lpageMetaDataFile::PAGE_SIZE, ptr.getBuf());
      buffer.setBuffer(0x00);
      block = buffer.getWritableObjPtr<lpmUberBlock>(0);
      SDB_ASSERT(nullptr != block, "can not be invalid");
      block->reset();
      block->version = lpmUberBlock::VERSION;
      block->smeEntryPid = smePid;
      block->refillChecksum();
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != pid)
      {
         _mfile.freePid(pid);
      }
      if (INVALID_PAGE_ID != smePid)
      {
         _mfile.freePid(smePid);
      }
      goto done;
   }

   INT32 logicalPageSpace::_createMetaFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _manifest && _manifest->isValid(), "can not be invalid");
      SDB_ASSERT(!_mfile.isOpen(), "can not be open");
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest->id.getSpaceId());
      storageFileName fn;
      createStorageFileOptions options;
      storageCoreArgs args(lpageMetaDataFile::PAGE_SIZE,
                           lpageMetaDataFile::PAGE_COUNT_PER_SEG,
                           lpageMetaDataFile::MAX_SEG_COUNT);
      createStorageFileOptions o;

      if (!fn.build(FILE_TYPE_LPM, getSpaceType()))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      o.secretValue = _manifest->secretValue;
      o.args = args;
      o.createAsTmpFile = TRUE;
      o.replaceWhenCreate = TRUE;
      o.flags = storageFileCtlFlag::MMAP_DATA_SEGMENT;

      rc = sfm.createStorageFile(fn, o, _mfile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create meta data file:%d", rc);
         goto error;
      }

      rc = _mfile.fsync();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fsync meta file:%d", rc);
         goto error;
      }

      rc = _mfile.removeShadowSuffix();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove file's shadow suffix:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      if (_mfile.isOpen())
      {
         _mfile.destroy();
      }
      goto done;
   }

   INT32 logicalPageSpace::_openMetaFile()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _manifest && _manifest->isValid(), "can not be invalid");
      SDB_ASSERT(!_mfile.isOpen(), "can not be open");
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _manifest->id.getSpaceId());
      storageFileName fn;

      if (!fn.build(FILE_TYPE_LPM, getSpaceType()))
      {
         PD_LOG(PDERROR, "failed to build file name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = sfm.openStorageFile(fn, _getStorageFileCtlFlags(), _mfile);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open meta file, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::_openFileCluster(const storageFileLoader *loader)
   {
      INT32 rc = SDB_OK;
      storageFileManifest manifest;
      manifest.sid = _manifest->id.getSpaceId();
      manifest.stype = getSpaceType();
      manifest.ftype = FILE_TYPE_DATA_STORAGE;
      manifest.secretValue = _manifest->secretValue;
      if (SPACE_TYPE_MAIN_DATA == getSpaceType())
      {
         manifest.args = _manifest->dataArgs;
      }
      else if (SPACE_TYPE_IDX == getSpaceType())
      {
         manifest.args = _manifest->idxArgs;
      }
      else
      {
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _fcluster.open(manifest, _getStorageFileCtlFlags(), loader);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open file cluster:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::_initSpaceManager()
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      PAGE_ID pid = UBER_BLOCK_PID;
      const lpmUberBlock *block = nullptr;

      rc = _mfile.makeWritableBuffer(pid, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make buffer of pid[%d], rc:%d", pid, rc);
         goto error;
      }

      block = buffer.getReadableObjPtr<lpmUberBlock>(0);
      rc = _smgr.init(block->smeEntryPid, &_mfile, &_fcluster,
                      _getSegmentPcntReused());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed toi init space manager:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::_initLpidAllocator()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_lpm.isValid(), "can not be invalid");
      fixedBitset<lpageMapping::LPID_UNIT_SIZE> bs;

      _LPID_ALLOCATOR::options o;
      o.minFreeReused = 8;
      o.frozenPreUnits = _getReservedLpidUnits();
      _allocator.init(o);

      for (UINT32 i = o.frozenPreUnits; i < lpageMapping::MAX_UNIT_COUNT; ++i)
      {
         BOOLEAN exists = FALSE;
         bs.clearAll();

         rc = _lpm.dumpUnitSme(i, exists, bs);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump sme of unit[%d], rc:%d", i, rc);
            goto error;
         }
         else if (!exists)
         {
            break;
         }
         else
         {
            rc = _allocator.appendUnit(bs);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to append unit into allocator:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      _allocator.fini();
      goto done;
   }

   INT32 logicalPageSpace::_getRuntimePageBuffer(requestContext *context,
                                                 PAGE_ID pid,
                                                 const ossSharedLatchMode &mode,
                                                 runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      _runtimePageBufferIniter initer;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = 0;
      liteIOBufferPool::allocateOptions o;
      o.mode = mode;
      liteIOBuffer buffer;

      rpb.fini();

      if (OSS_UNLIKELY(nullptr == context ||
                      INVALID_PAGE_ID == pid ||
                      mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      gpid.reset(getSpaceID(),
                 getSpaceType(),
                 FILE_TYPE_DATA_STORAGE,
                 pid);

      pageSize = getFileCluster()->getCoreArgs().pageSize;
      rc = context->getEnv()->ioBufferPool.allocate(gpid, o, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate io buffer of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      initer.initWithBuffer(gpid, pageSize, buffer, rpb);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::_getRuntimePageBufferToReset(requestContext *context,
                                                        PAGE_ID pid,
                                                        runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      _runtimePageBufferIniter initer;
      GLOBAL_PAGE_ID gpid(getSpaceID(),
                          getSpaceType(),
                          FILE_TYPE_DATA_STORAGE,
                          pid);
      UINT32 pageSize = getFileCluster()->getCoreArgs().pageSize;
      liteIOBuffer buffer;
      
      if (OSS_UNLIKELY(nullptr == context ||
                       INVALID_PAGE_ID == pid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->getEnv()->ioBufferPool.allocateToReset(gpid, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate io buffer of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      initer.initWithBuffer(gpid, pageSize, buffer, rpb);

      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      buffer.reset();
      goto done;
   }

   INT32 logicalPageSpace::_copyPageAndReinitBuffer(requestContext *context,
                                                    PAGE_SNAPSHOT_VERION psv,
                                                    PAGE_ID newPid,
                                                    runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      _runtimePageBufferIniter initer;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = getFileCluster()->getCoreArgs().pageSize;
      CHAR *buffer = nullptr;
      slice rs;
      liteIOBuffer iobuffer;

      dpsWriteReqBuilder jpad;
      dpsLogRecordHeader jres;
      dpsWriteRequest jrequest;
      IDataJournal *journal = context->getEnv()->resource.journal;

      if (OSS_UNLIKELY(nullptr == context ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       INVALID_PAGE_ID == newPid ||
                       !rpb.isValid() ||
                       !rpb.isCacheBuffer()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rs = rpb.getSlice();
      if (isPageCrashed((ossValuePtr)rs.data(), pageSize))
      {
         PD_LOG(PDERROR, "page[%s] may be crashed", rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      buffer = context->allocateBuffer(pageSize);
      ossMemcpy(buffer, rs.data(), pageSize);
      rpb.fini();

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 FILE_TYPE_DATA_STORAGE,
                 newPid);

      rc = context->getEnv()->ioBufferPool.allocateToReset(gpid, iobuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate io buffer of page[%s], rc:%d",
                gpid.toString().c_str(), rc);
         goto error;
      }

      jpad.setType(LOG_TYPE_VESSEL_COPY_PAGE);
      rc = jpad.append(DPS_LOG_PUBLIC_VESSEL_FULL_PAGE_DUMP,
                       pageSize,
                       reinterpret_cast<const void *>(iobuffer.getBufferPtr()));
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append page buffer into pad:%d", rc);
         goto error;
      }

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      ossMemcpy(iobuffer.getBufferPtr(),
                 rs.data(),
                 pageSize);
      ((pageHead *)(iobuffer.getBufferPtr()))->pid = newPid;
      ((pageHead *)(iobuffer.getBufferPtr()))->psv = psv;
      updatePageLsn((ossValuePtr)(iobuffer.getBufferPtr()), jres._lsn);
      
      iobuffer.commit(jres._lsn);
      rpb.fini();

      initer.initWithBuffer(gpid, pageSize, iobuffer, rpb);

      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         rpb.fini();
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         SDB_ASSERT(FALSE, "impossible");
         goto error;
      }
   done:
      if (nullptr != buffer)
      {
         context->releaseBuffer(buffer);
      }
      return rc;
   error:
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != jres._lsn, "to do: rollback");
      goto done;
   }

   INT32 logicalPageSpace::_initAndCreateMapping(requestContext *context,
                                                 pageInitializer *initer,
                                                 UINT32 size,
                                                 const PAGE_ID *lpids,
                                                 const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(nullptr != initer, "can not be null");
      SDB_ASSERT(0 < size, "can not be invalid");
      SDB_ASSERT(nullptr != lpids && nullptr != pids, "can not be invalid");

      runtimePageBuffer rpb;
      PAGE_SNAPSHOT_VERION psv = context->getEnv()->dms.getOnlinePageSnapshotVersion();

      for (UINT32 i = 0; i < size; ++i)
      {
         PAGE_ID lpid = lpids[i];
         SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
         PAGE_ID pid = pids[i];
         SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
         rc = _getRuntimePageBufferToReset(context, pid, rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get runtime buffer of page[%d], rc:%d",
                   pid, rc);
            goto error;
         }
         SDB_ASSERT(rpb.isWritingPrepared(), "must be prepared");

         rc = initer->initInTurns(context, i, lpid, psv, &rpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init page[%d,%d], rc:%d",
                   lpid, pid, rc);
            goto error;
         }
         rpb.fini();
      }

      rc = _lpm.setBatch(size, psv, lpids, pids);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create logical pages mapping:%d", rc);
         SDB_ASSERT(FALSE, "unexpected error");
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageSpace::_reserveLpids(UINT32 size, PAGE_ID *lpids, BOOLEAN autoExtendLPM)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(0 < size && nullptr != lpids, "can not be invalid");

      UINT32 count = 0;
      std::unique_lock<std::mutex> guard(_am);

      do
      {
         INT32 offset = _allocator.pop();
         if (0 <= offset)
         {
#if defined(_DEBUG)
            SDB_ASSERT(!_isReservedLpid(offset), "should not be reserved lpid");
#endif
            lpids[count++] = static_cast<PAGE_ID>(offset);
         }
         else if (_allocator.getUnitCount() == lpageMapping::MAX_UNIT_COUNT)
         {
            rc = SDB_VESSEL_FS_UPPER_LIMIT;
            goto error;
         }
         else
         {
            if (autoExtendLPM)
            {
               rc = _lpm.ensureUnitSpace(_allocator.getUnitCount());
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to extend lpm file:%d", rc);
                  goto error;
               }
            }
            
            rc = _allocator.extendUnitNum(1, TRUE);
            if (SDB_OK != rc)
            {
               /// no need to rollback lpm here. just wait for next reserving.
               PD_LOG(PDERROR, "failed to extend allocator:%d", rc);
               goto error;
            }
         }
      } while (count < size);

   done:
      return rc;
   error:
      for (UINT32 i = 0; i < count; ++i)
      {
         _allocator.set(lpids[i]);
      }
      goto done;
   }

   void logicalPageSpace::_freeLpids(UINT32 size, const PAGE_ID *lpids)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(0 < size && nullptr != lpids, "can not be invalid");
      std::unique_lock<std::mutex> guard(_am);
      for (UINT32 i = 0; i < size; ++i)
      {
         if (OSS_LIKELY(INVALID_PAGE_ID != lpids[i]))
         {
            BOOLEAN r = FALSE;
            _allocator.set(lpids[i], &r);
            SDB_ASSERT(!r, "unexpected bit value");
         }
      }
      return;
   }

   void logicalPageSpace::_freeLpid(PAGE_ID lpid)
   {
      _freeLpids(1, &lpid);
   }

   void logicalPageSpace::_freeLpids(const sparseBitmap32 &bm)
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      BOOLEAN quit = FALSE;
      constexpr UINT32 BATCH_SIZE = 32;
      sparseBitmap32::iterator itr;
      do
      {
         std::unique_lock<std::mutex> guard(_am);
         for (UINT32 i = 0; i < BATCH_SIZE; ++i)
         {
            BOOLEAN r = FALSE;
            if (bm.next(itr))
            {
               _allocator.set(itr.get(), &r);
               SDB_ASSERT(!r, "unexpected bit value");
            }
            else
            {
               quit = TRUE;
               break;
            }
         }
      } while (!quit);
      
      return;
   }

   INT32 logicalPageSpace::_updateUberBlockOnDisk(BOOLEAN fsync)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      lpmUberBlock *ub = nullptr;

      strictBuffer buffer;
      rc = _mfile.makeWritableBuffer(UBER_BLOCK_PID, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get writable buffer of uber block:%d", rc);
         goto error;
      }

      ub = buffer.getWritableObjPtr<lpmUberBlock>(0);
      SDB_ASSERT(nullptr != ub, "can not be invalid");
      if (_lpm.getRoot().update(ub))
      {
         ub->refillChecksum();

         if (fsync)
         {
            rc = _mfile.fsyncPage(UBER_BLOCK_PID);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to fysnc meta block page:%d", rc);
               goto error;
            }
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
