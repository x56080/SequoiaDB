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

   Source File Name = rdpReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rdpReader.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"
#include "vessel/logicalPageSpace.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   INT32 rdpReader::open(requestContext *context,
                          PAGE_ID lpid,
                          const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      lpsObject lps;
      const recordDataPageHead *head = NULL;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isSpaceIdLocked() ||
                       !context->getGlobalCollectionId().isValid() ||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;

      rc = context->getEnv()->dms.getLogicalPageSpace(context->getSpaceID(),
                                                      SPACE_TYPE_MAIN_DATA, lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps object[%d], rc:%d", context->getSpaceID(), rc);
         goto error;
      }

      rc = lps->getLogicalPageBuffer(_context, lpid, mode, _lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = _lpb.validatePage(PAGE_TYPE_RECORD);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to validate page[%s], rc:%d",
                _lpb.getGlobalPid().toString().c_str(), rc);
         goto error;
      }

      head = _lpb.getReadableBodyBuffer().
             getReadableObjPtr<recordDataPageHead>(0);
      SDB_ASSERT(NULL != head, "impoosible");
      if (context->getLogicalCLID() != head->clLogcalID)
      {
         PD_LOG(PDERROR, "logical id does not match[%d,%d]",
                head->clLogcalID, context->getLogicalCLID());
         rc = SDB_VESSEL_PAGE_HEAD_NOT_MATCH;
         goto error;
      }

      _header = *head;
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void rdpReader::close()
   {
      if (isOpen())
      {
         _lpb.fini();
         _context = NULL;
         _header = recordDataPageHead();
      }
      return;
   }

   UINT32 rdpReader::getTotalSlotCount()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      return _header.totalSlotCount;
   }

   INT32 rdpReader::getSlot(RECORD_SLOT_ID pos, recordSlot &rs)const
   {
      INT32 rc = SDB_OK;
      const recordSlot *slot = NULL;
      UINT32 offset = 0;
      rs = recordSlot();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_header.totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      offset = RECORD_PAGE_HEAD_LEN + pos * RDP_RSLOT_SIZE;
      slot = _lpb.getReadableBodyBuffer().getReadableObjPtr<recordSlot>(offset);
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

   INT32 rdpReader::getNormalRecordHead(RECORD_SLOT_ID pos, 
                                         const recordHead **rh)const
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;
      strictBuffer buffer;
      const recordSlot *slot = NULL;
      const recordHead *head = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos ||
                            NULL == rh))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_header.totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      *rh = NULL;

      offset = RECORD_PAGE_HEAD_LEN + pos * RDP_RSLOT_SIZE;
      buffer = _lpb.getReadableBodyBuffer();
      slot = buffer.getReadableObjPtr<recordSlot>(offset);
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

      head = buffer.getReadableObjPtr<recordHead>(slot->getOffset());
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get record head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      *rh = head;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpReader::getNormalRecordBody(UINT32 pos,
                                        slice &data)const
   {
      INT32 rc = SDB_OK;
      const recordSlot *slot = NULL;
      const recordHead *head = NULL;
      UINT32 offset = 0;
      strictBuffer buffer;
      data.reset();

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (_header.totalSlotCount <= pos)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      offset = RECORD_PAGE_HEAD_LEN + pos * RDP_RSLOT_SIZE;
      buffer = _lpb.getReadableBodyBuffer();
      slot = buffer.getReadableObjPtr<recordSlot>(offset);
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

      head = buffer.getReadableObjPtr<recordHead>(slot->getOffset());
      if (NULL == head)
      {
         PD_LOG(PDERROR, "failed to get record head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(head->isDependent()))
      {
         PD_LOG(PDERROR, "unexpected dependent record at[%d]", pos);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      else if (head->isOverflow())
      {
         PD_LOG(PDERROR, "record is overflow at pos[%d]", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (head->isTombstone())
      {
         PD_LOG(PDERROR, "record is tomestone at pos[%d]", pos);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      SDB_ASSERT(!head->isCompressed(), "TODO");

      if (RDP_RECORD_HEAD_LEN < head->getSize())
      {
         slice tmp = buffer.getSlice(slot->getOffset() + RDP_RECORD_HEAD_LEN,
                                         head->getSize() - RDP_RECORD_HEAD_LEN);
         if (!tmp.isValid())
         {
            PD_LOG(PDERROR, "failed to get readble record body ptr[%d,%d]",
                   slot->getOffset() + RDP_RECORD_HEAD_LEN,
                   head->getSize() - RDP_RECORD_HEAD_LEN);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         data = tmp;
      }
      else
      {
         PD_LOG(PDERROR, "invalid record size[%d] found in offset[%d] of page[%s]",
                head->getSize(), slot->getOffset(),
                _lpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}//namesapce vessel
}//namespace engine