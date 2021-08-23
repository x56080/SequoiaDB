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

   Source File Name = dmlContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dmlContext.h"
#include "utilMemListPool.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/instanceEnv.h"
#include "utilBsonHash.hpp"

namespace engine
{
namespace vessel
{
   dmlContext::~dmlContext()
   {
      unlockRidsAndUniqueKeys();
   }

   void dmlContext::close()
   {
      unlockRidsAndUniqueKeys();
      fini();
      requestContext::close();
      return;
   }

   void dmlContext::fini()
   {
      SDB_ASSERT(_uniqueKeyContext.empty(), "release locks first");
      SDB_ASSERT(_uniqueKeyHash.empty(), "release locks first");
      SDB_ASSERT(_ridLatchContext.isEmpty(), "release locks first");

      _minFreeSize = 0;
      _compressionType = UTIL_COMPRESSOR_INVALID;
      _transID.reset();
      _lockRid = FALSE;
      _uniqueKeyHash.clear();
      _uniqueKeyContext.clear();
      _rid = recordID();
      _lsn = DPS_INVALID_LSN_OFFSET;
      _seq = INVALID_CL_PAGE_SEQ;
      return;
   }

   INT32 dmlContext::lockUniqueIndexKeys(const dmlIndexRequestArray &requests)
   {
      INT32 rc = SDB_OK;
      if (!requestContext::isOpen() ||
          !requestContext::isSpaceIdLocked() ||
          !requestContext::isMbLocked())
      {
         SDB_ASSERT(FALSE, "impossible");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!_uniqueKeyContext.empty())
      {
         SDB_ASSERT(FALSE, "do not relock");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (0 == requests.getUniqueIndexCount())
      {
         goto done;
      }

      buildUniqueKeyHash(requests, _uniqueKeyHash);

      rc = _lockUniqueIndexKeys();
      if (SDB_OK != rc)
      {
         _uniqueKeyHash.clear();
         PD_LOG(PDERROR, "failed to lock unique index keys:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void dmlContext::buildUniqueKeyHash(const dmlIndexRequestArray &requests,
                                       ossPoolVector<UINT32> &hashArray)const
   {
      UINT32 size = requests.getSize();
      for (UINT32 i = 0; i < size; ++i)
      {
         uniqueIndexLatchKey keyHash;
         UNIQUE_INDEX_LATCH_MAP::object obj;
         dmlIndexRequest *r = requests.get(i);
         SDB_ASSERT(NULL == r || r->isValid(), "impossible");
         if (NULL == r || !r->isUnique())
         {
            continue;
         }

         for (ossPoolList<bson::BSONObj>::const_iterator itr = r->getKeys().begin();
              itr != r->getKeys().end(); ++r)
         {
            UINT32 hash = BSON_HASHER::hashObj(*itr) + r->getIndexSlot();
            hashArray.push_back(hash);
         }
      }
      return;
   }

   INT32 dmlContext::_lockUniqueIndexKeys()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_uniqueKeyContext.empty(), "must be empty");      
      UNIQUE_INDEX_LATCH_MAP &latchMap = requestContext::getEnv()->uniqueIndexLathMap;

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

      for (UINT32 i = 1; i < _uniqueKeyHash.size(); ++i)
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

         UINT32 stopped = 0;
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
               stopped = i;
               break;
            }
         }

         if (lockedThisLoop == _uniqueKeyContext.size())
         {
            break;
         }

         for (UINT32 i = lockedThisLoop; i > 0; --i)
         {
            _uniqueKeyContext[i - 1].getValue().release();
         }

