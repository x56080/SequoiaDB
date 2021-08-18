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
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   INT32 rdpScanner::open(requestContext *context,
                          const logicalPageBuffer *lpb)
   {
      INT32 rc = SDB_OK;
      const recordDataPageHead *head = NULL;
      const runtimePageBuffer *rpb = NULL;

      close();
      if (OSS_UNLIKELY(NULL == context ||
                       NULL == lpb ||
                       !lpb->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rpb = &(lpb->getRuntimeBuffer());
      rc = lpb->validatePage(PAGE_TYPE_RECORD);
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

      if (head->clLogcalID != context->getCLLid())
      {
         PD_LOG(PDERROR, "logical id does not match[%d,%d]",
                head->clLogcalID, context->getCLLid());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      _lpb = lpb;
      _head = head;
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void rdpScanner::close()
   {
      _lpb = NULL;
      _head = NULL;
   }

   UINT32 rdpScanner::getTotalSlotCount()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      return NULL == _head ? 0 : _head->totalSlotCount;
   }

   INT32 rdpScanner::getSlot(UINT32 pos, recordSlot &rs)const
   {
      INT32 rc = SDB_OK;
      const recordSlot *slot = NULL;
      UINT32 offset = 0;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if ((UINT32)(_head->totalSlotCount) <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      offset = RECORD_PAGE_HEAD_LEN + pos * RDP_RSLOT_SIZE;
      slot = _lpb->getRuntimeBuffer().getReadablePtrOfBody<recordSlot>(offset);
      if (NULL == slot)
      {
         PD_LOG(PDERROR, "failed to get readable ptr of slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rs = *slot;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpScanner::getNormalRecordHeadAndBody(UINT32 pos,
                                                recordHead &rh,
                                                slice &bodySlice)const
   {
      INT32 rc = SDB_OK;
      const recordSlot *slot = NULL;
      const recordHead *head = NULL;
      ossValuePtr recordBody = 0;
      UINT32 offset = 0;
      bodySlice.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if ((UINT32)(_head->totalSlotCount) <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      offset = RECORD_PAGE_HEAD_LEN + pos * RDP_RSLOT_SIZE;
      slot = _lpb->getRuntimeBuffer().getReadablePtrOfBody<recordSlot>(offset);
      if (NULL == slot)
      {
         PD_LOG(PDERROR, "failed to get readable ptr of slot[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (!slot->isValidAndVisible())
      {
         PD_LOG(PDERROR, "can not get invisible record at slot[%d]", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (!slot->isNormalRecordHead())
      {
         PD_LOG(PDERROR, "can not read slot[%d] as normal record head", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      head = _lpb->getRuntimeBuffer().getReadablePtrOfBody<recordHead>(slot->getOffset());
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get record head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      SDB_ASSERT(!head->isDependent(), "impossible");

      if (RDP_RECORD_HEAD_LEN < head->getSize())
      {
         rc = _lpb->getRuntimeBuffer().
               getReadablePtrOfBodyWithRc(slot->getOffset() + RDP_RECORD_HEAD_LEN,
                                          head->getSize() - RDP_RECORD_HEAD_LEN,
                                          recordBody);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get readble record body ptr:%d", rc);
            goto error;
         }

         bodySlice.reset(head->getSize() - RDP_RECORD_HEAD_LEN,
                           (const CHAR *)recordBody);
      }

      rh = *head;
   done:
      return rc;
   error:
      goto done;
   }

   const recordDataPageHead &rdpScanner::getPageHead()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      return *_head;
   }
}//namesapce vessel
}//namespace engine