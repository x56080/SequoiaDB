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

   Source File Name = lpageMapping.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpageMapping.h"
#include "pdTrace.hpp"
#include "vessel/strictBuffer.h"

namespace engine
{
namespace vessel
{
   /// we use pos << 3 instead of pos * 8
   static_assert(8 == sizeof(lpageDescriptor), "must be 8");

   lpageMapping::lpageMapping()
   {
      ossMemset(_entries, 0xFF, sizeof(_entries));
   }

   INT32 lpageMapping::init(lpageMetaDataFile *mfile,
                            lpmUberBlock *ub)
   {
      INT32 rc = SDB_OK;

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
      _lub = ub;
      ossMemcpy(_entries, ub->mappingEntries, sizeof(_entries));
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void lpageMapping::fini()
   {
      _mfile = nullptr;
      _lub = nullptr;
      ossMemset(_entries, 0xFF, sizeof(_entries));
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

         rc = _getDescriptorPage(unitId, pid);
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
            rc = _getDescriptorPage(unitId, pid);
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
         _getDescriptorPage(unitId, pid);
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

      rc = _getDescriptorPage(_getUnitId(lpid), pid);
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
            rc = _getDescriptorPage(unitId, pid);
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
         _getDescriptorPage(unitId, pid);
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

   INT32 lpageMapping::_getDescriptorPage(UINT32 unitId, PAGE_ID &pid)
   {
      INT32 rc = SDB_OK;
      UINT32 posInEntry = 0;
      UINT32 entryPos = _getEntryPosByUnitId(unitId, posInEntry);
      SDB_ASSERT(entryPos < lpmUberBlock::MAPPING_ENTRY_SIZE, "impossible");
      PAGE_ID entryPid = _entries[entryPos];
      strictBuffer buffer;

      if (INVALID_PAGE_ID == entryPid)
      {
         goto done;
      }

      rc = _mfile->makeReadableBuffer(entryPid, buffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to make buffer of pid[%d], rc:%d", entryPid, rc);
         goto error;
      }

      pid = *(buffer.getReadableObjPtr<PAGE_ID>(posInEntry << 2));
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
      PAGE_ID entryPid = _entries[entryPos];
      PAGE_ID *descPidPtr = nullptr;
      strictBuffer buffer;
      PAGE_ID newPid = INVALID_PAGE_ID;

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
      SDB_ASSERT(INVALID_PAGE_ID == _entries[pos], "do not reinit");
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
      _lub->mappingEntries[pos] = pid;
      _entries[pos] = pid;
      
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

      rc = _getDescriptorPage(unitId, pid);
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
} // namespace vessel

} // namespace engine
