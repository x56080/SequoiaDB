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
#include "vessel/indexMappingPage.h"
#include "vessel/indexMappingPageAccessor.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   static const UINT32 TOTAL_DIRECT_MAPPED_IMP = 65536 * DIRECT_MAPPING_INDEX_COUNT_PER_CL / ID_MAP_PAGE_CAPACITY;

   UINT32 indexSpace::getReservedImpCount()const
   {
      return TOTAL_DIRECT_MAPPED_IMP + 1;
   }

   PAGE_ID indexSpace::getDirectMappedIndexLpid(CL_MB_ID mbID, INT32 slot)const
   {
      PAGE_ID lpid = INVALID_PAGE_ID;
      if (INVALID_CL_MB_ID != mbID &&
          isValidIndexSlot(slot))
      {
         lpid = mbID * DIRECT_MAPPING_INDEX_COUNT_PER_CL + slot;
      }
      return lpid;
   }

   PAGE_ID indexSpace::getMappingPageLpid(CL_MB_ID mbID,
                                          INT32 slot,
                                          UINT32 &pos)const
   {
      PAGE_ID lpid = INVALID_PAGE_ID;
      static constexpr UINT32 _BEGIN_LPID = TOTAL_DIRECT_MAPPED_IMP *
                                            ID_MAP_PAGE_CAPACITY;

      if (INVALID_CL_MB_ID != mbID &&
          isValidIndexSlot(slot) &&
          (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL <= slot)
      {
         UINT32 globalPos = ((MAX_INDEX_COUNT_PER_CL - DIRECT_MAPPING_INDEX_COUNT_PER_CL)
                             * mbID + slot - DIRECT_MAPPING_INDEX_COUNT_PER_CL);
         UINT32 pageSize = _storage.getCoreArgs().pageSize;
         UINT32 capacity = getIndexMappingPageCapacity(pageSize);
         if (0 == capacity)
         {
            goto done;
         }
         lpid = (globalPos / capacity) + _BEGIN_LPID;
         pos = globalPos % capacity;
         SDB_ASSERT(lpid < (getReservedImpCount() * ID_MAP_PAGE_CAPACITY), "impossible");
      }

   done:
      return lpid;
   }

   INT32 indexSpace::getIndexDefPage(requestContext *context,
                                     CL_MB_ID mbID,
                                     INT32 slot,
                                     PAGE_ID &lpid)
   {
      INT32 rc = SDB_OK;
      logicalPageBuffer lpb;
      
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_CL_MB_ID == mbID ||
                            !isValidIndexSlot(slot)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (slot < (INT32)DIRECT_MAPPING_INDEX_COUNT_PER_CL)
      {
         lpid = getDirectMappedIndexLpid(mbID, slot);
      }
      else
      {
         indexMappingPageAccessor accessor;
         PAGE_ID mappingPageLpid = INVALID_PAGE_ID;
         UINT32 pos = -1;
         mappingPageLpid = getMappingPageLpid(mbID, slot, pos);
         if (INVALID_PAGE_ID == mappingPageLpid)
         {
            PD_LOG(PDERROR, "failed to get mapping page of [%d,%d]",
                   mbID, slot);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = getLogicalPageBuffer(context, mappingPageLpid,
                                   ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM_SHARED),
                                   lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get page[%d] buffer:%d",
                   mappingPageLpid, rc);
            goto error;
         }

         rc = accessor.getIndexDefPage(context, pos, lpb, lpid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get index def page:%d", rc);
            goto error;
         }
         
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::getMinUncompletedLSN(requestContext *context,
                                          DPS_LSN_OFFSET &lsn)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      /// getMinUncompletedLSN is very expensive.
      lsn = context->getOuterResource()->getMinUncompletedLSN();
      return SDB_OK;
   }

   INT32 indexSpace::getRuntimePageBuffer(requestContext *context,
                                              PAGE_ID pid,
                                              const ossSharedLatchMode &mode,
                                              runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      dataPageCluster *dpc = NULL;
      logicalPageSpace::_runtimePageBufferIniter initer;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = 0;
      mmapPagePointer ptr;

      if (OSS_UNLIKELY(NULL == context ||
                      INVALID_PAGE_ID == pid ||
                      mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      dpc = getDataStorageObj();
      SDB_ASSERT(NULL != dpc, "can not be null");

      rc = dpc->getDataPagePtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 getStorageFileType(),
                 pid);
      pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;

      rc = initer.initWithMmap(gpid, pageSize, ptr, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::getRuntimePageBufferToReset(requestContext *context,
                                                     PAGE_ID pid,
                                                     runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      rc = this->getRuntimePageBuffer(context, pid,
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

   INT32 indexSpace::copyPageAndReinitBuffer(requestContext *context,
                                                 PAGE_SNAPSHOT_VERION psv,
                                                 PAGE_ID newPid,
                                                 runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = logicalPageSpace::getStorageCoreArgs().pageSize;
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

      rc = getDataStorageObj()->getDataPagePtr(newPid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get new page[%d] ptr:%d", newPid, rc);
         goto error;
      }

      ossMemcpy((void *)(ptr.get()), rs.data(), pageSize);
      ((pageHead *)(ptr.get()))->pid = newPid;
      ((pageHead *)(ptr.get()))->psv = psv;

      rpb.fini();

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 getStorageFileType(),
                 newPid);

      rc = initer.initWithMmap(gpid, pageSize, ptr, rpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rpb:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::_create(requestContext *context)
   {
      return SDB_OK;
   }

   INT32 indexSpace::_open(requestContext *context,
                           const storageFileLoader &loader)
   {
      return SDB_OK;
   }
   
   void indexSpace::_close()
   {
      _storage.close();
   }
   
   void indexSpace::_destroy(requestContext *context)
   {
      _storage.destroy();
   }

   dataPageCluster::options indexSpace::getStorageOptions()const
   {
      dataPageCluster::options o;
      o.segmentReusedMinFreePercent = 0.2f;
      return o;
   }
}//namespace vessel
}//namespace engine