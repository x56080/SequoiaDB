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
                                RECORD_SLOT_ID begin,
                                const options *o)
   {
      INT32 rc = SDB_OK;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_RECORD_SLOT_ID == begin))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _lpid = lpid;
      if (NULL != o)
      {
         _o = *o;
      }

      rc = initAccessor();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = scanFrom(begin);
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

   INT32 rdpRecordScanner::openToRead(requestContext *context,
                                      const recordID &rid)
   {
      INT32 rc = SDB_OK;
      options o;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isMbContextAttached() ||
                       !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      o.endBound = rid.getSlotID() + 1;
      o.nolockWhenScanForNone = TRUE;
      rc = open(context, rid.getPageID(), rid.getSlotID(), &o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open scanner:%d", rc);
         goto error;
      }

      if (!isReadyToRead())
      {
         PD_LOG(PDERROR, "rid:%s not found", rid.toString().c_str());
         rc = SDB_VESSEL_RECORD_NOT_FOUND;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void rdpRecordScanner::close()
   {
      if (isOpen())
      {
         _lpb.fini();
         _o = options();
         _lpid = INVALID_PAGE_ID;
         _accessor.fini();
         clearDataCached();

         _context = NULL;
      }

      return;
   }

   INT32 rdpRecordScanner::next()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isReadyToRead()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = scanFrom(_pos + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to search visible record slot:%d", rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      close();
      goto done;
   }

   BOOLEAN rdpRecordScanner::isReadyToRead()const
   {
      return isOpen() && 
             INVALID_RECORD_SLOT_ID != _pos;
   }

   INT32 rdpRecordScanner::fetchRecord(RECORD_SLOT_ID pos,
                                       UINT8 type)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(RDP_RECORD_HEAD_TYPE_INVALID != type, "can not be invalid");

      if (RDP_RECORD_HEAD_TYPE_NORMAL == type)
      {
         rc = fetchNormalRecord(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch normal record:%d", rc);
            goto error;
         }
      }
      else
      {
         SDB_ASSERT(FALSE, "TODO");
         rc = SDB_VESSEL_INTERNAL_ERR;
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
      _overflowAddr = recordID();
      _recordType = RDP_RECORD_HEAD_TYPE_INVALID;
      _flags = 0;
      _transID = DPS_TRANS_ID();
      _recordData.reset();
      if (NULL != _recordBuffer)
      {
         _context->releaseBuffer(_recordBuffer);
         _recordBuffer = NULL;
         _recordBufferSize = 0;
      }
      
      return;
   }

   INT32 rdpRecordScanner::fetchNormalRecord(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      normalRecordHead rh;
      slice recordData;
      rc = _accessor.getNormalRecord(pos, rh, recordData);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get normal record at[%d], rc:%d", pos, rc);
         goto error;
      }

      SDB_ASSERT(!rh.isCompressed(), "TODO");
      _pos = pos;
      _recordType = RDP_RECORD_HEAD_TYPE_NORMAL;
      _transID.setNodeID(rh.transNode);
      _transID.setSN(rh.transSN);
      _recordData = recordData;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpRecordScanner::scanFrom(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      if (_o.scanOptions.isScanForNone())
      {
         rc = scanWithRU(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to scan with ru:%d", rc);
            goto error;
         }
      }
      else
      {
         rc = scanWithLockingRecord(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to scan with locking record:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpRecordScanner::scanWithLockingRecord(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      SDB_ASSERT(!_o.scanOptions.isScanForNone(), "can not be none");
      DPS_TRANSLOCK_TYPE mode = _o.scanOptions.isScanForShare() ?
                                DPS_TRANSLOCK_S : DPS_TRANSLOCK_U;
      UINT8 recordType = RDP_RECORD_HEAD_TYPE_INVALID;
      recordID rid;
      BOOLEAN locked = FALSE;

      while (pos < _o.endBound &&
             pos < _accessor.getTotalSlotCount())
      {
         recordSlot rs;
         recordType = RDP_RECORD_HEAD_TYPE_INVALID;

         rc = _accessor.getSlot(pos, rs);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get slot data[%d], rc:%d", pos, rc);
            goto error;
         }
         else if (!rs.isValidAndVisible() ||
                  rs.isTombstoneRecord())
         {
            ++pos;
            continue;
         }

         SDB_ASSERT(!locked, "impossible");
         rid.setPageID(_lpb.getLogicalPid());
         rid.setSlotID(pos);
         rc = _context->tryAcquireTransLock(rid, mode, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock rid:%s, rc:%d", rid.toString().c_str(), rc);
            goto error;
         }
         else if (locked)
         {
            recordType = rs.type;
            break;
         }

         _accessor.fini();
         _lpb.fini();
         rc = _context->acquireTransLock(rid, mode);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock record:%s, rc:%d", rid.toString().c_str(), rc);
            goto error;
         }

         _context->releaseTransLock(rid);
         rc = initAccessor();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reinit accessor:%d", rc);
            goto error;
         }

         continue;
      }

      if (RDP_RECORD_HEAD_TYPE_INVALID != recordType)
      {
         rc = fetchRecord(pos, recordType);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch record:%d", rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      if (locked)
      {
         _context->releaseTransLock(rid);
      }
      goto done;
   }

   INT32 rdpRecordScanner::scanWithRU(RECORD_SLOT_ID pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != pos, "can not be invalid");
      
      DPS_LSN_OFFSET minFileLsn = _context->getOuterResource()->logger->getMinFileLSN();
      DPS_LSN_OFFSET lsn = _lpb.getRuntimeBuffer().getPageHead()->lsn;
      BOOLEAN nolock = _o.nolockWhenScanForNone ||
                       (DPS_INVALID_LSN_OFFSET != minFileLsn &&
                        DPS_INVALID_LSN_OFFSET != lsn &&
                        lsn < minFileLsn);
      RECORD_ID_LATCH_MAP &globalRidLatchMap = _context->getEnv()->ridLatchMap;
      UINT8 recordType = RDP_RECORD_HEAD_TYPE_INVALID;

      clearDataCached();

      while (pos < _o.endBound &&
             pos < _accessor.getTotalSlotCount())
      {
         recordSlot rs;
         recordType = RDP_RECORD_HEAD_TYPE_INVALID;
         
         rc = _accessor.getSlot(pos, rs);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get slot data[%d], rc:%d", pos, rc);
            goto error;
         }
         else if (!rs.isValidAndVisible() ||
                  rs.isTombstoneRecord())
         {
            ++pos;
            continue;
         }
         else if (nolock)
         {
            recordType = rs.type;
            break;
         }
         else
         {
            recordID rid(_lpb.getLogicalPid(), pos);
            recordIdLatchKey key(_context->getSpaceID(),
                                 _context->getMBID(), rid);
            RECORD_ID_LATCH_MAP::object latchObj = globalRidLatchMap.get(key);
            if (!latchObj.isValid())
            {
               /// no one holding rid latch now
               recordType = rs.type;
               break;
            }
            else if (latchObj.getValue().try_get_shared())
            {
               latchObj.getValue().release_shared();
               globalRidLatchMap.release(latchObj);
               recordType = rs.type;
               break;
            }
            else
            {
               /// some one holding x latch, we release page latch and rescan.
               _accessor.fini();
               _lpb.fini();
               latchObj.getValue().get_shared();
               latchObj.getValue().release_shared();
               globalRidLatchMap.release(latchObj);
               rc = initAccessor();
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to reinit accessor:%d", rc);
                  goto error;
               }

               continue;
            }
         }
      }

      if (RDP_RECORD_HEAD_TYPE_INVALID != recordType)
      {
         rc = fetchRecord(pos, recordType);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch record:%d, rc:%d", pos, rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      clearDataCached();
      goto done;
   }

   recordID rdpRecordScanner::getCurrentRid()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return recordID(_lpid, _pos);
   }

   BOOLEAN rdpRecordScanner::isOverflow()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return _overflowAddr.isValid();
   }

   BOOLEAN rdpRecordScanner::isBigRecord()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isReadyToRead(), "can not be invalid");
      return 0 != OSS_BIT_TEST(_flags, _FLAG_BIG_RECORD);
   }

   UINT32 rdpRecordScanner::getCurrentPageSeq()const
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      return _accessor.getReadablePageHead()->pageSeq;
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
