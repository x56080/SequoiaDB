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

   Source File Name = btreeNode.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/btreeNode.h"
#include "vessel/requestContext.h"
#include "vessel/indexContext.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/orderingWrapper.h"

namespace engine
{
namespace vessel
{
   btreeNode::btreeNode(logicalPageBuffer *buffer,
                        indexContext *ic,
                        UINT32 depth):
   _buffer(buffer),
   _ic(ic),
   _depth(depth)
   {
      SDB_ASSERT(NULL != _buffer && NULL != _ic, "can not be invalid");
   }

   void btreeNode::reset()
   {
      _buffer = NULL;
      _ic = NULL;
      _depth = 0;
      return;
   }


   BOOLEAN btreeNode::isRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return 0 == _depth;
   }

   BOOLEAN btreeNode::isLeaf()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadbleHead();
      SDB_ASSERT(NULL != head, "impossible");
      return INVALID_PAGE_ID == head->rightChild;
   }

   BOOLEAN btreeNode::hasExtNode()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const btreeNodePageHead *head = getReadbleHead();
      SDB_ASSERT(NULL != head, "impossible");
      return INVALID_PAGE_ID != head->extNode;
   }

   const btreeNodePageHead *btreeNode::getReadbleHead()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _buffer->getRuntimeBuffer().getReadablePtrOfBody<btreeNodePageHead>(0);
   }


   INT32 btreeNode::search(const ixmKey &key,
                           const recordID &rid,
                           RECORD_SLOT_ID &slotNo,
                           BOOLEAN &identical)const
   {
      INT32 rc = SDB_OK;
   

      const btreeNodePageHead *head = NULL;
      orderingWrapper ow;
      slotNo = INVALID_RECORD_SLOT_ID;
      identical = FALSE;

      INT32 low = 0;
      INT32 high = 0;
      INT32 middle = 0;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!key.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = getReadbleHead();
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get readable page head");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ow = _ic->getObj().getPattern().getOrdering();
      high = (INT32)(head->totalSlotCount) - 1;
      middle = ((low + high) >> 1);

      while (low <= high)
      {
         ixmKey currentKey;
         INT32 res = 0;
         btreeIndexTuple tuple;
         rc = getIndexTuple((RECORD_SLOT_ID)middle, tuple);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get tuple at[%d], rc:%d", middle, rc);
            goto error;
         }

         if (tuple.getSlot()->isKeyCompressed())
         {
            SDB_ASSERT(FALSE, "TODO");
         }

         tuple.getKeyWhenNotCompressed(currentKey);
         res = key.woCompare(currentKey, ow.toBsonOrdering());
         if (0 == res)
         {
            res = rid.compare(tuple.getRid());
         }

         if (res < 0)
         {
            high = middle - 1;
         }
         else if (0 < res)
         {
            low = middle + 1;
         }
         else
         {
            slotNo = middle;
            identical = TRUE;
            goto done;
         }

         middle = ((low + high) >> 1); 
      }

      slotNo = low;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 btreeNode::getIndexTuple(RECORD_SLOT_ID slotNo,
                                  btreeIndexTuple &tuple)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotNo, "can not be invalid");
      const btreeNodePageHead *head = NULL;
      const btreeNodeSlot *slot = NULL;
      const CHAR *keyData = NULL;

      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == slotNo))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      head = getReadbleHead();
      if (OSS_UNLIKELY(NULL == head))
      {
         PD_LOG(PDERROR, "failed to get readble head ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (head->totalSlotCount <= slotNo)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      slot = _buffer->getRuntimeBuffer().getReadablePtrOfBody
             <btreeNodeSlot>(BTREE_NODE_PAGE_HEAD_SIZE +
                             (slotNo * BTREE_NODE_SLOT_SIZE));
      if (OSS_UNLIKELY(NULL == slot))
      {
         PD_LOG(PDERROR, "failed to get slot ptr");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (OSS_UNLIKELY(!slot->isValid()))
      {
         PD_LOG(PDERROR, "slot[%d] is invalid", slotNo);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!slot->isKeySavedInSlot())
      {
         keyData = _buffer->getRuntimeBuffer().getReadablePtrOfBody<CHAR>(slot->data.pointer.offset);
         if (OSS_UNLIKELY(NULL == keyData))
         {
            PD_LOG(PDERROR, "failed to get key data ptr");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }

      if (!tuple.init(slotNo, slot, keyData))
      {
         PD_LOG(PDERROR, "failed to init index tuple");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      return rc;
   error:
      tuple.fini();
      goto done;
   }
} // namespace vessel

} // namespace engine

