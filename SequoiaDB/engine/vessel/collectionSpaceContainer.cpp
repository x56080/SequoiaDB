
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
#include "vessel/IQueryFilter.h"
#include "vessel/spaceIDLockHelper.h"
#include "vessel/vesselFileName.h"
#include "vessel/storageUnit.h"



namespace engine
{
namespace vessel
{
   collectionSpaceContainer::collectionSpaceContainer()
   {

   }

   collectionSpaceContainer::~collectionSpaceContainer()
   {
      fini();
   }

   INT32 collectionSpaceContainer::openStorageUnits(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(CLOSED == _status, "do not reinit");
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _storageUnits.init(MAX_SPACE_COUNT, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init storage unit slots:%d", rc);
         goto error;
      }
      
      /// init slots of storageunit
      rc = loadStorageUnitsOnDisk(context);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _status = SU_LOADED;
   done:
      return rc;
   error:
      close();
      goto done;
   }

   INT32 collectionSpaceContainer::openCollectionSpaces(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(SU_LOADED == _status, "must be loaded");
      BOOLEAN rollback = FALSE;

      if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (SU_LOADED != _status)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rollback = TRUE;
      rc = _collectionSpaces.init(MAX_SPACE_COUNT, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init collection space slots:%d", rc);
         goto error;
      }

      for (SPACE_ID i = 0; i < MAX_SPACE_COUNT; ++i)
      {
         spaceIDLockHelper lh(context);
         collectionSpace *cs = NULL;
         storageUnit *su = _storageUnits.getObject(i);
         if (NULL == su)
         {
            _freeStorageUnits.push_back(i);
            continue;
         }

         if (!su->isOpen())
         {
            PD_LOG(PDERROR, "storage unit[%d] has not been open", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = _collectionSpaces.allocateNewObj(i, &cs);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate new cs obj:%d", rc);
            goto error;
         }

         /// unnecessary locking. just page accessor required.
         rc = lh.lock(i, SHARED);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = cs->open(context, su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open collection space[%d], rc:%d",
                   i, rc);
            goto error;
         }

         rc = addToIndex(strSlice(cs->getCSName()),
                                  cs->getUniqueID(),
                                  cs->getSpaceID(),
                                  cs->getLogicalID());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to add cs[%s] to index when open:%d",
                   cs->getCSName(), rc);
            goto error;
         }

         if (_nextLogicalID <= cs->getLogicalID())
         {
            _nextLogicalID = cs->getLogicalID() + 1;
         }

         lh.unlock();
      }

