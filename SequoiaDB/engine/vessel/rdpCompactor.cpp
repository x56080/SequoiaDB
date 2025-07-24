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

   Source File Name = pageCompactor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/31/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/rdpCompactor.h"
#include "utilMemListPool.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   
   rdpCompactor::rdpCompactor(CHAR* buf, UINT32 bufSize)
   {
      reset(buf, bufSize);
   }

   void rdpCompactor::reset(CHAR* buf, UINT32 bufSize)
   {
      SDB_ASSERT(NULL != buf, "can not be null");
      SDB_ASSERT(0 < bufSize, "must be larger than zero");
      ossMemset(buf, 0, bufSize);
      _buffer.makeWritable(bufSize, buf);
      _totalSlotCount = 0;
      _totalFreeSpace = bufSize;
      _frontOffset = 0;
      _backOffset = bufSize;
   }

   INT32 rdpCompactor::push(const UINT16 &slotFlags,
                            const UINT8 &slotType, 
                            const slice &recordHeadAndData)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(RDP_RECORD_HEAD_TYPE_INVALID != slotType, "can not be invalid");
      SDB_ASSERT(recordHeadAndData.isValid(), "can not be invalid");
      SDB_ASSERT(_buffer.isWritable(), "must be writable");

      recordSlot slot;
      UINT16 offset = _backOffset - recordHeadAndData.getSize();
      UINT32 deltaSize = recordHeadAndData.getSize() + RDP_RSLOT_SIZE;
      if (_totalFreeSpace < deltaSize)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         PD_LOG(PDERROR, "do not have enough space");
         goto error;
      }

      slot.flags = slotFlags;
      slot.type = slotType;
      slot.reservedSpaceSize = 0;
      slot.offset = RECORD_PAGE_HEAD_SIZE + offset;
      slot.size = recordHeadAndData.getSize();

      _buffer.write(_frontOffset, RDP_RSLOT_SIZE, &slot);
      _buffer.write(offset, 
                    recordHeadAndData.getSize(),
                    recordHeadAndData.getData());

      _frontOffset += RDP_RSLOT_SIZE;
      ++_totalSlotCount;
      _backOffset -= recordHeadAndData.getSize(); 
      _totalFreeSpace -= deltaSize;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpCompactor::push(const UINT16 &slotFlags,
                            const UINT8 &slotType, 
                            const slice &recordHead,
                            const slice &recordData)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(RDP_RECORD_HEAD_TYPE_INVALID != slotType, "can not be invalid");
      SDB_ASSERT(recordHead.isValid(), "can not be invalid"); 
      SDB_ASSERT(recordData.isValid(), "can not be invalid");
      SDB_ASSERT(_buffer.isWritable(), "must be writable");

      recordSlot slot;
      UINT16 offset = _backOffset - 
                      recordHead.getSize() - 
                      recordData.getSize();
      UINT32 deltaSize = recordHead.getSize() + 
                         recordData.getSize() + 
                         RDP_RSLOT_SIZE;
      if (_totalFreeSpace < deltaSize)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         PD_LOG(PDERROR, "do not have enough space");
         goto error;
      }

      slot.flags = slotFlags;
      slot.type = slotType;
      slot.reservedSpaceSize = 0;
      slot.offset = RECORD_PAGE_HEAD_SIZE + offset;
      slot.size = recordHead.getSize() + recordData.getSize();

      _buffer.write(_frontOffset, RDP_RSLOT_SIZE, &slot);
      _buffer.write(offset, 
                    recordHead.getSize(),
                    recordHead.getData());
      _buffer.write(offset + recordHead.getSize(), 
                    recordData.getSize(),
                    recordData.getData());
      
      _frontOffset += RDP_RSLOT_SIZE;
      ++_totalSlotCount;
      _backOffset -= recordHead.getSize() + recordData.getSize();
      _totalFreeSpace -= deltaSize;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpCompactor::pushEmptySlot()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_buffer.isWritable(), "must be writable");

      recordSlot slot;
      if (_totalFreeSpace < RDP_RSLOT_SIZE)
      {
         rc = SDB_VESSEL_NOT_ENOUGH_SPACE_IN_PAGE;
         PD_LOG(PDERROR, "do not have enough space");
         goto error;
      }
      _buffer.write(_frontOffset, RDP_RSLOT_SIZE, &slot);
      _frontOffset += RDP_RSLOT_SIZE;
      ++_totalSlotCount;
      _totalFreeSpace -= RDP_RSLOT_SIZE;

   done:
      return rc;
   error:
      goto done;
   }

   const CHAR* rdpCompactor::getBuffer()
   {
      SDB_ASSERT(_buffer.isValid(), "must be valid");
      return _buffer.getReadableBuffer().getReadablePtr(0, getBufferSize());
   }

   UINT32 rdpCompactor::getBufferSize()const
   {
      SDB_ASSERT(_buffer.isValid(), "must be valid");
      return _buffer.getSize();
   }
}
}