         _uniqueKeyContext[stopped].getValue().get();
         _uniqueKeyContext[stopped].getValue().release();
      } while (TRUE);
      return;
   }

   void dmlContext::unlockUniqueKeys()
   {
      if (_ridLatchContext.isEmpty())
      {
         goto done;
      }

      {
      SDB_ASSERT(requestContext::isOpen(), "can not be closed");
      UNIQUE_INDEX_LATCH_MAP &latchMap = requestContext::getEnv()->uniqueIndexLathMap;
      _UNIQUE_KEY_CONTEXT::reverse_iterator ritr = _uniqueKeyContext.rbegin();
      for (; ritr != _uniqueKeyContext.rend(); ++ritr)
      {
         ritr->getValue().release();
         latchMap.release(*ritr);
      }
      _uniqueKeyContext.clear();
      _uniqueKeyHash.clear();
      }
   done:
      return;
   }

   INT32 dmlContext::tryToLockRid(const recordID &rid,
                                  const ossSharedLatchMode &mode,
                                  BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      recordIdLatchKey key(getSpaceID(), getMBID(), rid);
      locked = FALSE;
      RECORD_ID_LATCH_MAP *latchMap = NULL;
      RECORD_ID_LATCH_MAP::object latchObj;

      if (OSS_UNLIKELY(!requestContext::isOpen() ||
                        !requestContext::isSpaceIdLocked() ||
                        !requestContext::isMbLocked()))
      {
         SDB_ASSERT(FALSE, "context not ready");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!rid.valid() ||
                             mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      latchMap = &(getEnv()->ridLatchMap);
      rc = latchMap->ensure(key, latchObj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
         goto error;
      }

      locked = latchObj.getValue().tryLockWith(mode);
      if (!locked)
      {
         latchMap->release(latchObj);
         goto done;
      }

      rc = _ridLatchContext.push(latchObj, mode);
      if (SDB_OK != rc)
      {
         latchObj.getValue().unlockWith(mode);
         latchMap->release(latchObj);
         PD_LOG(PDERROR, "failed toi push latch obj:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dmlContext::lockRid(const recordID &rid,
                             const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      recordIdLatchKey key(getSpaceID(), getMBID(), rid);
      RECORD_ID_LATCH_MAP *latchMap = NULL;
      RECORD_ID_LATCH_MAP::object latchObj;

      if (OSS_UNLIKELY(!requestContext::isOpen() ||
                        !requestContext::isSpaceIdLocked() ||
                        !requestContext::isMbLocked()))
      {
         SDB_ASSERT(FALSE, "context not ready");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!rid.valid() ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      latchMap = &(getEnv()->ridLatchMap);
      rc = latchMap->ensure(key, latchObj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
         goto error;
      }

      rc = _ridLatchContext.push(latchObj, mode);
      if (SDB_OK != rc)
      {
         latchMap->release(latchObj);
         PD_LOG(PDERROR, "failed toi push latch obj:%d", rc);
         goto error;
      }

      latchObj.getValue().lockWith(mode);
   done:
      return rc;
   error:
      goto done;
   }

   void dmlContext::unlockRid(const recordID &rid)
   {
      recordIdLatchKey key(getSpaceID(), getMBID(), rid);
      RECORD_ID_LATCH_MAP *latchMap = NULL;
      RECORD_ID_LATCH_MAP::object latchObj;
      ossSharedLatchMode mode;
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!requestContext::isOpen() ||
                        !requestContext::isSpaceIdLocked() ||
                        !requestContext::isMbLocked()))
      {
         SDB_ASSERT(FALSE, "context not ready");
         goto done;
      }
      else if (OSS_UNLIKELY(!rid.valid()))
      {
         SDB_ASSERT(FALSE, "rid can not be invalid");
         goto done;
      }

      latchMap = &(getEnv()->ridLatchMap);
      rc = _ridLatchContext.pop(key, latchObj, mode);
      if (SDB_OK != rc)
      {
         SDB_ASSERT(FALSE, "rid not found");
         goto done;
      }

      latchObj.getValue().unlockWith(mode);
      latchMap->release(latchObj);

   done:
      return;
   }

   void dmlContext::unlockRids()
   {
      RECORD_ID_LATCH_MAP *latchMap = NULL;
      RECORD_ID_LATCH_MAP::object latchObj;
      ossSharedLatchMode mode;

      if (_ridLatchContext.isEmpty())
      {
         goto done;
      }

      if (OSS_UNLIKELY(!requestContext::isOpen() ||
                        !requestContext::isSpaceIdLocked() ||
                        !requestContext::isMbLocked()))
      {
         SDB_ASSERT(FALSE, "context not ready");
         goto done;
      }

      latchMap = &(getEnv()->ridLatchMap);
      while (_ridLatchContext.pop(latchObj, mode))
      {
         latchObj.getValue().unlockWith(mode);
         latchMap->release(latchObj);
      }

   done:
      return;
   }

   void dmlContext::unlockRidsAndUniqueKeys()
   {
      unlockRids();
      unlockUniqueKeys();
   }
}//namespace vessel
}//namespace engine