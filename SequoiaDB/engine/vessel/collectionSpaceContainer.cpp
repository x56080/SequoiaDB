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

   Source File Name = collectionSpaceContainer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/


#include "vessel/collectionSpaceContainer.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/bitMapUtils.h"
#include "vessel/requestContext.h"
#include "vessel/vesselOptions.h"
#include "vessel/collectionSpace.h"
#include "vessel/listCSCursor.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageFileUtil.h"
#include "vessel/IQueryFilter.h"
#include "vessel/spaceIDLockHelper.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   collectionSpaceContainer::collectionSpaceContainer():
   _isOpen(FALSE),
   _nextLogicalID(0),
   _slotAllocated(0),
   _slots(NULL),
   _creatingCount(0)
   {
      resetBitMap32(MAX_SPACE_SLOT_COUNT, _slotBits, TRUE);
   }

   collectionSpaceContainer::~collectionSpaceContainer()
   {
      fini();
   }

   INT32 collectionSpaceContainer::open(requestContext *context)
   {
      INT32 rc = SDB_OK;
      BOOLEAN rollback = FALSE;
      SDB_ASSERT(!isOpen(), "can not be open");
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rollback = TRUE;

      _slots = SDB_OSS_NEW _spaceSlot[MAX_SPACE_COUNT];
      if (NULL == _slots)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = loadCollectionSpacesOnDisk(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = updateIndexWhenOpen(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _isOpen = TRUE;
   done:
      return rc;
   error:
      if (rollback)
      {
         close(context);
      }
      goto done;
   }

   INT32 collectionSpaceContainer::close(requestContext *context)
   {
      INT32 rc = SDB_OK;
      UINT32 count = _slotAllocated;

      for (UINT32 i = 0; i < MAX_SPACE_COUNT && 0 < count; ++i)
      {
         if (_slots[i].isFree())
         {
            continue;
         }

         _slots[i].getCS()->close(context);
         --count;
      }

      fini();
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpaceContainer::fini()
   {
      _isOpen = FALSE;
      _nextLogicalID = 0;
      _slotAllocated = 0;
      
      if (NULL != _slots)
      {
         SDB_OSS_DEL []_slots;
         _slots = NULL;
         resetBitMap32(MAX_SPACE_SLOT_COUNT, _slotBits, TRUE);
      }
      _nameIndex.clear();
      _uidIndex.clear();
      _creatingCount = 0;
      return;
   }

   INT32 collectionSpaceContainer::createCS(requestContext *context,
                                            const strSlice &csName,
                                            utilCSUniqueID uniqueID,
                                            const createCSOptions &options,
                                            SPACE_ID *outSid,
                                            UINT32 *outLid)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      BOOLEAN locked = FALSE;
      collectionSpace *cs = NULL;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;

      if (OSS_UNLIKELY(NULL == context ||
                       context->getSpaceIDLocked() ||
                       csName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = precreateCS(context, csName, uniqueID, logicalID, sid);
      if (SDB_OK != rc)
      {
         goto error;
      }
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalID, "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");

      rc = context->lockSpaceID(sid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      rc = _slots[sid].allocate();
      if (SDB_OK != rc)
      {
         goto error;
      }

      cs = _slots[sid].getCS();

      rc = cs->create(context, csName, uniqueID,
                      logicalID, options);
      if (SDB_OK != rc)
      {
         goto error;
      }

      context->unlockSpaceID();
      endToCreateCS(context, csName, uniqueID, logicalID, sid);

      if (NULL != outSid)
      {
         *outSid = sid;
      }
      if (NULL != outSid)
      {
         *outLid = logicalID;
      }
   done:
      return rc;
   error:
      if (NULL != cs)
      {
         cs->destroy(context);
         _slots[sid].release();
      }
      if (locked)
      {
         context->unlockSpaceID();
      }
      if (INVALID_SPACE_ID != sid)
      {
         rollbackPrecreating(context, csName, uniqueID, logicalID, sid);
      }
      goto done;
   }

   UINT32 collectionSpaceContainer::getCSCount()
   {
      UINT32 cnt = 0;
      ossScopedLock(&_latch, SHARED);
      SDB_ASSERT(_creatingCount <= _nameIndex.size(), "can not be invalid");
      cnt = _nameIndex.size() - _creatingCount;

   done:
      return cnt;
   }

   INT32 collectionSpaceContainer::getCLCount(requestContext *context,
                                              const CHAR *csName,
                                              UINT32 &cnt)
   {
      INT32 rc = SDB_OK;
      strSlice nameslice(csName);
      collectionSpace *cs = NULL;
      rc = getCSByName(context, nameslice, SHARED, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      cnt = cs->getCollectionCount();
   done:
      if (NULL != cs)
      {
         context->unlockSpaceID();
      }
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContainer::listCollectionSpaces(requestContext *context,
                                                        listCSCursor *cursor)
   {
      INT32 rc = SDB_OK;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *obj = NULL;
      listCollectionSpaceRecord record;
      strSlice nameSlice;
      const static UINT32 BF_SIZE = DMS_COLLECTION_SPACE_NAME_SZ + 1;
      CHAR name[BF_SIZE] = {0};
      UINT32 loop = 0;
      static const UINT32 quitCheck = 16;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == cursor ||
                       !cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         if (loop++ == quitCheck)
         {
            if (context->getSession()->quit())
            {
               rc = SDB_APP_INTERRUPT;
               goto error;
            }
            loop = 0;
         }

         nameSlice.reset(cursor->getCSName());
         if (!upperBoundCSName(nameSlice, BF_SIZE, name, logicalID, sid))
         {
            cursor->pushEnd();
            break;
         }

         SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
         SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalID, "can not be invalid");
         
         rc = context->lockSpaceID(sid, SHARED);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = getCSByLockedSpaceID(context, logicalID, &obj);
         if (SDB_DMS_CS_NOTEXIST == rc)
         {
            /// cs dropped, just continue;
            context->unlockSpaceID();
            cursor->setLastName(name);
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            context->unlockSpaceID();
            goto error;
         }

         if (cursor->isPushed(logicalID))
         {
            context->unlockSpaceID();
            cursor->setLastName(name);
            continue;
         }

         rc = obj->dump(context, record);
         if (SDB_OK != rc)
         {
            context->unlockSpaceID();
            goto error;
         }
         context->unlockSpaceID();

         rc = cursor->push(sizeof(record), (const CHAR *)(&record));
         if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
         {
            rc = SDB_OK;
            goto done;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }
         else
         {
            cursor->markLIdPushed(logicalID);
            cursor->setLastName(name);
            if (cursor->hasNoSpaceToPush(sizeof(record)))
            {
               break;
            }
            continue;
         }
         
      } while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContainer::testCS(requestContext *context,
                                          const strSlice &nameSlice,
                                          utilCSUniqueID uniqueID,
                                          UINT32 &logicalID,
                                          SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      ossScopedLock(&_latch, SHARED);

      if (OSS_UNLIKELY(NULL == context ||
                       nameSlice.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         if (!getLIdAndSid(uniqueID, TRUE, logicalID, sid))
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
      }
      else
      {
         if (!getLIdAndSid(nameSlice, TRUE, logicalID, sid))
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
      }

      /// collection space is creating or removing.
      if (DMS_INVALID_LOGICCSID == logicalID)
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContainer::getCSByLockedSpaceID(requestContext *context,
                                                        UINT32 logicalID,
                                                        collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *tmp = NULL;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      sid = context->getSpaceID();
      if (_slots[sid].isFree())
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      tmp = _slots[sid].getCS();
      if (DMS_INVALID_LOGICCSID != logicalID &&
          logicalID != tmp->getLogicalID())
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      *obj = tmp;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContainer::getCSByName(requestContext *context,
                                               const strSlice &nameSlice,
                                               OSS_LATCH_MODE mode,
                                               collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      SPACE_ID sid = INVALID_SPACE_ID;
      BOOLEAN locked = FALSE;

      ossScopedLock(&_latch, SHARED);

      if (OSS_UNLIKELY(NULL == context ||
                       nameSlice.empty() ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!getLIdAndSid(nameSlice, TRUE, logicalID, sid))
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      rc = context->lockSpaceID(sid, mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      rc = getCSByLockedSpaceID(context, logicalID, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockSpaceID();
      }
      goto done;
   }

   INT32 collectionSpaceContainer::getCSByUniqueID(requestContext *context,
                                                   utilCSUniqueID uniqueID,
                                                   OSS_LATCH_MODE mode,
                                                   collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      SPACE_ID sid = INVALID_SPACE_ID;
      BOOLEAN locked = FALSE;

      ossScopedLock(&_latch, SHARED);

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!getLIdAndSid(uniqueID, TRUE, logicalID, sid))
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      rc = context->lockSpaceID(sid, mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      rc = getCSByLockedSpaceID(context, logicalID, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockSpaceID();
      }
      goto done;
   }

   INT32 collectionSpaceContainer::getCSBySpaceID(requestContext *context,
                                                  SPACE_ID sid,
                                                  UINT32 logicalID,
                                                  OSS_LATCH_MODE mode,
                                                  collectionSpace **obj)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_SPACE_ID == sid ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->lockSpaceID(sid, mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      rc = getCSByLockedSpaceID(context, logicalID, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockSpaceID();
      }
      goto done;
   }

   INT32 collectionSpaceContainer::getSUByLockedSpaceID(requestContext *context,
                                                        storageUnit **su)
   {
      INT32 rc = SDB_OK;
      collectionSpace *cs = NULL;
      if (OSS_UNLIKELY(NULL == su))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCSByLockedSpaceID(context, DMS_INVALID_LOGICCSID, &cs);
      if (SDB_OK != rc)
      {
         goto error;
      }

      *su = cs->getSU();
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContainer::getUnlockedSU(SPACE_ID sid,
                                                 storageUnit **su)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid || NULL == su))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_slots[sid].isFree())
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      *su = _slots[sid].getCS()->getSU();
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN collectionSpaceContainer::getLIdAndSid(const strSlice &csName,
                                                  BOOLEAN mustBeValid,
                                                  UINT32 &logicalID,
                                                  SPACE_ID &sid)
   {
      BOOLEAN r = FALSE;
      ossPoolString str(csName.str(), csName.strLen());
      NAME_INDEX::const_iterator itr = _nameIndex.find(str);
      if (_nameIndex.end() == itr)
      {
         goto done;
      }

      if (mustBeValid && DMS_INVALID_LOGICCSID == itr->second.logicalID)
      {
         goto done;
      }

      logicalID = itr->second.logicalID;
      sid = itr->second.sid;
      r = TRUE;
   done:
      return r;
   }
   BOOLEAN collectionSpaceContainer::getLIdAndSid(utilCSUniqueID uniqueID,
                                                  BOOLEAN mustBeValid,
                                                  UINT32 &logicalID,
                                                  SPACE_ID &sid)
   {
      BOOLEAN r = FALSE;
      UID_INDEX::const_iterator itr = _uidIndex.find(uniqueID);
      if (_uidIndex.end() == itr)
      {
         goto done;
      }

      if (mustBeValid && DMS_INVALID_LOGICCSID == itr->second.logicalID)
      {
         goto done;
      }

      logicalID = itr->second.logicalID;
      sid = itr->second.sid;
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN collectionSpaceContainer::upperBoundCSName(const strSlice &name,
                                                      UINT32 bufferSize,
                                                      CHAR *nextName,
                                                      UINT32 &nextLId,
                                                      SPACE_ID &nextSid)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT((DMS_COLLECTION_SPACE_NAME_SZ+1) <= bufferSize, "impossible");
      SDB_ASSERT(NULL != nextName, "can not be null");
      ossScopedLock lock(&_latch, SHARED);
      NAME_INDEX::const_iterator itr = _nameIndex.begin();
      if (!name.empty())
      {
         itr = _nameIndex.upper_bound(name.str());
      }

      while (_nameIndex.end() != itr)
      {
         if (DMS_INVALID_LOGICCSID == itr->second.logicalID)
         {
            ++itr;
            continue;
         }

         ossMemcpy(nextName, itr->first.c_str(), itr->first.size() + 1);
         nextLId = itr->second.logicalID;
         nextSid = itr->second.sid;
         r = TRUE;
         goto done;
      }
   done:
      return r;
   }

   INT32 collectionSpaceContainer::updateIndexWhenOpen(requestContext *context)
   {
      INT32 rc = SDB_OK;
      UINT32 count = _slotAllocated;
      for (UINT32 i = 0; i < MAX_SPACE_COUNT && 0 < count; ++i)
      {
         if (_slots[i].isFree())
         {
            continue;
         }

         collectionSpace *cs = _slots[i].getCS();
         rc = addToIndex(strSlice(cs->getCSName()),
                                  cs->getUniqueID(),
                                  cs->getSpaceID(),
                                  cs->getLogicalID());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add index when open:%d", rc);
            goto error;
         }
         --count;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContainer::loadCollectionSpacesOnDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      const storagePathOptions &path = context->getEnv()->options.path;
      fs::directory_iterator end_iter ;
      fs::path dataDir(path.dataPath);

      if (!fs::exists(dataDir) || !fs::is_directory(dataDir))
      {
         PD_LOG(PDERROR, "invalid data path:%s", path.dataPath.c_str());
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (fs::directory_iterator dir_iter(dataDir);
            dir_iter != end_iter; ++dir_iter)
      {
         std::string name = dir_iter->path().filename().string();
         strSlice nameSlice(name.c_str(), name.length());
         SPACE_ID sid = INVALID_SPACE_ID;
         spaceIDLockHelper lhelper(context);
         
         if (!fs::is_directory(dir_iter->status()))
         {
            continue;
         }

         if (!parseStorageUnitDir(nameSlice, &sid))
         {
            continue;
         }

         if (MAX_SPACE_ID < sid)
         {
            PD_LOG(PDERROR, "invalid sid:%d", sid);
            continue;
         }

         /// unnecessary locking. just page accessor required.
         rc = lhelper.lock(sid, SHARED);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get lock of space id[%d], rc:%d", sid, rc);
            goto error;
         }

         if (!_slots[sid].isFree())
         {
            PD_LOG(PDERROR, "space id[%d] is not free", sid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = _slots[sid].allocate();
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = _slots[sid].getCS()->open(context, _nextLogicalID, nameSlice);
         if (SDB_VESSEL_CRASHED_WHEN_CREATING == rc)
         {
            rc = SDB_OK;
            _slots[sid].release();
            PD_LOG(PDERROR, "crashed when creating su:%s", name.c_str());
             continue; /// recreate file when redo
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open cs[%s]:%d", name.c_str(), rc);
            goto error;
         }

         if (!setNotFreeIfFree32(MAX_SPACE_SLOT_COUNT, _slotBits, sid))
         {
            PD_LOG(PDERROR, "failed to update slot bits:%d", sid);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         lhelper.unlock();
         ++_slotAllocated;
         ++_nextLogicalID;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContainer::precreateCS(requestContext *context,
                                               const strSlice &csName,
                                               utilCSUniqueID uniqueID,
                                               UINT32 &logicalID,
                                               SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!csName.empty(), "can not be empty");
      ossScopedLock(&_latch, EXCLUSIVE);

      if (_nextLogicalID == DMS_INVALID_LOGICCSID)
      {
         PD_LOG(PDERROR, "logical cs id has hit the max value");
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }

      if (exists(csName, uniqueID))
      {
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }

      rc = allocateSpaceID(sid);
      if (SDB_OK != rc)
      {
         goto error;
      }

      /// add to index with invalid logical id.
      /// it will be set as valid value at last.
      addToIndex(csName, uniqueID, sid, DMS_INVALID_LOGICCSID);
      logicalID = _nextLogicalID++;
      ++_creatingCount;
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpaceContainer::endToCreateCS(requestContext *context,
                                                const strSlice &csName,
                                                utilCSUniqueID uniqueID,
                                                UINT32 logicalID,
                                                SPACE_ID sid)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!csName.empty(), "can not be empty");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalID, "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(0 < _creatingCount, "can not be zero");
      ossScopedLock(&_latch, EXCLUSIVE);
      upsertToIndex(csName, uniqueID, logicalID, sid);
      --_creatingCount;
      return;
   }

   void collectionSpaceContainer::rollbackPrecreating(requestContext *context,
                                                      const strSlice &csName,
                                                      utilCSUniqueID uniqueID,
                                                      UINT32 logicalID,
                                                      SPACE_ID sid)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalID, "can not be invalid");
      ossScopedLock(&_latch, EXCLUSIVE);

      removeFromIndex(csName, uniqueID);
      if (OSS_LIKELY(INVALID_SPACE_ID != sid))
      {
         releaseSpaceID(sid);
      }
      if (_nextLogicalID == logicalID + 1)
      {
         --_nextLogicalID;
      }
      return;
   }

   INT32 collectionSpaceContainer::addToIndex(const strSlice &csName,
                                              utilCSUniqueID uniqueID,
                                              SPACE_ID sid,
                                              UINT32 logicalID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!csName.empty(), "can not be empty");
      _LID_SID_PAIR usp(logicalID, sid);
      ossPoolString name(csName.str());

      if (!_nameIndex.insert(std::make_pair(name, usp)).second)
      {
         PD_LOG(PDERROR, "duplicated cs name[%s]", csName.str());
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }

      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         if (!_uidIndex.insert(std::make_pair(uniqueID, usp)).second)
         {
            _nameIndex.erase(name);
            PD_LOG(PDERROR, "duplicated cs unique id[%d]", uniqueID);
            rc = SDB_DMS_CS_EXIST;
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpaceContainer::upsertToIndex(const strSlice &csName,
                                                utilCSUniqueID uniqueID,
                                                SPACE_ID sid,
                                                UINT32 logicalID)
   {
      SDB_ASSERT(!csName.empty(), "can not be empty");
      _LID_SID_PAIR usp(logicalID, sid);
      ossPoolString str(csName.str(), csName.strLen());
      _nameIndex[str] = usp;
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         _uidIndex[uniqueID] = usp;
      }
      return;
   }

   BOOLEAN collectionSpaceContainer::exists(const strSlice &csName,
                                            utilCSUniqueID uniqueID)
   {
      SDB_ASSERT(!csName.empty(), "can not be empty");
      return 0 < _nameIndex.count(csName.str()) ||
             (UTIL_IS_VALID_CSUNIQUEID(uniqueID) &&
              0 < _uidIndex.count(uniqueID));
   }

   void collectionSpaceContainer::removeFromIndex(const strSlice &csName,
                                        utilCSUniqueID uniqueID)
   {
      _nameIndex.erase(csName.str());
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         _uidIndex.erase(uniqueID);
      }
      return;
   }

   INT32 collectionSpaceContainer::allocateSpaceID(SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      UINT32 offset = 0;
      SDB_ASSERT(isOpen(), "must be open");

      if (MAX_SPACE_COUNT == _slotAllocated)
      {
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }

      if (OSS_LIKELY(allocateFromBitMap32(MAX_SPACE_SLOT_COUNT, _slotBits, offset) &&
                     offset <= (UINT32)MAX_SPACE_ID))
      {
         ++_slotAllocated;
         sid = (SPACE_ID)offset;
      }
      else
      {
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpaceContainer::releaseSpaceID(SPACE_ID sid)
   {
      SDB_ASSERT(isOpen(), "must be open");
      if (OSS_LIKELY(INVALID_SPACE_ID != sid))
      {
         if (setFreeIfNotFree32(MAX_SPACE_SLOT_COUNT, _slotBits, sid))
         {
            --_slotAllocated;
         }
      }
      return;
   }

//////////collectionSpace::_spaceSlot

//   static const UINT32 SC_SPACE_SLOT_FLAG_NONE = 0;
//   static const UINT32 SC_SPACE_SLOT_FLAG_CREATING = 1;
//   static const UINT32 SC_SPACE_SLOT_FLAG_REMOVING = 2;

   collectionSpaceContainer::_spaceSlot::_spaceSlot():
   _cs(NULL)
   {}

   collectionSpaceContainer::_spaceSlot::~_spaceSlot()
   {
      release();
   }

   BOOLEAN collectionSpaceContainer::_spaceSlot::isFree()const
   {
      return NULL == _cs;
   }

   INT32 collectionSpaceContainer::_spaceSlot::allocate()
   {
      INT32 rc = SDB_OK;
      if (!isFree())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _cs = SDB_OSS_NEW collectionSpace();
      if (NULL == _cs)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpaceContainer::_spaceSlot::release()
   {
      SAFE_OSS_DELETE(_cs);
      return;
   }

}//namespace vessel
}//namespace engine