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
                                RECORD_SLOT_ID endBound)
   {
      INT32 rc = SDB_OK;

      close();

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _lpid = lpid;
      _endBound = endBound;
      _buffer = (NULL == buffer) ? &_mb : buffer;
      _buffer->resize(0);

      rc = _reader.open(_context, _lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open rdp[%d] reader:%d", _lpid, rc);
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
         _reader.close();
         _context = NULL;
         _lpid = INVALID_PAGE_ID;
         _endBound = INVALID_RECORD_SLOT_ID;
         
         _pos = INVALID_RECORD_SLOT_ID;
         _rs = recordSlot();
         _rh = NULL;
         _transID = DPS_TRANS_ID();
         _recordData.reset();

         _buffer = NULL;
         _mb.release();
      }

      return;
   }

   INT32 rdpRecordScanner::locate(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_RECORD_SLOT_ID == pos))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = searchVisibleAndStableSlot(pos);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to search visible record slot:%d", rc);
         goto error;
      }
      else if (INVALID_RECORD_SLOT_ID != _pos)
      {
         rc = _reader.getNormalRecordHead(_pos, &_rh);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get record header:%d", rc);
            goto error;
         }
      }
      else
      {
         /// hit the end, do nothing.
      }
      
   done:
      return rc;
   error:
      close();
      goto done;
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

      rc = searchVisibleAndStableSlot(_pos + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to search visible record slot:%d", rc);
         goto error;
      }
      else if (INVALID_RECORD_SLOT_ID != _pos)
      {
         rc = _reader.getNormalRecordHead(_pos, &_rh);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get record header:%d", rc);
            goto error;
         }
      }
      else
      {
         /// hit the end, do nothing.
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN rdpRecordScanner::isReadyToFetch()const
   {
      return isOpen() && 
             INVALID_RECORD_SLOT_ID != _pos &&
             _rs.isValidAndVisible() &&
             NULL != _rh;
   }

   INT32 rdpRecordScanner::fetchRecord()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen() || !isReadyToFetch()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      SDB_ASSERT(NULL != _rh, "impossible to be null");
      if (_rh->isTombstone())
      {
         /// no record data
         _transID = _rh->getTransID();
         goto done;
      }
      else if (_rs.isBigRecordHead())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      else if (_rh->isOverflow())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      else
      {
         rc = fetchNormalRecord();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch record data:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void rdpRecordScanner::clearDataCached()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      _pos = INVALID_RECORD_SLOT_ID;
      _rs = recordSlot();
      _rh = NULL;
      _transID = DPS_TRANS_ID();
      _recordData.reset();
      _buffer->resize(0);
      return;
   }

   INT32 rdpRecordScanner::fetchNormalRecord()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isReadyToFetch(), "can not be invisible");
      SDB_ASSERT(_rs.isNormalRecordHead(), "must be normal header");
      SDB_ASSERT(NULL != _rh, "can not be null");
      SDB_ASSERT(!_rh->isDependent() && !_rh->isTombstone() && !_rh->isOverflow(),
                 "must be valid");
      
      SDB_ASSERT(!_rh->isCompressed(), "TODO");
      rc = _reader.getNormalRecordBody(_pos, _recordData);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fetch record data:%d", rc);
         goto error;
      }

      _transID = _rh->getTransID();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpRecordScanner::searchVisibleAndStableSlot(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(_reader.isOpen(), "must be open");
      DPS_LSN_OFFSET minFileLsn = _context->getOuterResource()->logger->getMinFileLSN();
      DPS_LSN_OFFSET lsn = _reader.getPageBuffer().getRuntimeBuffer().getPageHead()->lsn;
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "impossible");
      RECORD_ID_LATCH_MAP::object latchObj;
      RECORD_ID_LATCH_MAP &globalRidLatchMap = _context->getEnv()->ridLatchMap;

      clearDataCached();

      while (pos <= _endBound &&
             pos < _reader.getTotalSlotCount())
      {
         recordID rid(_lpid, pos);
         recordSlot slot;
         rc = _reader.getSlot(pos, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get slot data[%d], rc:%d", pos, rc);
            goto error;
         }
         else if (!slot.isValidAndVisible())
         {
            ++_pos;
            continue;
         }
         else if (DPS_INVALID_LSN_OFFSET != minFileLsn &&
                  lsn < minFileLsn)
         {
            _pos = pos;
            _rs = slot;
            goto done;
         }
         else if (_context->testRidLocked(rid))
         {
            _pos = pos;
            _rs = slot;
            goto done;
         }
         else
         {
            recordIdLatchKey key(_context->getLogicalCSID(),
                                 _context->getLogicalCLID(), rid);
            latchObj = globalRidLatchMap.get(key);
            if (!latchObj.isValid())
            {
               /// no one holding rid latch now
               _pos = pos;
               _rs = slot;
               goto done;
            }
            else if (latchObj.getValue().tryLockShared())
            {
               latchObj.getValue().unlockShared();
               globalRidLatchMap.release(latchObj);
               _pos = pos;
               _rs = slot;
               goto done;
            }
            else
            {
               /// some one holding x latch, we release page latch and wait for it.
               _reader.close();
               latchObj.getValue().lockShared();
               rc = _reader.open(_context, _lpid);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to open rdp[%d] reader, rc:%d", _lpid, rc);
                  goto error;
               }

               if (_reader.getTotalSlotCount() <= pos)
               {
                  /// out of bound, records may be removed.
                  goto done;
               }

               rc = _reader.getSlot(pos, slot);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get slot data[%d], rc:%d", pos, rc);
                  goto error;
               }

               latchObj.getValue().unlockShared();
               globalRidLatchMap.release(latchObj);

               if (slot.isValidAndVisible())
               {
                  _pos = pos;
                  _rs = slot;
                  goto done;
               }
               else
               {
                  continue;
               }
            }
         }
      }
   done:
      if (latchObj.isValid())
      {
         latchObj.getValue().unlockShared();
         globalRidLatchMap.release(latchObj);
      }
      return rc;
   error:
      goto done;
   }

   recordID rdpRecordScanner::getCurrentRid()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(_rs.isValid(), "can not be invalid");
      return recordID(_lpid, _pos);
   }

   BOOLEAN rdpRecordScanner::isTombstoneRecord()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != _rh, "can not be null");
      return _rh->isTombstone();
   }

   BOOLEAN rdpRecordScanner::isOverflowRecord()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(NULL != _rh, "can not be null");
      return _rh->isOverflow();
   }

   BOOLEAN rdpRecordScanner::isBigRecord()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(_rs.isValid(), "can not be invalid");
      return _rs.isBigRecordHead();
   }
} // namespace vessel

} // namespace engine
