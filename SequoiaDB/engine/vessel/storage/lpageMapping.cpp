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

   Source File Name = lpageMapping.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lpageMapping.h"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   /// we use pos << 3 instead of pos * 8
   static_assert(8 == sizeof(lpageDescriptor), "must be 8");

   const UINT32 lpageMapping::_UNIT_SIZE_SQUARE = std::log2(lpageMapping::LPID_UNIT_SIZE);
   const UINT32 lpageMapping::_MAPPING_CAPACITY_SQUARE =
                     std::log2(lpageMapping::MAPPING_ENTRY_CAPACITY);

   INT32 lpageMapping::init(lpageMetaDataFile *mfile,
                            const lpmUberBlock *ub)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;

      fini();
      if (OSS_UNLIKELY(nullptr == mfile ||
                       !mfile->isOpen() ||
                       nullptr == ub ||
                       !ub->isVaild()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _mfile = mfile;
      _root.init(ub);

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void lpageMapping::fini()
   {
      _mfile = nullptr;
      _root.reset();
      return;
   }

   INT32 lpageMapping::ensureUnitSpace(UINT32 unitId)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(MAX_UNIT_COUNT <= unitId))
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }

      rc = _ensureDescriptorPage(unitId, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure descriptor page:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::set(PAGE_ID lpid,
                           const lpageDescriptor &desc,
                           lpageDescriptor *oldVal)
   {
      INT32 rc = SDB_OK;
      _reset(1, oldVal);

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                            !desc.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isOutOfMaxBound(lpid)))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else
      {
         UINT32 unitId = _getUnitId(lpid);
         PAGE_ID pid = INVALID_PAGE_ID;
         strictBuffer buffer;
         UINT32 pos = _getDescPos(lpid);
         lpageDescriptor *descPtr = nullptr;

         rc = _getDescriptorPage(nullptr, unitId, pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get pid of desc page[%d], rc:%d",
                     unitId, rc);
            goto error;
         }
         else if (INVALID_PAGE_ID == pid)
         {
            PD_LOG(PDERROR, "desc page of lpid[%d] not created yet", lpid);
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }

         rc = _mfile->makeWritableBuffer(pid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer of pid[%d], rc:%d", pid, rc);
            goto error;
         }

         descPtr = buffer.getWritableObjPtr<lpageDescriptor>(pos << 3);
         if (nullptr != oldVal)
         {
            *oldVal = *descPtr;
         }
         *descPtr = desc;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::setBatch(UINT32 size,
                                PAGE_SNAPSHOT_VERION psv,
                                const PAGE_ID *lpids,
                                const PAGE_ID *pids,
                                lpageDescriptor *oldVals)
   {
      INT32 rc = SDB_OK;
      _reset(size, oldVals);

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == size ||
                            INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                            nullptr == lpids ||
                            nullptr == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < size; ++i)
      {
         if (INVALID_PAGE_ID == lpids[i] ||
             INVALID_PAGE_ID == pids[i])
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (isOutOfMaxBound(lpids[i]))
         {
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }
         else
         {
            UINT32 unitId = _getUnitId(lpids[i]);
            PAGE_ID pid = INVALID_PAGE_ID;
            rc = _getDescriptorPage(nullptr, unitId, pid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get pid of desc page[%d], rc:%d",
                      unitId, rc);
               goto error;
            }
            else if (INVALID_PAGE_ID == pid)
            {
               PD_LOG(PDERROR, "desc page of lpid[%d] not created yet", lpids[i]);
               rc = SDB_OUT_OF_BOUND;
               goto error;
            }
            else if (0 == _mfile->getPagePtr(pid))
            {
               PD_LOG(PDERROR, "desc page[%d] not accessable", pid);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
         }
      }//(UINT32 i = 0; i < size; ++i)

      

      for (UINT32 i = 0; i < size; ++i)
      {
         UINT32 unitId = _getUnitId(lpids[i]);
         PAGE_ID pid = INVALID_PAGE_ID;
         _getDescriptorPage(nullptr, unitId, pid);
         SDB_ASSERT(INVALID_PAGE_ID != pid, "should not be invalid");
         strictBuffer buffer;
         lpageDescriptor *desc = nullptr;
         UINT32 pos = _getDescPos(lpids[i]);

         rc = _mfile->makeWritableBuffer(pid, buffer);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            SDB_ASSERT(FALSE, "should not be failed");
            goto error;
         }

         desc = buffer.getWritableObjPtr<lpageDescriptor>(pos << 3);
         if (nullptr != oldVals)
         {
            oldVals[i] = *desc;
         }

         *desc = lpageDescriptor(pids[i], psv);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::get(PAGE_ID lpid, lpageDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      strictBuffer buffer;
      const lpageDescriptor *descPtr = nullptr;
      desc.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isOutOfMaxBound(lpid)))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = _getDescriptorPage(nullptr, _getUnitId(lpid), pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get desc page:%d", rc);
         goto error;
      }

      if (INVALID_PAGE_ID == pid)
      {
         rc = SDB_OK;
         goto error;
      }

      rc = _mfile->makeReadableBuffer(pid, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d] ptr, rc:%d", pid, rc);
         goto error;
      }

      descPtr = buffer.getReadableObjPtr<lpageDescriptor>(_getDescPos(lpid) << 3);
      SDB_ASSERT(nullptr != descPtr, "impossible");
      desc = *descPtr;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::reset(PAGE_ID lpid, lpageDescriptor *oldVal)
   {
      return resetBatch(1, &lpid, oldVal);
   }

   INT32 lpageMapping::resetBatch(UINT32 size,
                                  const PAGE_ID *lpids,
                                  lpageDescriptor *oldVals)
   {
      INT32 rc = SDB_OK;

      _reset(size, oldVals);

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(0 == size ||
                            nullptr == lpids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < size; ++i)
      {
         if (INVALID_PAGE_ID == lpids[i])
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
         else if (isOutOfMaxBound(lpids[i]))
         {
            rc = SDB_OUT_OF_BOUND;
            goto error;
         }
         else
         {
            UINT32 unitId = _getUnitId(lpids[i]);
            PAGE_ID pid = INVALID_PAGE_ID;
            rc = _getDescriptorPage(nullptr, unitId, pid);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get pid of desc page[%d], rc:%d",
                      unitId, rc);
               goto error;
            }
            else if (INVALID_PAGE_ID == pid)
            {
               PD_LOG(PDERROR, "desc page of lpid[%d] not created yet", lpids[i]);
               rc = SDB_OUT_OF_BOUND;
               goto error;
            }
            else if (0 == _mfile->getPagePtr(pid))
            {
               PD_LOG(PDERROR, "desc page[%d] not accessable", pid);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }
         }
      }//(UINT32 i = 0; i < size; ++i)

      static_assert(8 == sizeof(lpageDescriptor), "must be 8");

      for (UINT32 i = 0; i < size; ++i)
      {
         UINT32 unitId = _getUnitId(lpids[i]);
         PAGE_ID pid = INVALID_PAGE_ID;
         _getDescriptorPage(nullptr, unitId, pid);
         SDB_ASSERT(INVALID_PAGE_ID != pid, "should not be invalid");
         strictBuffer buffer;
         lpageDescriptor *desc = nullptr;
         UINT32 pos = _getDescPos(lpids[i]);

         rc = _mfile->makeWritableBuffer(pid, buffer);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            SDB_ASSERT(FALSE, "should not be failed");
            goto error;
         }

         desc = buffer.getWritableObjPtr<lpageDescriptor>(pos << 3);
         if (nullptr != oldVals)
         {
            oldVals[i] = *desc;
         }
         if (desc->isValid())
         {
            desc->reset();
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::_getDescriptorPage(const lpageMappingRoot *pte,
                                          UINT32 unitId,
                                          PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      UINT32 posInEntry = 0;
      UINT32 entryPos = _getEntryPosByUnitId(unitId, posInEntry);
      SDB_ASSERT(entryPos < lpmUberBlock::MAPPING_ENTRY_SIZE, "impossible");
      strictBuffer buffer;
      PAGE_ID entryPid = (nullptr != pte && INVALID_PAGE_ID != pte->get(entryPos)) ?
                         pte->get(entryPos) : _root.get(entryPos);
      pid = INVALID_PAGE_ID;

      if (INVALID_PAGE_ID != entryPid)
      {
         rc = _mfile->makeReadableBuffer(entryPid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make buffer of pid[%d], rc:%d", entryPid, rc);
            goto error;
         }

         pid = *(buffer.getReadableObjPtr<PAGE_ID>(posInEntry << 2));
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::_ensureDescriptorPage(UINT32 unitId, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      
      UINT32 posInEntry = 0;
      UINT32 entryPos = _getEntryPosByUnitId(unitId, posInEntry);
      SDB_ASSERT(entryPos < lpmUberBlock::MAPPING_ENTRY_SIZE, "impossible");
      PAGE_ID entryPid = _root.get(entryPos);
      PAGE_ID *descPidPtr = nullptr;
      strictBuffer buffer;
      PAGE_ID newPid = INVALID_PAGE_ID;

      pid = INVALID_PAGE_ID;

      if (OSS_UNLIKELY(INVALID_PAGE_ID == entryPid))
      {
         rc = _createEntry(entryPos, entryPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure entry[%d] pid:%d", entryPos, rc);
            goto error;
         }
      }

      rc = _mfile->makeWritableBuffer(entryPid, buffer);
      if (OSS_UNLIKELY(SDB_OK != rc))
      {
         PD_LOG(PDERROR, "failed to get entry page[%d] ptr, rc:%d", entryPid, rc);
         goto error;
      }

      static_assert(4 == sizeof(PAGE_ID), "must be 4");
      descPidPtr = buffer.getWritableObjPtr<PAGE_ID>(posInEntry << 2);
      SDB_ASSERT(nullptr != descPidPtr, "impossible");
      if (INVALID_PAGE_ID == *descPidPtr)
      {
         mmapPagePointer ptr;
         rc = _mfile->reservePid(newPid, &ptr);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve new pid:%d", rc);
            goto error;
         }

         ossMemset(ptr.getBuf(), 0x00, lpageMetaDataFile::PAGE_SIZE);
         *descPidPtr = newPid;
      }

      pid = *descPidPtr;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::_createEntry(UINT32 pos, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(pos < lpmUberBlock::MAPPING_ENTRY_SIZE, "out of bound");
      SDB_ASSERT(INVALID_PAGE_ID == _root.get(pos), "do not recreate");
      pid = INVALID_PAGE_ID;
      
      mmapPagePointer ptr;
      strictBuffer b;

      rc = _mfile->reservePid(pid, &ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve pid:%d", rc);
         goto error;
      }

      b.makeWritable(lpageMetaDataFile::PAGE_SIZE, ptr.getBuf());
      b.setBuffer(0xFF);
      _root.set(pos, pid);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::dumpUnitSme(UINT32 unitId,
                                   BOOLEAN &exists,
                                   fixedBitset<LPID_UNIT_SIZE> &bs)
   {
      INT32 rc = SDB_OK;
      PAGE_ID pid = INVALID_PAGE_ID;
      strictBuffer buffer;

      exists = FALSE;
      bs.clearAll();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(MAX_UNIT_COUNT <= unitId))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      rc = _getDescriptorPage(nullptr, unitId, pid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get desc page:%d", rc);
         goto error;
      }

      if (INVALID_PAGE_ID == pid)
      {
         goto done;
      }

      rc = _mfile->makeReadableBuffer(pid, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make buffer of pid[%d], rc:%d",
                pid, rc);
         goto error;
      }

      exists = TRUE;
      for (UINT32 i = 0; i < LPID_UNIT_SIZE; ++i)
      {
         const lpageDescriptor *desc = buffer.getReadableObjPtr<lpageDescriptor>(i << 3);
         if (!desc->isValid())
         {
            bs.set(i);
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lpageMapping::_reset(UINT32 size, lpageDescriptor *descriptors)
   {
      if (nullptr != descriptors)
      {
         for (UINT32 i = 0; i < size; ++i)
         {
            descriptors[i].reset();
         }
      }
      return;
   }

   INT32 lpageMapping::set(lpageMappingPteCtx &ctx,
                           PAGE_ID lpid,
                           const lpageDescriptor &desc)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                            !desc.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isOutOfMaxBound(lpid)))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else
      {
         lpageDescriptor *descPtr = nullptr;
         PAGE_ID descPid = INVALID_PAGE_ID;
         strictBuffer buffer;

         rc = _ensurePrivatePath(ctx, _getUnitId(lpid), descPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure private path:%d", rc);
            goto error;
         }

         rc = _mfile->makeWritableBuffer(descPid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make writable buffer of pid[%d], rc:%d", descPid, rc);
            goto error;
         }

         static_assert(8 == sizeof(lpageDescriptor), "must be 8");
         descPtr = buffer.getWritableObjPtr<lpageDescriptor>(_getDescPos(lpid) << 3);
         *descPtr = desc;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::setBatch(lpageMappingPteCtx &ctx,
                                PAGE_SNAPSHOT_VERION psv,
                                UINT32 size,
                                const PAGE_ID *lpids,
                                const PAGE_ID *pids)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                            0 == size ||
                            nullptr == lpids ||
                            nullptr == pids))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (UINT32 i = 0; i < size; ++i)
      {
         PAGE_ID descPid = INVALID_PAGE_ID;

         if (INVALID_PAGE_ID == lpids[i] ||
             isOutOfMaxBound(lpids[i]) ||
             INVALID_PAGE_ID == pids[i])
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         rc = _ensurePrivatePath(ctx, _getUnitId(lpids[i]), descPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure private path:%d", rc);
            goto error;
         }
      }

      for (UINT32 i = 0; i < size; ++i)
      {
         PAGE_ID lpid = lpids[i];
         lpageDescriptor desc(pids[i], psv);
         PAGE_ID descPid = INVALID_PAGE_ID;
         strictBuffer buffer;
         lpageDescriptor *descPtr = nullptr;

         rc = _ensurePrivatePath(ctx, _getUnitId(lpid), descPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure private path:%d", rc);
            goto error;
         }

         rc = _mfile->makeWritableBuffer(descPid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make writable buffer of pid[%d], rc:%d", descPid, rc);
            goto error;
         }

         static_assert(8 == sizeof(lpageDescriptor), "must be 8");
         
         descPtr = buffer.getWritableObjPtr<lpageDescriptor>(_getDescPos(lpid) << 3);
         SDB_ASSERT(nullptr != descPtr, "can not be invalid");
         SDB_ASSERT(INVALID_PAGE_ID == descPtr->pid, "reset it first");
         *descPtr = desc;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpageMapping::getPtePrior(const lpageMappingPteCtx &ctx,
                                   PAGE_ID lpid,
                                   lpageDescriptor &desc)
   {
      INT32 rc = SDB_OK;
      desc.reset();

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isOutOfMaxBound(lpid)))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else
      {
         const lpageDescriptor *descPtr = nullptr;
         strictBuffer buffer;
         PAGE_ID pid = INVALID_PAGE_ID;
         rc = _getDescriptorPage(&(ctx._root), _getUnitId(lpid), pid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get desc page:%d", rc);
            goto error;
         }

         if (INVALID_PAGE_ID != pid)
         {
            rc = _mfile->makeReadableBuffer(pid, buffer);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get page[%d] ptr, rc:%d", pid, rc);
               goto error;
            }

            descPtr = buffer.getReadableObjPtr<lpageDescriptor>(_getDescPos(lpid) << 3);
            SDB_ASSERT(nullptr != descPtr, "impossible");
            desc = *descPtr;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }
   
   INT32 lpageMapping::reset(lpageMappingPteCtx &ctx,
                             PAGE_ID lpid,
                             lpageDescriptor *oldVal)
   {
      INT32 rc = SDB_OK;

      if (nullptr != oldVal)
      {
         oldVal->reset();
      }

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isOutOfMaxBound(lpid)))
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }
      else
      {
         lpageDescriptor *descPtr = nullptr;
         PAGE_ID descPid = INVALID_PAGE_ID;
         strictBuffer buffer;

         rc = _ensurePrivatePath(ctx, _getUnitId(lpid), descPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to ensure private path:%d", rc);
            goto error;
         }

         rc = _mfile->makeWritableBuffer(descPid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make writable buffer of pid[%d], rc:%d", descPid, rc);
            goto error;
         }

         static_assert(8 == sizeof(lpageDescriptor), "must be 8");
         descPtr = buffer.getWritableObjPtr<lpageDescriptor>(_getDescPos(lpid) << 3);
         SDB_ASSERT(nullptr != descPtr, "impossible");
         if (nullptr != oldVal)
         {
            *oldVal = *descPtr;
         }
         descPtr->reset();
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lpageMapping::publish(lpageMappingPteCtx &ctx)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      PD_LOG(PDDEBUG, "new/obsolete meta file pages[%d:%d]",
             ctx._brandNewSet.size(), ctx._obsoleteSet.size());
      _root.merge(ctx._root);

      return;
   }

   void lpageMapping::freeOboleteSetAfterPublish(lpageMappingPteCtx &ctx)
   {
      for (auto itr = ctx._obsoleteSet.cbegin();
            itr != ctx._obsoleteSet.cend(); ++itr)
      {
         _mfile->freePid(*itr);
      }
      return;
   }

   void lpageMapping::abort(lpageMappingPteCtx &ctx)
   {
      for (auto itr = ctx._brandNewSet.cbegin();
           itr != ctx._brandNewSet.cend(); ++itr)
      {
         _mfile->freePid(*itr);
      }
      ctx.reset();
      return;
   }

   INT32 lpageMapping::_ensurePrivatePath(lpageMappingPteCtx &ctx,
                                          UINT32 unitId,
                                          PAGE_ID &descPid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");

      BOOLEAN newDescriptor = FALSE;
      PAGE_ID descriptorPid = INVALID_PAGE_ID;
      UINT32 posInEntry = 0;
      UINT32 entryPos = _getEntryPosByUnitId(unitId, posInEntry);
      PAGE_ID rootEntry = INVALID_PAGE_ID;
      strictBuffer mappingBuffer;

      descPid = INVALID_PAGE_ID;

      if (INVALID_PAGE_ID == ctx._root.get(entryPos))
      {
         rc = _ensurePrivateRootEntry(entryPos, ctx);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to create private root:%d", rc);
            goto error;
         }
      }
      
      rootEntry = ctx._root.get(entryPos);
      SDB_ASSERT(INVALID_PAGE_ID != rootEntry, "can not be invalid");
      rc = _mfile->makeWritableBuffer(rootEntry, mappingBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make pid[%] readable:%d", rootEntry, rc);
         goto error;
      }

      descriptorPid = *(mappingBuffer.getReadableObjPtr<PAGE_ID>(posInEntry << 2));
      if (INVALID_PAGE_ID != descriptorPid &&
          ctx.isBrandNewPid(descriptorPid))
      {
         descPid = descriptorPid;
      }
      else
      {
         strictBuffer buffer;
         PAGE_ID publicDescriptorPid = INVALID_PAGE_ID;
         std::unique_lock<std::mutex> guard(ctx._pathLock);

         /// reget pid from mmap ptr after locking
         descriptorPid = *(mappingBuffer.getReadableObjPtr<PAGE_ID>(posInEntry << 2));
         if (INVALID_PAGE_ID != descriptorPid &&
             ctx.isBrandNewPid(descriptorPid, FALSE))
         {
            descPid = descriptorPid;
            goto done;
         }
         
         rc = _mfile->reservePid(descriptorPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reserve new pid:%d", rc);
            goto error;
         }

         newDescriptor = TRUE;

         rc = _mfile->makeWritableBuffer(descriptorPid, buffer);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to make pid[%d] writable:%d", descriptorPid, rc);
            goto error;
         }

         rc = _getDescriptorPage(nullptr, unitId, publicDescriptorPid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get descriptor page[%d], rc:%d", unitId, rc);
            goto error;
         }

         if (INVALID_PAGE_ID == publicDescriptorPid)
         {
            buffer.setBuffer(0x00);
         }
         else
         {
            ossValuePtr ptr = _mfile->getPagePtr(publicDescriptorPid);
            SDB_ASSERT(0 != ptr, "impossible");
            buffer.write(0, buffer.getSize(), (const void *)ptr);
            ctx._obsoleteSet.insert(publicDescriptorPid);
         }

         *(mappingBuffer.getWritableObjPtr<PAGE_ID>(posInEntry << 2)) = descriptorPid;
         ctx._brandNewSet.insert(descriptorPid);
         descPid = descriptorPid;
      }//if (INVALID_PAGE_ID == descriptorPid)
   done:
      return rc;
   error:
      if (newDescriptor)
      {
         _mfile->freePid(descriptorPid);
      }
      goto done;
   }

   INT32 lpageMapping::_ensurePrivateRootEntry(UINT32 pos, lpageMappingPteCtx &ctx)
   {
      INT32 rc = SDB_OK;
      strictBuffer buffer;
      PAGE_ID reservedPid = INVALID_PAGE_ID;

      std::unique_lock<std::mutex> guard(ctx._pathLock);
      
      if (INVALID_PAGE_ID != ctx._root.get(pos))
      {
         goto done;
      }

      rc = _mfile->reservePid(reservedPid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve root entry pid:%d", rc);
         goto error;
      }

      rc = _mfile->makeWritableBuffer(reservedPid, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make pid[%d] writable:%d", reservedPid, rc);
         goto error;
      }

      /// copy data or init it.
      if (INVALID_PAGE_ID == _root.get(pos))
      {
         buffer.setBuffer(0xFF);
      }
      else
      {
         ossValuePtr ptr = _mfile->getPagePtr(_root.get(pos));
         SDB_ASSERT(0 != ptr, "impossible");
         buffer.write(0, buffer.getSize(), (const void *)ptr);
         ctx._obsoleteSet.insert(_root.get(pos));
      }

      ctx._root.set(pos, reservedPid); 
      ctx._brandNewSet.insert(reservedPid);  
   done:
      return rc;
   error:
      if (INVALID_PAGE_ID != reservedPid)
      {
         _mfile->freePid(reservedPid);
      }
      goto done;
   }
} // namespace vessel

} // namespace engine
 