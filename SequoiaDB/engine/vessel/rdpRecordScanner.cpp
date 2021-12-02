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

   Source File Name = rdpRecordScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rdpRecordScanner.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"

namespace engine
{
namespace vessel
{
   rdpRecordScanner::~rdpRecordScanner()
   {
      close();
   }

   INT32 rdpRecordScanner::open(requestContext *context,
                                PAGE_ID lpid,
                                memoryBlock *buffer,
                                RECORD_SLOT_ID begin,
                                RECORD_SLOT_ID end)
   {
      INT32 rc = SDB_OK;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isSpaceIdLocked() ||
                       DMS_INVALID_LOGICCLID == context->getLogicalCLID() ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_RECORD_SLOT_ID == begin))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _lpid = lpid;
      _endBound = end;
      _buffer = (NULL == buffer) ? &_mb : buffer;
      _buffer->resize(0);

      rc = initAccessor();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = relocateFromPos(begin);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to locate slot:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void rdpRecordScanner::close()
   {
      if (isOpen())
      {
         _context = NULL;
         _lpid = INVALID_PAGE_ID;
         _endBound = INVALID_RECORD_SLOT_ID;
         _lpb.fini();
         _accessor.fini();
         _pos = INVALID_RECORD_SLOT_ID;
         _rs.reset();
         _rh.reset();
         _transID = DPS_TRANS_ID();
         _recordData.reset();
         _buffer = NULL;
         _mb.release();
      }

      return;
   }

   INT32 rdpRecordScanner::next()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == _pos))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = relocateFromPos(_pos + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to search visible record slot:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN rdpRecordScanner::isReadyToFetch()const
   {
      return isOpen() && 
             INVALID_RECORD_SLOT_ID != _pos;
   }

   INT32 rdpRecordScanner::fetchRecord(BOOLEAN forceCopy)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen() || !isReadyToFetch()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(_rs.isValid(), "can not be invalid");
      SDB_ASSERT(!_rs.isTombstone(), "impossible");
      SDB_ASSERT(!_rs.isOverflow(), "TODO");
      SDB_ASSERT(RDP_RECORD_HEAD_TYPE_NORMAL == _rs.type, "TODO");

      if (_recordData.isValid())
      {
         /// do not refetch
         goto done;
      }
      
      rc = fetchNormalRecord();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fetch record data:%d", rc);
         goto error;
      }

      if (forceCopy)
      {
         rc = _buffer->copy(_recordData.getSize(), _recordData.data());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to copy record data to buffer:%d", rc);
            goto error;
         }

         _recordData.reset(_buffer->getSize(), _buffer->getBuffer());
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void rdpRecordScanner::clearDataCached()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      _pos = INVALID_RECORD_SLOT_ID;
      _rs.reset();
      _rh.reset();
      _transID = DPS_TRANS_ID();
      _recordData.reset();
      _buffer->resize(0);
      return;
   }

   INT32 rdpRecordScanner::fetchNormalRecord()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReadyToFetch(), "can not be invisible");
      SDB_ASSERT(RDP_RECORD_HEAD_TYPE_NORMAL == _rs.type, "must be normal header");
      SDB_ASSERT(!_rs.isTombstone() && !_rs.isOverflow(), "can not be invalid");
      rc = _accessor.getRecord(_pos, _rh, _recordData);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fetch record data:%d", rc);
         goto error;
      }

      SDB_ASSERT(!_rh.format.normal.isCompressed(), "TODO");
      _transID = _rh.format.normal.getTransID();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpRecordScanner::relocateFromPos(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");

      DPS_LSN_OFFSET minFileLsn = _context->getOuterResource()->logger->getMinFileLSN();
      DPS_LSN_OFFSET lsn = _lpb.getRuntimeBuffer().getPageHead()->lsn;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "impossible");
      RECORD_ID_LATCH_MAP::object latchObj;
      RECORD_ID_LATCH_MAP &globalRidLatchMap = _context->getEnv()->ridLatchMap;
      recordSlot slot;

      clearDataCached();

      while (pos < _endBound &&
             pos < _accessor.getTotalSlotCount())
      {
         recordID rid(_lpid, pos);
         slot.reset();
         rc = _accessor.getSlot(pos, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get slot data[%d], rc:%d", pos, rc);
            goto error;
         }
         else if (!slot.isValid() ||
                  slot.isInvisible() ||
                  slot.isTombstone())
         {
            ++pos;
            continue;
         }
         else if (DPS_INVALID_LSN_OFFSET != minFileLsn &&
                  lsn < minFileLsn)
         {
            break;
         }
         else if (_context->testRidLocked(rid))
         {
            break;
         }
         else
         {
            recordIdLatchKey key(_context->getLogicalCSID(),
                                 _context->getLogicalCLID(), rid);
            latchObj = globalRidLatchMap.get(key);
            if (!latchObj.isValid())
            {
               /// no one holding rid latch now
               break;
            }
            else if (latchObj.getValue().tryLockShared())
            {
               latchObj.getValue().unlockShared();
               globalRidLatchMap.release(latchObj);
               break;
            }
            else
            {
               /// some one holding x latch, we release page latch and wait for it.
               _accessor.fini();
               _lpb.fini();
               slot.reset();
               latchObj.getValue().lockShared();
               rc = initAccessor();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reinit accessor:%d", rc);
                  goto error;
               }

               if (_accessor.getTotalSlotCount() <= pos)
               {
                  /// out of bound, records may be removed.
                  goto done;
               }

               rc = _accessor.getSlot(pos, slot);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get slot data[%d], rc:%d", pos, rc);
                  goto error;
               }

               latchObj.getValue().unlockShared();
               globalRidLatchMap.release(latchObj);

               if (slot.isValid() && !slot.isInvisible() &&
                   !slot.isTombstone())
               {
                  break;
               }
               else
               {
                  ++pos;
                  continue;
               }
            }
         }
      }

      if (slot.isValid())
      {
         _pos = pos;
         _rs = slot;
      }
   done:
      if (latchObj.isValid())
      {
         latchObj.getValue().unlockShared();
         globalRidLatchMap.release(latchObj);
      }
      return rc;
   error:
      clearDataCached();
      goto done;
   }

   recordID rdpRecordScanner::getCurrentRid()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isReadyToFetch(), "can not be invalid");
      return recordID(_lpid, _pos);
   }

   BOOLEAN rdpRecordScanner::isOverflow()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isReadyToFetch(), "can not be invalid");
      return _rs.isOverflow();
   }

   BOOLEAN rdpRecordScanner::isBigRecord()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isReadyToFetch(), "can not be invalid");
      return _rs.isBigRecord();
   }

   UINT32 rdpRecordScanner::getCurrentPageSeq()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      return _accessor.getReadablePageHead()->pageSeq;
   }

   recordID rdpRecordScanner::getOverflowAddr()const
   {
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(isReadyToFetch(), "can not be invalid");
      SDB_ASSERT(_recordData.isValid(), "must be fetched");
      return _rh.format.overflow.getRid();
   }

   INT32 rdpRecordScanner::initAccessor()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != _lpid, "can not be invalid");

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      LPS_OBJ_PTR lps;

      rc = _context->getEnv()->dms.getLogicalPageSpace(_context->getSpaceID(),
                                                       SPACE_TYPE_MAIN_DATA,
                                                       lps);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lps[%d], rc:%d", _context->getSpaceID(), rc);
         goto error;
      }

      rc = lps->getLogicalPageBuffer(_context, _lpid, mode, _lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", _lpid, rc);
         goto error;
      }

      rc = _accessor.init(_context, &_lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rdp accessor:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
