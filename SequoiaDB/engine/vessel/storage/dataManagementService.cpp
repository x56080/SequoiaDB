
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

   Source File Name = dataManagementService.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/


#include "vessel/dataManagementService.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/vesselOptions.h"
#include "vessel/collectionSpace.h"
#include "vessel/listCSCursor.h"
#include "vessel/instanceEnv.h"
#include "vessel/api/IQueryFilter.h"
#include "vessel/spaceIDLockHelper.h"
#include "vessel/vesselFileName.h"
#include "vessel/storageFileLoader.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
   constexpr UINT32 ALLOCATOR_PAGE_CAPACITY_BITWISE = 9;
   constexpr UINT32 ALLOCATOR_PAGE_CAPACITY = ((UINT32)1 << ALLOCATOR_PAGE_CAPACITY_BITWISE);

   dataManagementService::dataManagementService()
   {

   }

   dataManagementService::~dataManagementService()
   {
      close();
   }

   INT32 dataManagementService::open(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      inMemBitmap::options o;
      ossPoolList<SPACE_ID> list;

      if (NULL == context)
      {
         goto error;
      }

      _isOpen = TRUE;

      rc = _sus.init(MAX_SU_COUNT, ALLOCATOR_PAGE_CAPACITY);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init su array:%d", rc);
         goto error;
      }

      o.bitmapBeginPage = 0;
      o.maxBitmapPageCount = MAX_SU_COUNT / ALLOCATOR_PAGE_CAPACITY;

      rc = _suAllocator.initWithNoLatch(ALLOCATOR_PAGE_CAPACITY, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init allocator:%d", rc);
         goto error;
      }

      rc = loadStorageUnits(context, list);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load storage units:%d", rc);
         goto error;
      }

      rc = loadCollectionSpaces(context, list);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load collection spaces:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void dataManagementService::close()
   {
      SDB_ASSERT(_unformalNameIndex.empty(), "should be empty");
      SDB_ASSERT(_unformalUidIndex.empty(), "should be empty");

      _unformalUidIndex.clear();
      _unformalNameIndex.clear();
      _nameIndex.clear();
      _uidIndex.clear();
      _nextLogicalID = VESSEL_MIN_CS_LID;
      _suAllocator.fini();

      for (_SPACE_ID_INDEX::const_iterator itr = _mainIndex.begin();
           itr != _mainIndex.end(); ++itr)
      {
         storageUnit *su = itr->second->getSU();
         itr->second->close();
         su->close();
         SDB_OSS_DEL itr->second;
      }
      _mainIndex.clear();
      
      _sus.fini();
      _isOpen = FALSE;
   done:
      return;
   }

   INT32 dataManagementService::getPageSize(SPACE_ID sid,
                                            SPACE_TYPE spaceType,
                                            FILE_TYPE fileType,
                                            UINT32 &pageSize)const
   {
      INT32 rc = SDB_OK;
      logicalPageSpace *lps = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_SPACE_ID == sid ||
               INVALID_SPACE_TYPE == spaceType ||
               INVALID_FILE_TYPE == fileType)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getLogicalPageSpace(sid, spaceType, &lps);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (fileType == lps->getStorageFileType())
      {
         pageSize = lps->getStorageCoreArgs().pageSize;
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::getLogicalPageSpace(SPACE_ID sid,
                                                    SPACE_TYPE type,
                                                    logicalPageSpace **lps)const
   {
      INT32 rc = SDB_OK;
      storageUnit *su = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_SPACE_ID == sid ||
               INVALID_SPACE_TYPE == type ||
               NULL == lps)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _sus.get(sid, &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get su of sid[%d], rc:%d", sid, rc);
         goto error;
      }

      if (SPACE_TYPE_MAIN_DATA == type)
      {
         *lps = &(su->getMainDataSpace());
      }
      else if (SPACE_TYPE_IDX == type)
      {
         *lps = &(su->getIndexSpace());
      }
      else
      {
         SDB_ASSERT(FALSE, "todo");
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::createCS(requestContext *context,
                                         const strSlice &csName,
                                         utilCSUniqueID uniqueID,
                                         const createCSOptions &options,
                                         collectionSpaceIdentifier &identifier)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      spaceIDLockHelper lh(context);
      collectionSpace *obj = NULL;

      identifier = collectionSpaceIdentifier();

      if (OSS_UNLIKELY(NULL == context ||
                       context->isSpaceIdLocked() ||
                       csName.empty() ||
                       !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      /// preallocate: 
      /// 1. Ensure name and uid unique.
      /// 2. Allocate sid and logical id.
      /// 3. Add name and uid to tmp index.
      rc = precreateCS(csName, uniqueID, logicalID, sid);
      if (SDB_OK != rc)
      {
         goto error;
      }
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalID, "can not be invalid");
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");

      /// Must lock first.
      /// "getCSByXX" may holding latch.
      rc = lh.lock(sid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space id[%d], rc:%d", sid, rc);
         goto error;
      }

      rc = createCS(context, csName, uniqueID, logicalID, options, &obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cs obj[%s], rc:%d", csName.str(), rc);
         goto error;
      }

      /// Do not goto error from here.
      /// end:
      /// 1. Drop name and uid from tmp idex.
      /// 2. Add obj to formal index.
      endToCreateCS(obj);
      identifier = collectionSpaceIdentifier(obj->getLogicalID(),
                                             obj->getUniqueID(),
                                             obj->getSpaceId());
      lh.unlock();

      
   done:
      return rc;
   error:
      SDB_ASSERT(NULL == obj, "must be null");
      lh.unlock();
      if (INVALID_SPACE_ID != sid)
      {
         rollbackPrecreating(csName, uniqueID, logicalID, sid);
      }
      goto done;
   }

   INT32 dataManagementService::createSU(requestContext *context,
                                         const createCSOptions &options,
                                         storageUnit **out)
   {
      INT32 rc = SDB_OK;
      OSS_LATCH_MODE lockingMode = SHARED;
      SDB_ASSERT(_sus.isInitialized(), "must be inited");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->isSpaceIdLocked(&lockingMode), "must holding lock");
      SDB_ASSERT(EXCLUSIVE == lockingMode, "must be exslusive");
      SDB_ASSERT(options.isValid(), "can not be invalid");

      SPACE_ID sid = context->getSpaceID();
      storageUnit *su = NULL;
      createSUOptions suOptions;

      suOptions.dataArgs.pageSize = options.dataPageSize;
      suOptions.dataArgs.maxPageCountPerSeg = options.dataSegSize / options.dataPageSize;
      suOptions.dataArgs.maxSegmentCountPerFile = STORAGE_FILE_SIZE / options.dataSegSize;

      suOptions.indexArgs.pageSize = options.idxPageSize;
      suOptions.indexArgs.maxPageCountPerSeg = options.idxSegSize / options.idxPageSize;
      suOptions.indexArgs.maxSegmentCountPerFile = STORAGE_FILE_SIZE / options.idxSegSize;

      suOptions.lobArgs.pageSize = options.lobPageSize;
      suOptions.lobArgs.maxPageCountPerSeg = options.lobSegSize / options.lobPageSize;
      /// single lobd file.
      suOptions.lobArgs.maxSegmentCountPerFile = DMS_MAX_PG / suOptions.lobArgs.maxPageCountPerSeg;

      if (!suOptions.isValid())
      {
         PD_LOG(PDERROR, "invalid su options");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _sus.ensure(sid, &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate storage unit obj:%d", rc);
         goto error;
      }

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
         _sus.release(sid);
      }
      goto done;
   }

   INT32 dataManagementService::createCS(requestContext *context,
                                         const strSlice &csName,
                                         utilCSUniqueID uniqueId,
                                         UINT32 logicalID,
                                         const createCSOptions &options,
                                         collectionSpace **out)
   {
      INT32 rc = SDB_OK;
      OSS_LATCH_MODE lockingMode = SHARED;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->isSpaceIdLocked(&lockingMode), "must holding lock");
      SDB_ASSERT(EXCLUSIVE == lockingMode, "must be exslusive");
      SDB_ASSERT(NULL != out, "can not be null");

      SPACE_ID sid = context->getSpaceID();
      collectionSpace *obj = NULL;
      storageUnit *su = NULL;
      rc = createSU(context, options, &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create storage unit[%s], rc:%d",
                csName.str(), rc);
         goto error;
      }

      obj = SDB_OSS_NEW collectionSpace();
      if (NULL == obj)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = obj->create(context, csName, uniqueId, logicalID, su, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cs[%s], rc:%d",
                csName.str(), rc);
         goto error;
      }

      *out = obj;

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(obj);

      if (NULL != su)
      {
         su->destroy(context);
         _sus.release(sid);
      }
      goto done;
   }

   UINT32 dataManagementService::getCSCount()
   {
      UINT32 cnt = 0;
      if (isOpen())
      {
         ossScopedRWLock guard(&_latch, SHARED);
         cnt = _mainIndex.size();
      }
      return cnt;
   }

   INT32 dataManagementService::listCollectionSpaces(requestContext *context,
                                                     listCSCursor *cursor)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj record;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       NULL == cursor ||
                       !cursor->isOpen() ||
                       context->isSpaceIdLocked()))
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
         UINT32 lid = DMS_INVALID_LOGICCSID;
         SPACE_ID sid = INVALID_SPACE_ID;
         collectionSpace *obj = NULL;
         strSlice nameSlice;
         BOOLEAN sidLocked = FALSE;
         nameSlice.reset(cursor->getCSName());

         if (!locked)
         {
            _latch.lock_r();
            locked = TRUE;
         }

         if (!upperBoundCS(nameSlice, lid, sid, &obj))
         {
            cursor->pushEnd();
            goto done;
         }

         rc = context->tryLockSpaceID(sid, SHARED, sidLocked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to try lock sid[%d], rc:%d", sid, rc);
            goto error;
         }

         if (sidLocked)
         {
            if (cursor->isPushed(obj->getLogicalID()))
            {
               cursor->setLastName(obj->getCSName());
               context->unlockSpaceID();
               continue;
            }

            rc = obj->dump(context, record);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to dump cs[%d] record, rc:%d", sid, rc);
               goto error;
            }
            else
            {
               rc = cursor->pushData(record.objsize(), record.objdata());
               if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
               {
                  rc = SDB_OK;
                  goto done;
               }
               else if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push record to cursor:%d", rc);
                  goto error;
               }
               else
               {
                  cursor->setLastName(obj->getCSName());
                  cursor->markLIdPushed(obj->getLogicalID());
                  context->unlockSpaceID();
                  if (!cursor->isWaitingMorePushing())
                  {
                     goto done;
                  }
               }
            }
         }
         else
         {
            _latch.release_r();
            locked = FALSE;
            rc = context->lockSpaceID(sid, SHARED);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d", sid, rc);
               goto error;
            }
            context->unlockSpaceID();
            continue;
         }
            
      } while (TRUE);
   done:
      if (context->isSpaceIdLocked())
      {
         context->unlockSpaceID();
      }
      if (locked)
      {
         _latch.release_r();
      }
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::testCS(requestContext *context,
                                       const strSlice &nameSlice,
                                       collectionSpaceIdentifier &identifier)
   {
      INT32 rc = SDB_OK;
      UINT32 lid = DMS_INVALID_LOGICCSID;
      SPACE_ID sid = INVALID_SPACE_ID;
      collectionSpace *obj = NULL;

      identifier = collectionSpaceIdentifier();

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

      {
      ossScopedRWLock guard(&_latch, SHARED);
      if (!testCS(nameSlice, lid, sid, &obj))
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      identifier = collectionSpaceIdentifier(lid, obj->getUniqueID(), sid);
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::testCS(requestContext *context,
                                       utilCSUniqueID uniqueId,
                                       UINT32 &logicalID,
                                       SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      

      if (OSS_UNLIKELY(NULL == context ||
                       !UTIL_IS_VALID_CSUNIQUEID(uniqueId)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      {
      ossScopedRWLock guard(&_latch, SHARED);
      if (!testCS(uniqueId, logicalID, sid, NULL))
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::getCSBySpaceID(requestContext *context,
                                               SPACE_ID sid,
                                               UINT32 logicalID,
                                               OSS_LATCH_MODE mode,
                                               collectionSpace **out)
   {
      INT32 rc = SDB_OK;
      collectionSpace *tmp = NULL;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            context->isSpaceIdLocked() ||
                            INVALID_SPACE_ID == sid ||
                            NULL == out))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = context->lockSpaceID(sid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space[%d], rc:%d", sid, rc);
         goto error;
      }
      locked = TRUE;

      {
      ossScopedRWLock guard(&_latch, SHARED);
      tmp = getCS(sid);
      if (NULL == tmp)
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
      }

      if (DMS_INVALID_LOGICCSID != logicalID &&
          logicalID != tmp->getLogicalID())
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      context->initSpaceContextUnderLock(tmp->getLogicalID(),
                                         strSlice(tmp->getCSName()));
      *out = tmp;
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockSpaceID();
      }
      goto done;
   }

   INT32 dataManagementService::getCSByLockedSpaceID(requestContext *context,
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
      else if (OSS_UNLIKELY(!context->isSpaceIdLocked()))
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
      {
      ossScopedRWLock guard(&_latch, SHARED);
      tmp = getCS(sid);
      if (NULL == tmp)
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }
      }

      if (DMS_INVALID_LOGICCSID != logicalID &&
          logicalID != tmp->getLogicalID())
      {
         rc = SDB_DMS_CS_NOTEXIST;
         goto error;
      }

      context->initSpaceContextUnderLock(tmp->getLogicalID(),
                                         strSlice(tmp->getCSName()));
      *obj = tmp;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::getCSByName(requestContext *context,
                                            const strSlice &nameSlice,
                                            OSS_LATCH_MODE mode,
                                            collectionSpace **out)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == context ||
                       nameSlice.empty() ||
                       NULL == out))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(context->isSpaceIdLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _getCSByName(context, nameSlice, mode, out);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::_getCSByName(requestContext *context,
                                             const strSlice &nameSlice,
                                             OSS_LATCH_MODE mode,
                                             collectionSpace **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!nameSlice.empty(), "can not be empty");
      SDB_ASSERT(NULL != out, "can not be null");
      SDB_ASSERT(!context->isSpaceIdLocked(), "can not be locked");

      ossRWMutexGuard guard(&_latch, SHARED, FALSE);
   
      do
      {
         collectionSpace *obj = NULL;
         SPACE_ID sid = INVALID_SPACE_ID;
         UINT32 lid = DMS_INVALID_LOGICCSID;
         BOOLEAN sidLocked = FALSE;
         
         guard.autoLock();

         if (!testCS(nameSlice, lid, sid, &obj))
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }

         rc = context->tryLockSpaceID(sid, mode, sidLocked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to try lock sid[%d], :%d", sid, rc);
            goto error;
         }

         guard.autoUnlock();

         if (sidLocked)
         {
            context->initSpaceContextUnderLock(obj->getLogicalID(),
                                               strSlice(obj->getCSName()));
            *out = obj;
            break;
         }
         else
         {
            obj = NULL;

            rc = getCSBySpaceID(context, sid, DMS_INVALID_LOGICCSID, mode, &obj);
            if (SDB_OK == rc)
            {
               if (0 == ossStrcmp(nameSlice.str(), obj->getCSName()))
               {
                  *out = obj;
                  goto done;
               }
               context->unlockSpaceID();
            }
            else if (SDB_DMS_CS_NOTEXIST == rc)
            {
               rc = SDB_OK;
               context->unlockSpaceID();
               continue;
            }
            else
            {
               context->unlockSpaceID();
               PD_LOG(PDERROR, "failed to get cs obj:%d", rc);
               goto error;
            }
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::getCSByUniqueID(requestContext *context,
                                                   utilCSUniqueID uniqueID,
                                                   OSS_LATCH_MODE mode,
                                                   collectionSpace **obj)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(NULL == context ||
                       !UTIL_IS_VALID_CSUNIQUEID(uniqueID) ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(context->isSpaceIdLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _getCSByUniqueId(context, uniqueID, mode, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::_getCSByUniqueId(requestContext *context,
                                                 utilCSUniqueID uniqueId,
                                                 OSS_LATCH_MODE mode,
                                                 collectionSpace **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(UTIL_IS_VALID_CSUNIQUEID(uniqueId), "can not be invalid");
      SDB_ASSERT(NULL != out, "can not be null");
      SDB_ASSERT(!context->isSpaceIdLocked(), "can not be locked");
      BOOLEAN locked = FALSE;
   
      do
      {
         collectionSpace *obj = NULL;
         SPACE_ID sid = INVALID_SPACE_ID;
         UINT32 lid = DMS_INVALID_LOGICCSID;
         BOOLEAN sidLocked = FALSE;
         _latch.lock_r();
         locked = TRUE;
         if (!testCS(uniqueId, lid, sid, &obj))
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }

         rc = context->tryLockSpaceID(sid, mode, sidLocked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to try lock sid[%d], :%d", sid, rc);
            goto error;
         }

         _latch.release_r();
         locked = FALSE;

         if (!sidLocked)
         {
            obj = NULL;
            rc = context->lockSpaceID(sid, mode);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d", sid, rc);
               goto error;
            }

            rc = getCSByLockedSpaceID(context, DMS_INVALID_LOGICCSID, &obj);
            if (SDB_OK == rc)
            {
               if (uniqueId == obj->getUniqueID())
               {
                  *out = obj;
                  goto done;
               }
               context->unlockSpaceID();
            }
            else if (SDB_DMS_CS_NOTEXIST == rc)
            {
               context->unlockSpaceID();
               rc = SDB_OK;
               continue;
            }
            else
            {
               context->unlockSpaceID();
               PD_LOG(PDERROR, "failed to get cs obj:%d", rc);
               goto error;
            }
         }

         *out = obj;
         break;
      } while (TRUE);
      
   done:
      if (locked)
      {
         _latch.release_r();
      }
      return rc;
   error:
      goto done;
   }

   BOOLEAN dataManagementService::testCS(utilCSUniqueID uniqueID,
                                         UINT32 &logicalID,
                                         SPACE_ID &sid,
                                         collectionSpace **obj)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(UTIL_IS_VALID_CSUNIQUEID(uniqueID), "can not be invalid");

      _UID_INDEX::const_iterator itr = _uidIndex.find(uniqueID);
      if (_uidIndex.end() == itr)
      {
         goto done;
      }

      logicalID = itr->second->getLogicalID();
      sid = itr->second->getSpaceId();
      if (NULL != obj)
      {
         *obj = itr->second;
      }
      r = TRUE;
   done:
      return r;
   }

   BOOLEAN dataManagementService::testCS(const strSlice &csName,
                                         UINT32 &logicalID,
                                         SPACE_ID &sid,
                                         collectionSpace **obj)const
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(!csName.empty(), "can not be empty");
      _NAME_INDEX::const_iterator itr = _nameIndex.find(csName.str());
      if (_nameIndex.end() == itr)
      {
         goto done;
      }

      logicalID = itr->second->getLogicalID();
      sid = itr->second->getSpaceId();
      if (NULL != obj)
      {
         *obj = itr->second;
      }
      r = TRUE;
   done:
      return r;
   }

   collectionSpace *dataManagementService::getCS(SPACE_ID sid)const
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      collectionSpace *obj = NULL;
      _SPACE_ID_INDEX::const_iterator itr = _mainIndex.find(sid);
      if (_mainIndex.end() != itr)
      {
         obj = itr->second;
      }
      return obj;
   }

   BOOLEAN dataManagementService::upperBoundCS(const strSlice &name,
                                               UINT32 &logicalID,
                                               SPACE_ID &sid,
                                               collectionSpace **obj)const
   {
      BOOLEAN r = FALSE;
      _NAME_INDEX::const_iterator itr = _nameIndex.upper_bound(name.str());
      if (_nameIndex.end() != itr)
      {
         logicalID = itr->second->getLogicalID();
         sid = itr->second->getSpaceId();
         if (NULL != obj)
         {
            *obj = itr->second;
         }
         r = TRUE;
      }
      return r;
   }

   INT32 dataManagementService::loadCollectionSpaces(requestContext *context,
                                                     const ossPoolList<SPACE_ID> &sidList)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      UINT32 maxLogicalId = VESSEL_MIN_CS_LID;
      collectionSpace *obj = NULL;

      for (ossPoolList<SPACE_ID>::const_iterator itr = sidList.begin();
           itr != sidList.end(); ++itr)
      {
         spaceIDLockHelper lh(context);
         collectionSpace *obj = NULL;
         storageUnit *su = NULL;
         rc = _sus.get(*itr, &su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get storage unit obj[%d], rc:%d", *itr, rc);
            goto error;
         }

         obj = SDB_OSS_NEW collectionSpace();
         if (NULL == obj)
         {
            PD_LOG(PDERROR, "failed to alloate mem");
            rc = SDB_OOM;
            goto error;
         }

         /// not necessary locking, just to
         /// ensure runtime latches can work.
         rc = lh.lock(*itr, EXCLUSIVE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d", *itr, rc);
            goto error;
         }

         rc = obj->open(context, su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open collection space[%d], rc:%d",
                   *itr, rc);
            goto error;
         }

         if (maxLogicalId < obj->getLogicalID())
         {
            maxLogicalId = obj->getLogicalID();
         }

         rc = insertIntoFormalIndex(obj);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert obj[%d] into index:%d", *itr, rc);
            goto error;
         }

         obj = NULL;
      }

      SDB_ASSERT(DMS_INVALID_LOGICCSID != maxLogicalId, "impossible");
      _nextLogicalID = maxLogicalId + 1;
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(obj);
      goto done;
   }

   INT32 dataManagementService::loadStorageUnits(requestContext *context,
                                                 ossPoolList<SPACE_ID> &sidList)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(_sus.isInitialized(), "must be invalid");
      SDB_ASSERT(sidList.empty(), "must be empty");
      const storagePathOptions &path = context->getEnv()->options.path;
      fs::directory_iterator end_iter ;
      fs::path dataDir(path.dataPath);
      spaceIDLockHelper lh(context);

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

         lh.lock(sid, EXCLUSIVE);

         rc = occupySpaceId(sid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to occupy space id[%d], rc:%d", sid, rc);
            goto error;
         }

         rc = _sus.ensure(sid, &su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to allocate su obj at[%d], rc:%d",
                   sid, rc);
            goto error;
         }

         rc = su->open(context);
         if (SDB_VESSEL_TEMP_SU == rc)
         {
            PD_LOG(PDERROR, "storage unit[%s] may crashed when creating/removing",
                   nameSlice.str());
            rc = SDB_OK;
            _sus.release(sid);
            continue;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to open storage unit[%s], rc:%d",
                   nameSlice.str(), rc);
            goto error;
         }

         sidList.push_back(sid);
         lh.unlock();
      }
   done:
      lh.unlock();
      return rc;
   error:
      sidList.clear();
      goto done;
   }

   INT32 dataManagementService::occupySpaceId(SPACE_ID sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(sid < MAX_SU_COUNT, "can not be invalid");
      SDB_ASSERT(_suAllocator.isInitialized(), "must be inited");

      UINT32 minPageCount = (sid >> ALLOCATOR_PAGE_CAPACITY_BITWISE) + 1;
      rc = _suAllocator.ensureBitmapPageCount(minPageCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure allocator's page count:%d", rc);
         goto error;
      }

      rc = _suAllocator.occupy(sid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy sid[%d], rc:%d", sid, rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::precreateCS(const strSlice &csName,
                                            utilCSUniqueID uniqueID,
                                            UINT32 &logicalID,
                                            SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!csName.empty(), "can not be empty");
      UINT32 newSpaceId = 0;
      BOOLEAN rollbackName = FALSE;
      BOOLEAN rollbackUid = FALSE;
      ossScopedRWLock guard(&_latch, EXCLUSIVE);

      if (_nextLogicalID == DMS_INVALID_LOGICCSID)
      {
         PD_LOG(PDERROR, "logical cs id has hit the max value");
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }

      if (0 < _nameIndex.count(csName.str()))
      {
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }
      else if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         if (0 < _uidIndex.count(uniqueID))
         {
            rc = SDB_DMS_CS_EXIST;
            goto error;
         }
      }

      if (!_unformalNameIndex.insert(csName.str()).second)
      {
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }
      rollbackName = TRUE;

      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         if (!_unformalUidIndex.insert(uniqueID).second)
         {
            rc = SDB_DMS_CS_EXIST;
            goto error;
         }
         rollbackUid = TRUE;
      }

      rc = _suAllocator.allocateBits(1, &newSpaceId, 1);
      if (SDB_OK == rc)
      {
         SDB_ASSERT(newSpaceId <= MAX_SPACE_ID, "impossible");
      }
      else if (SDB_VESSEL_OUT_OF_RESOURCE == rc)
      {
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }
      else
      {
         PD_LOG(PDERROR, "failed to allocate space id:%d", rc);
         goto error;
      }

      logicalID = _nextLogicalID++;
      sid = newSpaceId;
   done:
      return rc;
   error:
      if (rollbackUid)
      {
         _unformalUidIndex.erase(uniqueID);
      }
      if (rollbackName)
      {
         _unformalNameIndex.erase(csName.str());
      }
      goto done;
   }

   void dataManagementService::endToCreateCS(collectionSpace *obj)
   {
      SDB_ASSERT(!_unformalNameIndex.empty(), "can not be empty");
      SDB_ASSERT(NULL != obj, "can not be null");
      const CHAR *name = obj->getCSName();
      UINT32 uniqueId = obj->getUniqueID();
      ossScopedRWLock guard(&_latch, EXCLUSIVE);
      _unformalNameIndex.erase(name);
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueId))
      {
         _unformalUidIndex.erase(uniqueId);
      }

      INT32 rc = insertIntoFormalIndex(obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to insert obj[%s] to formal index:%d",
                name, rc);
         ossPanic();
      }
      
      return;
   }

   void dataManagementService::rollbackPrecreating(const strSlice &csName,
                                                   utilCSUniqueID uniqueID,
                                                   UINT32 logicalID,
                                                   SPACE_ID sid)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalID, "can not be invalid");
      ossScopedRWLock guard(&_latch, EXCLUSIVE);

      _unformalNameIndex.erase(csName.str());
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         _unformalUidIndex.erase(uniqueID);
      }
      _suAllocator.release(sid);
      if (_nextLogicalID == (logicalID + 1))
      {
         --_nextLogicalID;
      }
      return;
   }


   INT32 dataManagementService::insertIntoFormalIndex(collectionSpace *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != obj, "can not be null");
      SDB_ASSERT(obj->isOpen(), "must be open");
      SPACE_ID sid = obj->getSpaceId();
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      UINT32 uid = obj->getUniqueID();
      const CHAR *name = obj->getCSName();
      SDB_ASSERT(NULL != name, "can not be null");
      BOOLEAN rollbackMain = FALSE;
      BOOLEAN rollbackName = FALSE;
      
      if (!_mainIndex.insert(std::make_pair(sid, obj)).second)
      {
         PD_LOG(PDERROR, "duplicated space id[%d]", sid);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }
      rollbackMain = TRUE;

      if (!_nameIndex.insert(std::make_pair(name, obj)).second)
      {
         PD_LOG(PDERROR, "duplicated name [%s]", name);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }
      rollbackName = TRUE;

      if (UTIL_IS_VALID_CSUNIQUEID(uid))
      {
         if (!_uidIndex.insert(std::make_pair(uid, obj)).second)
         {
            PD_LOG(PDERROR, "duplicated unique id [%d]", uid);
            rc = SDB_VESSEL_DUPLICATED_KEY;
            goto error;
         }
      }
   done:
      return rc;
   error:
      if (rollbackName)
      {
         _nameIndex.erase(name);
      }
      if (rollbackMain)
      {
         _mainIndex.erase(sid);
      }
      goto done;
   }

   void dataManagementService::removeFromFormalIndex(collectionSpace *obj)
   {
      UINT32 logicalId = obj->getLogicalID();
      UINT32 uid = obj->getUniqueID();
      const CHAR *name = obj->getCSName();
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalId, "can not be invalid");
      SDB_ASSERT(NULL != name, "can not be null");
      if (UTIL_IS_VALID_CSUNIQUEID(uid))
      {
         _uidIndex.erase(uid);
      }
      _nameIndex.erase(name);
      _mainIndex.erase(logicalId);
      return;
   }

   PAGE_SNAPSHOT_VERION dataManagementService::getOnlinePageSnapshotVersion()
   {
      return 0;
   }

   INT32 dataManagementService::isSnapshotEffective(SPACE_ID sid,
                                                    PAGE_SNAPSHOT_VERION psv,
                                                    BOOLEAN &effective)
   {
      effective = FALSE;
      return SDB_OK;
   }

   INT32 dataManagementService::getMmapPagePtr(const GLOBAL_PAGE_ID &gpid,
                                               mmapPagePointer &ptr)const
   {
      INT32 rc = SDB_OK;
      storageUnit *su = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(!gpid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _sus.get(gpid.space(), &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get storage unit[%d], rc:%d", gpid.space(), rc);
         goto error;
      }

      rc = su->getMmapPagePointer(gpid.getSpaceType(),
                                  gpid.getFileType(),
                                  gpid.page(),
                                  ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::createCheckpointBeforeClosing(requestContext *context)
   {
      INT32 rc = SDB_OK;
      UINT32 count = 0;
      spaceIDLockHelper lh(context);

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      for (_SPACE_ID_INDEX::const_iterator itr = _mainIndex.begin();
           itr != _mainIndex.end(); ++itr)
      {
         rc = lh.lock(itr->first, SHARED);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock space[%d], rc:%d", itr->first, rc);
            rc = SDB_OK;
            continue;
         }

         rc = itr->second->createCheckpoint(context, FALSE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "collection space[%d] failed to create checkpoint:%d",
                   itr->first, rc);
            ++count;
            rc = SDB_OK;
         }

         lh.unlock();
      }

      if (0 != count)
      {
         PD_LOG(PDERROR, "total [%d] collection spaces failed to create checkpoint", count);
      }

      rc = SDB_OK;
   done:
      return rc;
   error:
      goto done;
   }

    storageUnit *dataManagementService::getStorageUnit(SPACE_ID sid)
    {
       SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
       SDB_ASSERT(isOpen(), "must be open");
       storageUnit *su = NULL;
       INT32 rc = _sus.get(sid, &su);
       if (SDB_OK != rc)
       {
          PD_LOG(PDERROR, "failed to get su[%d], rc:%d", sid, rc);
       }

       return su;
    }
}//namespace vessel
}//namespace engine