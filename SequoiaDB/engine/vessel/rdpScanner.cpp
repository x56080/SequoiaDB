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

   Source File Name = rdpScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rdpScanner.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/scanCLContext.h"
#include "vessel/scanCLCursor.h"

namespace engine
{
namespace vessel
{
   INT32 rdpScanner::getMore(scanCLContext *context,
                              const logicalPageBuffer *lpb,
                              scanCLCursor *cursor)const
   {
      INT32 rc = SDB_OK;
      RECORD_SLOT_ID slotId = INVALID_RECORD_SLOT_ID;
      const recordDataPageHead *head = NULL;
      const runtimePageBuffer *rpb = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == lpb ||
                       !lpb->isValid() ||
                       NULL == cursor ||
                       !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      rc = validatePage((ossValuePtr)(rpb->getReadOnlyBuffer()),
                         PAGE_TYPE_RECORD, rpb->getPageSize(),
                         rpb->getGlobalPid().page(),
                         lpb->getLogicalPid(),
                         lpb->getCowTrigger().getPsv());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                rpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = rpb->getReadablePtrOfBody<recordDataPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "faile to get readable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (head->clLogcalID != cursor->getHandle().getCLLId())
      {
         PD_LOG(PDERROR, "logical id does not match[%d,%d]",
                head->clLogcalID, cursor->getHandle().getCLLId());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }
      else if (head->pageSeq != cursor->getPageSeq())
      {
         PD_LOG(PDERROR, "page sequence does not match[%d,%d]",
                head->pageSeq, cursor->getPageSeq());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      slotId = INVALID_RECORD_SLOT_ID == cursor->getSlotID() ?
               0 : cursor->getSlotID() + 1;

      for (RECORD_SLOT_ID i = slotId; i < head->totalSlotCount; ++i)
      {
         UINT32 offset = RECORD_PAGE_HEAD_LEN + (i * RDP_RSLOT_SIZE);
         const recordSlot *slotPtr = rpb->getReadablePtrOfBody<recordSlot>(offset);
         if (NULL == slotPtr)
         {
            PD_LOG(PDERROR, "failed to get readble slot ptr[%d]", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         if (!slotPtr->isValid() || slotPtr->isInvisible())
         {
            cursor->setSlot(i);
            continue;
         }

         if (slotPtr->getType() == RDP_SLOT_TYPE_NORMAL)
         {
            rc = getNormalRecord(context, *slotPtr, rpb, cursor);
            if (SDB_OK != rc)
            {
               goto error;
            }
            cursor->setSlot(i);
         }
         else
         {
            SDB_ASSERT(FALSE, "TODO");
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpScanner::getRecourdCountInHead(requestContext *context,
                                           const logicalPageBuffer *lpb,
                                           UINT32 &count)const
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = NULL;
      const runtimePageBuffer *rpb = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      rc = validatePage((ossValuePtr)(rpb->getReadOnlyBuffer()),
                         PAGE_TYPE_RECORD, rpb->getPageSize(),
                         rpb->getGlobalPid().page(),
                         lpb->getLogicalPid(),
                         lpb->getCowTrigger().getPsv());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                rpb->getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = rpb->getReadablePtrOfBody<recordDataPageHead>(0);
      if (NULL == head)
      {
         PD_LOG(PDERROR, "faile to get readable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (NULL == head)
      {
         PD_LOG(PDERROR, "faile to get readable head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (head->clLogcalID != context->getCLLid())
      {
         PD_LOG(PDERROR, "logical id does not match[%d,%d]",
                head->clLogcalID, context->getCLLid());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      count = head->recordCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpScanner::getNormalRecord(scanCLContext *context,
                                     const recordSlot &slot,
                                     const runtimePageBuffer *rpb,
                                     scanCLCursor *cursor)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(slot.isValid(), "can not be invalid");
      SDB_ASSERT(RDP_SLOT_TYPE_NORMAL == slot.getType(), "must be normal");
      SDB_ASSERT(NULL != rpb, "can not be null");
      SDB_ASSERT(NULL != cursor, "can not be null");

      ossValuePtr ptr = 0;
      const recordHead *rh = rpb->getReadablePtrOfBody<recordHead>(slot.getOffset());
      if (NULL == rh)
      {
         PD_LOG(PDERROR, "failed to get ptr of record head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(rh->getSize() <= RDP_RECORD_HEAD_LEN))
      {
         PD_LOG(PDERROR, "invalid record size[%d]", rh->getSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(rh->isDependent() || rh->isTombstone()))
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (rh->isOverflow())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      else if (rh->isCompressed())
      {
         SDB_ASSERT(FALSE, "TODO");
      }

      rc = rpb->getReadablePtrOfBodyWithRc(slot.getOffset(), rh->getSize(), ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get ptr of record[%d,%d], rc:%d",
                slot.getOffset(), rh->getSize(), rc);
         goto error;
      }

      rc = cursor->push(rh->getSize() - RDP_RECORD_HEAD_LEN,
                        (const CHAR *)(ptr + RDP_RECORD_HEAD_LEN));
      if (SDB_OK == rc)
      {
         /// do nothing.
      }
      else if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
      {
         goto error;
      }
      else
      {
         PD_LOG(PDERROR, "failed to push data to cursor:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namesapce vessel
}//namespace engine