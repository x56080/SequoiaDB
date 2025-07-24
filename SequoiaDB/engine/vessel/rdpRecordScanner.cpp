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

   Source File Name = rdpRecordScanner.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/rdpRecordScanner.h"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/outerResource.h"

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
                                RECORD_SLOT_POS begin,
                                const options *o)
   {
      INT32 rc = SDB_OK;
      close();

      if (OSS_UNLIKELY(NULL == context ||
                       !context->isClPropertiesSet() ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_RECORD_SLOT_POS  == begin))
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
                       !context->isClPropertiesSet() ||
                       !rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      o.endBound = rid.getPos() + 1;
      o.nolockWhenScanForNone = TRUE;
      rc = open(context, rid.getPid(), rid.getPos(), &o);
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
             isValidRecordSlotPosition(_pos);
   }

   INT32 rdpRecordScanner::fetchRecord(RECORD_SLOT_POS  pos,
                                       UINT8 type)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
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
      else if (RDP_RECORD_HEAD_OVERFLOW == type)
      {
         rc = fetchOverflowedRecord(pos);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch overflowed record:%d", rc);
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

      _pos = INVALID_RECORD_SLOT_POS ;
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

   INT32 rdpRecordScanner::fetchNormalRecord(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
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

   INT32 rdpRecordScanner::fetchOverflowedRecord(RECORD_SLOT_POS pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");

      overflowedRecord ofr;
      normalRecordHead rh;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      logicalPageSpace *lps = NULL;
      rdpAccessor accessor;
      logicalPageBuffer lpb;
      strictBuffer buf;

      rc = _accessor.getOverflowedRecord(pos, ofr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get overflowed record at[%d], rc:%d", pos, rc);
         goto error;
      }

      lps = _lpb.getLogicalPageSpace();

      rc = lps->getLogicalPageBuffer(_context, ofr.lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", ofr.lpid, rc);
         goto error;
      }

      rc = accessor.init(_context, &lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init rdp accessor:%d", rc);
         goto error;
      }

      if (!ofr.isBigRecord())
      {
         slice recordData;
         rc = accessor.getNormalRecord(ofr.pos, rh, recordData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get normal record at [%d], rc:%d", ofr.pos, rc);
            goto error;
         }
         _recordBuffer = _context->allocateBuffer(recordData.getSize());
         if (NULL == _recordBuffer)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to allocate buffer, rc:%d", rc);
            goto error;
         }
         _recordBufferSize = recordData.getSize();
         buf.makeWritable(_recordBufferSize, _recordBuffer);
         rc = buf.write(0, _recordBufferSize,  recordData.getData());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write record data, rc:%d", rc);
            goto error;
         }
      }
      else
      {
         bigRecordEntrySlice entry;
         slice entryData;
         recordID sliceAddr;
         UINT32 offset = 0;
         UINT32 bodyCount = 0;
         rc = accessor.getBigRecordEntrySlice(ofr.pos, entry, entryData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get big record entry, rc:%d", rc);
            goto error;
         }

         _recordBuffer = _context->allocateBuffer(entry.totalRecordSize);
         if (NULL == _recordBuffer)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to allocate big record buffer, rc:%d", rc);
            goto error;
         }
         _recordBufferSize = entry.totalRecordSize;

         buf.makeWritable(_recordBufferSize, _recordBuffer);

         // write entry data
         rc = buf.write(offset, entryData.getSize(), entryData.getData());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to write slice entry data, rc:%d", rc);
            goto error;
         }
         offset += entryData.getSize(); 
         sliceAddr.setPid(entry.nextPage);
         sliceAddr.setPos(entry.nextPos);
         bodyCount = entry.sliceCount - 1;

         for (UINT32 i = 0; i < bodyCount; ++i)
         {
            logicalPageBuffer tmplpb;
            rdpAccessor tmpAccessor;
            bigRecordBodySlice body;
            slice bodyData;

            rc = lps->getLogicalPageBuffer(_context, sliceAddr.getPid(), mode, tmplpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get lpb[%d], rc:%d", sliceAddr.getPid(), rc);
               goto error;
            }

            rc = tmpAccessor.init(_context, &tmplpb);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init rdp accessor:%d", rc);
               goto error;
            }

            rc = tmpAccessor.getBigRecordBodySlice(sliceAddr.getPos(), body, bodyData);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get big record body at [%d,%d], rc:%d", 
                     sliceAddr.getPid(), sliceAddr.getPos(), rc);
               goto error;
            }

            sliceAddr.setPid(body.nextPage);
            sliceAddr.setPos(body.nextPos);

            // the last slice's next addr must be invalid
            if (!sliceAddr.isValid() && i != bodyCount - 1)
            {
               rc = SDB_VESSEL_INTERNAL_ERR;
               PD_LOG(PDERROR, "invalid big record slice at [%d,%d], rc:%d",
                      sliceAddr.getPid(), sliceAddr.getPos(), rc);
               goto error;
            }

            // write body data
            rc = buf.write(offset, bodyData.getSize(), bodyData.getData());
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to write body data, rc:%d", rc);
               goto error;
            }
            offset += bodyData.getSize();
            lpb.fini();
         }
         _flags = _FLAG_BIG_RECORD;
      }
      
      _pos = pos;
      _recordType = RDP_RECORD_HEAD_OVERFLOW;
      _transID.setNodeID(rh.transNode);
      _transID.setSN(rh.transSN);
      _recordData = slice(_recordBufferSize, _recordBuffer);
      _overflowAddr = recordID(ofr.lpid, ofr.pos);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 rdpRecordScanner::scanFrom(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
      if (DMS_SCAN_FOR::NONE == _o.so.scanFor)
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

   INT32 rdpRecordScanner::scanWithLockingRecord(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_POS  != pos, "can not be invalid");
      SDB_ASSERT(DMS_SCAN_FOR::NONE != _o.so.scanFor, "can not be none");
      DPS_TRANSLOCK_TYPE mode = (DMS_SCAN_FOR::SHARE == _o.so.scanFor) ?
                                DPS_TRANSLOCK_S : DPS_TRANSLOCK_U;
      UINT8 recordType = RDP_RECORD_HEAD_TYPE_INVALID;
      recordID rid;
      BOOLEAN locked = FALSE;

      clearDataCached();

      while ((!isValidRecordSlotPosition(_o.endBound) || pos < _o.endBound) &&
             pos < (INT16)_accessor.getTotalSlotCount())
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
                  rs.isTombstone())
         {
            ++pos;
            continue;
         }

         SDB_ASSERT(!locked, "impossible");
         rid.setPid(_lpb.getLogicalPid());
         rid.setPos(pos);
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

   INT32 rdpRecordScanner::scanWithRU(RECORD_SLOT_POS  pos)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      SDB_ASSERT(isValidRecordSlotPosition(pos), "can not be invalid");
      
      DPS_LSN_OFFSET minFileLsn = _context->getOuterResource()->journal->getMinFileLsnOffset();
      DPS_LSN_OFFSET lsn = _lpb.getRuntimeBuffer().getPageHead()->lsn;
      BOOLEAN nolock = _o.nolockWhenScanForNone ||
                       (DPS_INVALID_LSN_OFFSET != minFileLsn &&
                        DPS_INVALID_LSN_OFFSET != lsn &&
                        lsn < minFileLsn);
      RECORD_ID_LATCH_MAP &globalRidLatchMap = _context->getEnv()->latchEnv.ridLatchMap;
      UINT8 recordType = RDP_RECORD_HEAD_TYPE_INVALID;

      clearDataCached();

      while ((!isValidRecordSlotPosition(_o.endBound) || pos < _o.endBound) &&
             pos < (INT16)_accessor.getTotalSlotCount())
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
                  rs.isTombstone())
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
