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

   Source File Name = recordReader.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/recordReader.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/mainDataSpace.h"
#include "vessel/instanceEnv.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/IRedoLogger.h"

namespace engine
{
namespace vessel
{
   recordReader::recordReader()
   {}

   recordReader::~recordReader()
   {
      fini();
   }

   void recordReader::fini()
   {
      if (NULL == _context)
      {
         goto done;
      }

      _lpb.fini();
      _context = NULL;
      _scanner.close();
      _lpid = INVALID_PAGE_ID;
      _mds = NULL;
      _buffer = NULL;
      _mb.release();
      _nextSlot = 0;
      _currentSlot = INVALID_RECORD_SLOT_ID;
      _currentRecordHead = recordHead();
      _currentRecord.reset();

   done:
      return;
   }

   INT32 recordReader::init(requestContext *context,
                            PAGE_ID lpid,
                            mainDataSpace *mds,
                            RECORD_SLOT_ID seek,
                            memoryBlock *mb)
   {
      INT32 rc = SDB_OK;
      fini();

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_ID == lpid ||
                       INVALID_RECORD_SLOT_ID == seek ||
                       NULL == mds ||
                       !mds->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _context = context;
      _lpid = lpid;
      _nextSlot = seek;
      _mds = mds;
      _buffer = NULL == mb ? &_mb : mb;
      _buffer->resize(0);

      rc = openScanner();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open scanner:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 recordReader::fetchNextToReader(BOOLEAN &hitTheEnd)
   {
      INT32 rc = SDB_OK;
      hitTheEnd = FALSE;
      RECORD_ID_LATCH_MAP *latchMap = NULL;
      DPS_LSN_OFFSET minFileLsn = DPS_INVALID_LSN_OFFSET;

      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
  
      clearCurrentRecord();
      SDB_ASSERT(_scanner.isOpen(), "must be open");
      latchMap = &(_context->getEnv()->ridLatchMap);
      minFileLsn = _context->getOuterResource()->logger->getMinFileLsn();
      
      do
      {     
         recordSlot slot;
         UINT32 totalSlotCount = 0;
         /// Once we release lpid latch, page may be upadted.
         DPS_LSN_OFFSET lsn = _lpb.getRuntimeBuffer().getPageHead()->lsn;
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != minFileLsn, "impossible");
         SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "impossible");
         BOOLEAN needLockRid = minFileLsn <= lsn;

         totalSlotCount = _scanner.getTotalSlotCount();
         while (_nextSlot < totalSlotCount)
         {
            rc = _scanner.getSlot(_nextSlot, slot);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get slot[%d, %d], rc:%d",
                     _lpid, _nextSlot, rc);
               goto error;
            }

            if (!slot.isValidAndVisible())
            {
               ++_nextSlot;
               slot = recordSlot();
               continue;
            }

            break;   
         }

         if (!slot.isValidAndVisible())
         {
            hitTheEnd = TRUE;
            goto done;
         }

         if (needLockRid)
         {
            RECORD_ID_LATCH_MAP::object latchObj;
            recordIdLatchKey latchKey(_context->getSpaceID(),
                                      _context->getMBID(),
                                      recordID(_lpid, _nextSlot));
            rc = latchMap->get(latchKey, latchObj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get latch obj:%d", rc);
               goto error;
            }

            /// If latch obj not found, we do not need to 
            /// get rid latch.
            if (latchObj.isValid())
            {
               if (!latchObj.getValue().tryLockShared())
               {
                  closeScanner();
                  latchObj.getValue().lockShared();
                  latchObj.getValue().unlockShared();
                  latchMap->release(latchObj);
                  rc = openScanner();
                  if (SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "failed to reopen scanner:%d", rc);
                     goto error;
                  }
                  /// retry form begining.
                  continue;
               }
               else
               {
                  latchObj.getValue().unlockShared();
                  latchMap->release(latchObj);
               }
            }
         }

         rc = fetchRecord(_nextSlot, slot);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to fetch record[%d,%d], rc:%d",
                   _lpid, _nextSlot, rc);
            goto error;
         }

         ++_nextSlot;
         break;
      } while (TRUE);
      

   done:
      return rc;
   error:
      goto done;
   }

   recordID recordReader::getCurrentRid()const
   {
      recordID rid;
      if (isCurrentRecordAvailable())
      {
         rid.setPageID(_lpid);
         rid.setSlotID(_currentSlot);
      }
      return rid;
   }

   BOOLEAN recordReader::currentRecordIsTombstone()const
   {
      SDB_ASSERT(isCurrentRecordAvailable(), "must be valid");
      return _currentRecordHead.isTombstone();
   }

   INT32 recordReader::openScanner()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);
      rc = _mds->getLogicalPageBuffer(_context, _lpid, mode, _lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d", _lpid, rc);
         goto error;
      }

      rc = _scanner.open(_context, &_lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open rdp scanner:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void recordReader::closeScanner()
   {
      SDB_ASSERT(isOpen(), "must be open");
      _scanner.close();
      _lpb.fini();
      return;
   }

   void recordReader::clearCurrentRecord()
   {
      SDB_ASSERT(isOpen(), "must be open");
      _currentSlot = INVALID_RECORD_SLOT_ID;
      _currentRecordHead = recordHead();
      _currentRecord.reset();

   done:
      return;
   }

   INT32 recordReader::fetchRecord(RECORD_SLOT_ID slotID,
                                   const recordSlot &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(_scanner.isOpen(), "can not be closed");
      SDB_ASSERT(INVALID_RECORD_SLOT_ID != slotID, "can not be invalid");
      SDB_ASSERT(slot.isValidAndVisible(), "can not be invisible");
      SDB_ASSERT(!isCurrentRecordAvailable(), "can not be available");

      _currentSlot = slotID;

      if (slot.isBigRecordHead())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
      
      rc = _scanner.getNormalRecordHeadAndBody(_currentSlot,
                                                _currentRecordHead,
                                                _currentRecord);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get normal record[%d], rc:%d",
                  _currentSlot, rc);
         goto error;
      }

      SDB_ASSERT(!_currentRecordHead.isDependent(), "impossible");

      if (_currentRecordHead.isCompressed() ||
            _currentRecordHead.isOverflow() ||
            _currentRecordHead.isTombstone())
      {
         SDB_ASSERT(FALSE, "TODO");
      }
   done:
      return rc;
   error:
      clearCurrentRecord();
      goto done;
   }
}//namespace vessel
}//namespace engine