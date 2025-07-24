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

   Source File Name = dmlContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/dmlContext.h"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/instanceEnv.h"
#include "utilBsonHash.hpp"
#include "vessel/objectLatchHelper.hpp"

namespace engine
{
namespace vessel
{
   dmlContext::~dmlContext()
   {
      SDB_ASSERT(_uniqueKeyContext.empty(), "must be empty");
      SDB_ASSERT(!isClPropertiesSet(), "must be detached");
   }

   void dmlContext::_onClose()
   {
      _reset();
      return;
   }

   INT32 dmlContext::lockUniqueIndexKeys(const dmlIndexRequestArray &ra)
   {
      INT32 rc = SDB_OK;
      if (!requestContext::isClPropertiesSet())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_uniqueKeyContext.empty() ||
               !_uniqueKeyHash.empty())
      {
         SDB_ASSERT(FALSE, "do not relock");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }
      else if (!ra.withConstraint())
      {
         goto done;
      }

      SDB_ASSERT(requestContext::isSpaceIdLocked() &&
                 requestContext::isMbLocked(), "must be locking");

      for (UINT32 i = 0; i < ra.getSize(); ++i)
      {
         const dmlIndexRequest *req = ra.get(i);
         SDB_ASSERT(NULL != req && req->isValid(), "can not be invalid");
         if (!req->withConstraint())
         {
            continue;
         }

         for (ossPoolList<bson::BSONObj>::const_iterator itr = req->getKeysToInsert().begin();
              itr != req->getKeysToInsert().end(); ++itr)
         {
            UINT32 hash = BSON_HASHER::hashObj(*itr) + req->getObject()->getLogicalID();
            _uniqueKeyHash.push_back(hash);
         }

         for (ossPoolList<bson::BSONObj>::const_iterator itr = req->getKeysToRemove().begin();
              itr != req->getKeysToRemove().end(); ++itr)
         {
            UINT32 hash = BSON_HASHER::hashObj(*itr) + req->getObject()->getLogicalID();
            _uniqueKeyHash.push_back(hash);
         }
      }

      rc = _lockUniqueIndexKeys();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock unique index keys:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      SDB_ASSERT(_uniqueKeyContext.empty(), "must be empty");
      _uniqueKeyHash.clear();
      goto done;
   }

   INT32 dmlContext::_lockUniqueIndexKeys()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_uniqueKeyContext.empty(), "must be empty");      
      UNIQUE_INDEX_LATCH_MAP &latchMap = requestContext::getEnv()->latchEnv.uniqueIndexLathMap;

      if (_uniqueKeyHash.empty())
      {
         goto done;
      }
      if (1 < _uniqueKeyHash.size())
      {
         std::sort(_uniqueKeyHash.begin(), _uniqueKeyHash.end());
         ossPoolVector<UINT32>::iterator itr = std::unique(_uniqueKeyHash.begin(),
                                                           _uniqueKeyHash.end());
         _uniqueKeyHash.resize(std::distance(_uniqueKeyHash.begin(), itr));
      }

      for (UINT32 i = 0; i < _uniqueKeyHash.size(); ++i)
      {
         UNIQUE_INDEX_LATCH_MAP::object obj;
         uniqueIndexLatchKey key(requestContext::getSpaceID(),
                                 requestContext::getMBID(),
                                 _uniqueKeyHash.at(i));

         rc = latchMap.ensure(key, obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get latch obj:%d", rc);
            goto error;
         }
         _uniqueKeyContext.push_back(obj);
      }

      /// do not goto error from here
      loopTryLock();

   done:
      return rc;
   error:
      for (UINT32 i = 0; i < _uniqueKeyContext.size(); ++i)
      {
         latchMap.release(_uniqueKeyContext.at(i));
      }
      _uniqueKeyContext.clear();
      goto done;
   }

   void dmlContext::loopTryLock()
   {
      UINT32 loopCount = 0;
      do
      {
         if (0 == (++loopCount & 31))
         {
            PD_LOG(PDWARNING, "loop count is:%d now", loopCount);
         }

         UINT32 lockedThisLoop = 0;
         
         for (UINT32 i = 0; i < _uniqueKeyContext.size(); ++i)
         {
            UNIQUE_INDEX_LATCH_MAP::object &latch = _uniqueKeyContext[i];
            if (latch.getValue().try_get())
            {
               ++lockedThisLoop;
            }
            else
            {
               break;
            }
         }

         if (lockedThisLoop == _uniqueKeyContext.size())
         {
            break;
         }

         for (UINT32 i = 0; i < lockedThisLoop; ++i)
         {
            _uniqueKeyContext[i].getValue().release();
         }

         _uniqueKeyContext[lockedThisLoop].getValue().get();
         _uniqueKeyContext[lockedThisLoop].getValue().release();
      } while (TRUE);
      return;
   }

   void dmlContext::unlockUniqueKeys()
   {
      UNIQUE_INDEX_LATCH_MAP &latchMap = requestContext::getEnv()->latchEnv.uniqueIndexLathMap;
      for (_UNIQUE_KEY_CONTEXT::iterator itr = _uniqueKeyContext.begin();
           itr != _uniqueKeyContext.end(); ++itr)
      {
         itr->getValue().release();
         latchMap.release(*itr);
      }
      
      _uniqueKeyHash.clear();
      _uniqueKeyContext.clear();
   }

   void dmlContext::reset()
   {
      _reset();
      requestContext::unlockRids();
      return;
   }

   void dmlContext::_reset()
   {
      unlockUniqueKeys();
      _seq = 0;
      _rid = recordID();
      _lsn = DPS_INVALID_LSN_OFFSET;
      _indexReqCount = 0;
      if (_mrc.getTargetRecord().isValid())
      {
         CHAR *ptr = (CHAR *)_mrc.getTargetRecord().getData();
         releaseBuffer(ptr);
      }
      _mrc.clear();
      return;
   }

   void dmlContext::setDmlRecordInfo(UINT32 seq,
                                     const recordID &rid)
   {
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      _seq = seq;
      _rid = rid;
      return;
   }

   void dmlContext::setDmlLSN(const DPS_LSN_OFFSET &lsn)
   {
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      _lsn = lsn;
      return;
   }

   INT32 dmlContext::saveReocordDataToMrc(const slice &record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(record.isValid(), "can not be invalid");

      CHAR *buf = allocateBuffer(record.getSize());
      if (NULL == buf)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to allocate record buffer, rc:%d", rc);
         goto error;
      }
      ossMemcpy(buf, record.getData(), record.getSize());
      _mrc.setTargetRecord(slice(record.getSize(), buf));

   done:
      return rc;
   error:
      goto done;
   }

   void dmlContext::setStripingId(const dmsStripingId &striping)
   {
      _striping = striping;
   }

}//namespace vessel
}//namespace engine