      _status = OPEN;
   done:
      return rc;
   error:
      if (rollback)
      {
         _nameIndex.clear();
         _uidIndex.clear();
         _collectionSpaces.fini();
         _freeStorageUnits.clear();
         _nextLogicalID = VESSEL_MIN_CS_LID;
      }
      goto done;
   }

   void collectionSpaceContainer::close()
   {
      if (_collectionSpaces.isInitialized())
      {
         for (UINT32 i = 0; i < _collectionSpaces.size(); ++i)
         {
            collectionSpace *cs = _collectionSpaces.getObject(i);
            if (NULL == cs)
            {
               continue;
            }
            cs->close();
         }
      }
      if (_storageUnits.isInitialized())
      {
         for (UINT32 i = 0; i < _storageUnits.size(); ++i)
         {
            storageUnit *su = _storageUnits.getObject(i);
            if (NULL == su)
            {
               continue;
            }
            su->close();
         }
      }
      fini();
   done:
      return;
   }

   void collectionSpaceContainer::fini()
   {
      _status = CLOSED;
      _nextLogicalID = VESSEL_MIN_CS_LID;
      _freeStorageUnits.clear();
      _storageUnits.fini();
      _collectionSpaces.fini();
      _nameIndex.clear();
      _uidIndex.clear();
      _creatingCount = 0;
      return;
   }

   void collectionSpaceContainer::finiOpenCS()
   {
      SDB_ASSERT(_storageUnits.isInitialized(), "impossible");
      SDB_ASSERT(0 == _creatingCount, "impossible");
      _nextLogicalID = VESSEL_MIN_CS_LID;
      _freeStorageUnits.clear();
      _collectionSpaces.fini();
      _nameIndex.clear();
      _uidIndex.clear();
      _status = SU_LOADED;
      return;
   }

   INT32 collectionSpaceContainer::createCS(requestContext *context,
                                            const strSlice &csName,
                                            const createCSOptions &options,
                                            SPACE_ID *outSid,
                                            UINT32 *outLid)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      storageUnit *su = NULL;
      spaceIDLockHelper lh(context);

      if (OSS_UNLIKELY(NULL == context ||
                       context->getSpaceIDLocked() ||
                       csName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = precreateCS(context, csName, options.uniqueID, logicalID, sid);
      if (SDB_OK != rc)
      {
         goto error;
      }
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalID, "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");

      rc = lh.lock(sid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = createSU(context, logicalID, options, &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create storage unit on disk of cs[%s], rc:%d",
                csName.str(), rc);
         goto error;
      }

      rc = createCS(context, su, csName, logicalID, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cs obj[%s], rc:%d", csName.str(), rc);
         goto error;
      }

      /// do not goto error from here.
      lh.unlock();
      endToCreateCS(context, csName, options.uniqueID, logicalID, sid);

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
      if (NULL != su)
      {
         su->destroy(context);
         _storageUnits.releaseObject(sid);
      }
      lh.unlock();
      if (INVALID_SPACE_ID != sid)
      {
         rollbackPrecreating(context, csName, options.uniqueID, logicalID, sid);
      }
      goto done;
   }

   INT32 collectionSpaceContainer::createSU(requestContext *context,
                                            UINT32 logicalID,
                                            const createCSOptions &options,
                                            storageUnit **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isSULoaded(), "must loaded");
      SDB_ASSERT(_storageUnits.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->getSpaceIDLockedMode() == EXCLUSIVE, "must holding lock");

      SPACE_ID sid = context->getSpaceID();
      storageUnit *su = NULL;
      createSUOptions suOptions;
      UINT32 segmentSize = 0;
      rc = _storageUnits.allocateNewObj(sid, &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate su[%d], rc:%d", sid, rc);
         goto error;
      }

      suOptions.sid = sid;
      suOptions.logicalID = logicalID;
      suOptions.metaArgs.pageSize = DMS_PAGE_SIZE32K;
      suOptions.metaArgs.maxPageCountPerSeg = 64;
      suOptions.metaArgs.maxSegmentCountPerFile = 2048;
      suOptions.idxMetaArgs.pageSize = DMS_PAGE_SIZE32K;
      suOptions.idxMetaArgs.maxPageCountPerSeg = 64;
      suOptions.idxMetaArgs.maxSegmentCountPerFile = 2048;

      segmentSize = options.dataSegSize;
      segmentSize = (segmentSize << 20);
      suOptions.dataArgs.pageSize = options.dataPageSize;
      suOptions.dataArgs.maxPageCountPerSeg = segmentSize / options.dataPageSize;
      suOptions.dataArgs.maxSegmentCountPerFile = STORAGE_FILE_SIZE / segmentSize;

      segmentSize = options.idxSegSize;
      segmentSize = (segmentSize << 20);
      suOptions.idxArgs.pageSize = options.idxPageSize;
      suOptions.idxArgs.maxPageCountPerSeg = segmentSize / options.idxPageSize;
      suOptions.idxArgs.maxSegmentCountPerFile = STORAGE_FILE_SIZE / segmentSize;

      rc = su->create(context, suOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create storage unit[%d], rc:%d", sid, rc);
         goto error;
      }
      
      if (NULL != out)
      {
         *out = su;
      }
   done:
      return rc;
   error:
      if (NULL != su)
      {
         _storageUnits.releaseObject(sid);
      }
      if (NULL != *out)
      {
         *out = NULL;
      }
      goto done;
   }

   INT32 collectionSpaceContainer::createCS(requestContext *context,
                                            storageUnit *su,
                                            const strSlice &csName,
                                            UINT32 logicalID,
                                            const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(EXCLUSIVE == context->getSpaceIDLockedMode(), "must holding lock");
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != su && su->isOpen(), "can not be null");
      SDB_ASSERT(_collectionSpaces.isInitialized(), "can not be invalid");
      collectionSpace *cs = NULL;
      SPACE_ID sid = context->getSpaceID();

      rc = _collectionSpaces.allocateNewObj(sid, &cs);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new obj:%d", rc);
         goto error;
      }

      rc = cs->create(context, csName, logicalID, su, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cs[%s], rc:%d",
                csName.str(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      if (NULL != cs)
      {
         _collectionSpaces.releaseObject(sid);
      }
      goto done;
   }

   UINT32 collectionSpaceContainer::getCSCount()
   {
      UINT32 cnt = 0;
      if (isOpen())
      {
         ossScopedLock(&_latch, SHARED);
         SDB_ASSERT(_creatingCount <= _nameIndex.size(), "can not be invalid");
         cnt = _nameIndex.size() - _creatingCount;
      }
      return cnt;
   }

   INT32 collectionSpaceContainer::getCLCount(requestContext *context,
                                              const CHAR *csName,
                                              UINT32 &cnt)
   {
      INT32 rc = SDB_OK;
      strSlice nameslice(csName);
      collectionSpace *cs = NULL;
      if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

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
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
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
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
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
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      sid = context->getSpaceID();
      tmp = _collectionSpaces.getObject(sid);
      if (NULL == tmp)
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

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

      {
      ossScopedLock guard(&_latch, SHARED);
      if (!getLIdAndSid(nameSlice, TRUE, logicalID, sid))
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
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

      {
      ossScopedLock guard(&_latch, SHARED);
      if (!getLIdAndSid(uniqueID, TRUE, logicalID, sid))
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
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

   INT32 collectionSpaceContainer::getStorageUnit(SPACE_ID sid,
                                                  storageUnit **su)
   {
      INT32 rc = SDB_OK;
      storageUnit *tmp = NULL;
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid || NULL == su))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isSULoaded())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      *su = NULL;
      tmp = _storageUnits.getObject(sid);
      if (NULL == tmp)
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      *su = tmp;
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



   INT32 collectionSpaceContainer::loadStorageUnitsOnDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_storageUnits.isInitialized(), "must be invalid");
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
         storageUnit *su = NULL;
         
         if (!fs::is_directory(dir_iter->status()))
         {
            continue;
         }

         if (!vesselFileName::parseDirName(nameSlice, &sid))
         {
            continue;
         }

         if (MAX_SPACE_ID < sid)
         {
            PD_LOG(PDERROR, "invalid sid:%d", sid);
            continue;
         }

         rc = _storageUnits.allocateNewObj(sid, &su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate su obj at[%d], rc:%d",
                   sid, rc);
            goto error;
         }

         rc = su->open(context, nameSlice);
         if (SDB_VESSEL_CRASHED_WHEN_CREATING == rc)
         {
            PD_LOG(PDERROR, "storage unit[%d] crashed when creating", sid);
            rc = SDB_OK;
            /// TODO
            _storageUnits.releaseObject(sid);
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open storage unit[%s], rc:%d",
                   nameSlice.str(), rc);
            goto error;
         }
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
      ossScopedLock guard(&_latch, EXCLUSIVE);
      upsertToIndex(csName, uniqueID, sid, logicalID);
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
      ossScopedLock guard(&_latch, EXCLUSIVE);

      removeFromIndex(csName, uniqueID);
      if (OSS_LIKELY(INVALID_SPACE_ID != sid))
      {
         releaseSpaceID(sid);
      }
      if (_nextLogicalID == logicalID + 1)
      {
         --_nextLogicalID;
      }
      --_creatingCount;
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

   BOOLEAN collectionSpaceContainer::allocateSpaceID(SPACE_ID &sid)
   {
      BOOLEAN r = FALSE;
      sid = INVALID_SPACE_ID;
      if (_freeStorageUnits.empty())
      {
         goto done;
      }
      sid = _freeStorageUnits.front();
      _freeStorageUnits.pop_front();
   done:
      return r;
   }

   void collectionSpaceContainer::releaseSpaceID(SPACE_ID sid)
   {
      if (OSS_LIKELY(INVALID_SPACE_ID != sid))
      {
         _freeStorageUnits.push_back(sid);
      }
      return;
   }
}//namespace vessel
}//namespace engine