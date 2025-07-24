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

   Source File Name = dataManagementService.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/dataManagementService.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "vessel/requestContext.h"
#include "vessel/vesselOptions.h"
#include "vessel/collectionSpace.h"
#include "vessel/listCSCursor.h"
#include "vessel/instanceEnv.h"
#include "vessel/spaceIDLockHelper.h"
#include "vessel/storageFileName.h"
#include "vessel/storageFileLoader.h"
#include "vessel/storageUtils.h"
#include "dpsWriteReqBuilder.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace engine
{
namespace vessel
{
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
      ossPoolList<SPACE_ID> list;

      if (nullptr == context)
      {
         goto error;
      }

      _isOpen = TRUE;

      rc = _sus.init(MAX_SU_COUNT, 128);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init su array:%d", rc);
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
      _nextLogicalID = 0;
      _suAllocator.clear();

      for (auto itr = _mainIndex.begin();
           itr != _mainIndex.end(); ++itr)
      {
         storageUnit *su = itr->second->getSU();
         itr->second->close();
         su->close();
      }
      _mainIndex.clear();
      
      _sus.fini();
      _isOpen = FALSE;

      return;
   }

   INT32 dataManagementService::getLogicalPageSpace(SPACE_ID sid,
                                                    SPACE_TYPE type,
                                                    logicalPageSpace **lps)const
   {
      INT32 rc = SDB_OK;
      storageUnit *su  = nullptr;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (INVALID_SPACE_ID == sid ||
               INVALID_SPACE_TYPE == type ||
               nullptr == lps)
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

   INT32 dataManagementService::getLogicalPageSpace(SPACE_ID sid,
                                                    SPACE_TYPE type,
                                                    LPS_OBJ_PTR &out)const
   {
      INT32 rc = SDB_OK;
      out.reset();
      logicalPageSpace *ptr  = nullptr;
      rc = getLogicalPageSpace(sid, type, &ptr);
      if (SDB_OK != rc)
      {
         goto error;
      }

      out = LPS_OBJ_PTR(ptr);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::createCS(requestContext *context,
                                         const strSlice &csName,
                                         utilCSUniqueID uniqueID,
                                         const dmsCreateCSOptions &options,
                                         collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;
      SPACE_ID sid = INVALID_SPACE_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
      spaceIDLockHelper lh(context);
      _CS_UNIQUE_PTR obj;

      identifier.reset();

      if (OSS_UNLIKELY(nullptr == context ||
                       context->isSpaceIdLocked() ||
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

      /// preallocate: 
      /// 1. Ensure name and uid unique.
      /// 2. Allocate sid and logical id.
      /// 3. Add name and uid to tmp index.
      rc = reserveCSForCreating(csName, uniqueID, logicalID, sid);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
      identifier = collectionSpaceId(logicalID, uniqueID, sid);
      SDB_ASSERT(identifier.isValid(), "can not be invalid");

      rc = lh.lock(sid, EXCLUSIVE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock space id[%d], rc:%d", sid, rc);
         goto error;
      }

      rc = _createCS(context, csName, identifier, options, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cs obj[%s], rc:%d", csName.str(), rc);
         goto error;
      }

      /// Do not goto error from here.
      /// end:
      /// 1. Drop name and uid from tmp idex.
      /// 2. Add obj to formal index.
      _endToCreateCS(std::move(obj));
      lh.unlock();

      
   done:
      return rc;
   error:
      SDB_ASSERT(nullptr == obj, "must be null");
      lh.unlock();
      if (INVALID_SPACE_ID != sid)
      {
         clearReservedCSInfo(csName, uniqueID, logicalID, sid);
      }
      identifier.reset();
      goto done;
   }
   

   INT32 dataManagementService::createSU(requestContext *context,
                                         const collectionSpaceId &id,
                                         const dmsCreateCSOptions &options,
                                         storageUnit *su)
   {
      INT32 rc = SDB_OK;   
      SDB_ASSERT(_sus.isInitialized(), "must be inited");
      SDB_ASSERT(id.isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != su, "can not be invalid");
      
      SDB_ASSERT(DMS_PAGE_SIZE64K == options.dataPageSize, "must be 64KB");
      SDB_ASSERT(DMS_PAGE_SIZE64K == options.idxPageSize, "must be 64KB");
      SDB_ASSERT(DMS_PAGE_SIZE4K == options.lobdPageSize, "must be 4KB");

      createSUOptions suOptions;

      suOptions.dataArgs.pageSize = options.dataPageSize;
      suOptions.dataArgs.maxPageCountPerSeg = STORAGE_FILE_SEGMENT_SIZE_32MB / options.dataPageSize;
      suOptions.dataArgs.maxSegmentCountPerFile = DATA_STORAGE_FILE_SIZE / STORAGE_FILE_SEGMENT_SIZE_32MB;

      suOptions.indexArgs.pageSize = options.idxPageSize;
      suOptions.indexArgs.maxPageCountPerSeg = STORAGE_FILE_SEGMENT_SIZE_32MB / options.idxPageSize;
      suOptions.indexArgs.maxSegmentCountPerFile = DATA_STORAGE_FILE_SIZE / STORAGE_FILE_SEGMENT_SIZE_32MB;

      suOptions.lobArgs.pageSize = options.lobdPageSize;
      suOptions.lobArgs.maxPageCountPerSeg = STORAGE_FILE_SEGMENT_SIZE_32MB / options.lobdPageSize;
      suOptions.lobArgs.maxSegmentCountPerFile = DATA_STORAGE_FILE_SIZE / STORAGE_FILE_SEGMENT_SIZE_32MB;

      if (!suOptions.isValid())
      {
         PD_LOG(PDERROR, "invalid su options");
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = su->create(id, suOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create storage unit[%d], rc:%d", id.getSpaceId(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::_createCS(requestContext *context,
                                          const strSlice &csName,
                                          const collectionSpaceId &id,
                                          const dmsCreateCSOptions &options,
                                          std::unique_ptr<collectionSpace> &out)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(id.isValid(), "can not be invalid");

      storageUnit *su  = nullptr;

      IDataJournal *journal = context->getEnv()->resource.journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;
      dpsWriteOptions o;
      o.flushAtOnce = TRUE;
      out.reset();

      /// dummy log
      jpad.setType(LOG_TYPE_CS_CRT);
      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, o, &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      rc = _sus.ensure(id.getSpaceId(), &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure su obj:%d", rc);
         goto error;
      }

      rc = createSU(context, id, options, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create storage unit[%s], rc:%d",
                csName.str(), rc);
         goto error;
      }

      out.reset(SDB_OSS_NEW collectionSpace());
      if (!out)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = out->create(context, csName, su, jres._lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cs[%s], rc:%d",
                csName.str(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      out.reset();

      if (nullptr != su)
      {
         su->destroy();
         _sus.release(id.getSpaceId());
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
      ossRWMutexGuard guard(&_latch, SHARED, FALSE);
      UINT32 lid = DMS_INVALID_LOGICCSID;
      UINT32 count = 0;

      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == cursor ||
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
      
      guard.autoLock();
      lid = cursor->getFetched();

      do
      {
         collectionSpace *obj = _upperBound(lid);
         if (nullptr == obj)
         {
            break;
         }

         bson::BSONObj entry;
         rc = obj->dump(entry);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump cs info:%d", rc);
            goto error;
         }

         rc = cursor->pushData(entry.objsize(), entry.objdata());
         if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
         {
            rc = SDB_OK;
            SDB_ASSERT(0 < count, "impossible");
            break;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push data into cursor:%d", rc);
            goto error;
         }
         else
         {
            ++count;
            lid = obj->getLogicalID();
            cursor->setFetched(lid);
            if (cursor->noMorePushThisLoop())
            {
               break;
            }
         }
            
      } while (TRUE);

      if (0 == count)
      {
         cursor->setEOC();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::removeCS(requestContext *context,
                                         const collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;
      collectionSpace *space  = nullptr;
      OSS_LATCH_MODE mode = SHARED;
      storageUnit *su  = nullptr;
      utilCSUniqueID uniqueId = UTIL_UNIQUEID_NULL;
      CHAR csName[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {};
      strSlice csNameSlice;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !identifier.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!context->isSpaceIdLocked(&mode) ||
               EXCLUSIVE != mode ||
               context->getSpaceID() != identifier.getSpaceId())
      {
         rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
         goto error;
      }
      else
      {
         ossRWMutexGuard guard(&_latch, SHARED);
         space = _getCSByLid(identifier.getLid());
         if (nullptr == space)
         {
            PD_LOG(PDERROR, "failed to get cs obj[%d]", identifier.getLid());
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
      }

      SDB_ASSERT(nullptr != space, "can not be invalid");
      csNameSlice = space->getCSNameSlice();
      ossMemcpy(csName, csNameSlice.str(), csNameSlice.strLen());
      csNameSlice.reset(csName, csNameSlice.strLen());
      uniqueId = space->getUniqueID();
      PD_LOG(PDINFO, "begin to remove cs[%s, %d]", csNameSlice.str(), context->getSpaceID());

      /// do not goto error from here
      _prepareToDropCS(space);

      su = space->getSU();
      space->destroy(context);
      SDB_OSS_DEL space;
      space  = nullptr;

      rc = su->destroy();
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to destroy storage unit[%d], rc:%d",
                context->getSpaceID(), rc);
         /// do not goto error, just go on to clear last info.
         rc = SDB_OK;
      }

      _sus.release(context->getSpaceID());

      /// logical id will not be recycled when dropping.
      clearReservedCSInfo(csNameSlice, uniqueId,
                          DMS_INVALID_LOGICCSID,
                          context->getSpaceID());
      PD_LOG(PDINFO, "end to remove cs[%d]", context->getSpaceID());
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::testCS(const strSlice &nameSlice,
                                       collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;
      identifier.reset();

      if (OSS_UNLIKELY(nameSlice.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         ossScopedRWLock guard(&_latch, SHARED);
         collectionSpace *obj = _getCSByName(nameSlice);
         if (nullptr == obj)
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }

         identifier = obj->getIdentifier();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::testCS(utilCSUniqueID uniqueID,
                                       collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;
      identifier.reset();

      if (OSS_UNLIKELY(!UTIL_IS_VALID_CSUNIQUEID(uniqueID)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         ossScopedRWLock guard(&_latch, SHARED);
         collectionSpace *obj = _getCSByUid(uniqueID);
         if (nullptr == obj)
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }

         identifier = obj->getIdentifier();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::testCSByLid(UINT32 lid,
                                            collectionSpaceId &identifier)
   {
      INT32 rc = SDB_OK;
      identifier.reset();

      if (OSS_UNLIKELY(DMS_INVALID_LOGICCSID == lid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else
      {
         ossScopedRWLock guard(&_latch, SHARED);
         collectionSpace *obj = _getCSByLid(lid);
         if (nullptr == obj)
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }

         identifier = obj->getIdentifier();
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::getCSByCollectionSpaceId(requestContext *context,
                                                         const collectionSpaceId &id,
                                                         OSS_LATCH_MODE mode,
                                                         collectionSpace **out)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            context->isSpaceIdLocked() ||
                            !id.isValid() ||
                            nullptr == out))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else
      {
         rc = _lockAndGetCSByLid(context, id.getLid(), mode, out);
         if (SDB_OK != rc)
         {
            goto error;
         }

         SDB_ASSERT(id == (*out)->getIdentifier(), "must be same");
      }

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

      if (OSS_UNLIKELY(nullptr == context ||
                       nameSlice.empty() ||
                       nullptr == out))
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

      rc = _lockAndGetCSByName(context, nameSlice, mode, out);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::_lockAndGetCSByName(requestContext *context,
                                                    const strSlice &nameSlice,
                                                    OSS_LATCH_MODE mode,
                                                    collectionSpace **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(!nameSlice.empty(), "can not be empty");
      SDB_ASSERT(nullptr != out, "can not be null");
      SDB_ASSERT(!context->isSpaceIdLocked(), "can not be locked");
      *out = nullptr;

      do
      {
         SPACE_ID sid = INVALID_SPACE_ID;
         BOOLEAN sidLocked = FALSE;
         ossRWMutexGuard guard(&_latch, SHARED);
         collectionSpace *obj = _getCSByName(nameSlice);
         if (nullptr == obj)
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
         
         sid = obj->getSpaceId();
         rc = context->tryLockSpaceID(sid, mode, sidLocked);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to try lock sid[%d], :%d", sid, rc);
            goto error;
         }

         guard.autoUnlock();

         if (sidLocked)
         {
            *out = obj;
            break;
         }
         else
         {
            rc = context->lockSpaceID(sid, mode);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to lock sid[%d], :%d", sid, rc);
               goto error;
            }
            context->close();
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

      if (OSS_UNLIKELY(nullptr == context ||
                       !UTIL_IS_VALID_CSUNIQUEID(uniqueID) ||
                       nullptr == obj))
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

      rc = _lockAndGetCSByUid(context, uniqueID, mode, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::getCSByLogicalID(requestContext *context,
                                                 UINT32 logicalID,
                                                 OSS_LATCH_MODE mode,
                                                 collectionSpace **out)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(nullptr == context ||
                       DMS_INVALID_LOGICCSID == logicalID ||
                       nullptr == out))
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

      rc = _lockAndGetCSByLid(context, logicalID, mode, out);
      if (SDB_OK != rc)
      {
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::_lockAndGetCSByUid(requestContext *context,
                                                   utilCSUniqueID uniqueId,
                                                   OSS_LATCH_MODE mode,
                                                   collectionSpace **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(UTIL_IS_VALID_CSUNIQUEID(uniqueId), "can not be invalid");
      SDB_ASSERT(nullptr != out, "can not be null");
      SDB_ASSERT(!context->isSpaceIdLocked(), "can not be locked");
   
      do
      {
         SPACE_ID sid = INVALID_SPACE_ID;
         BOOLEAN sidLocked = FALSE;
         ossRWMutexGuard guard(&_latch, SHARED);
         collectionSpace *obj = _getCSByUid(uniqueId);
         if (nullptr == obj)
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
         
         sid = obj->getSpaceId();
         rc = context->tryLockSpaceID(sid, mode, sidLocked);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to try lock sid[%d], :%d", sid, rc);
            goto error;
         }

         guard.autoUnlock();

         if (sidLocked)
         {
            *out = obj;
            break;
         }
         else
         {
            rc = context->lockSpaceID(sid, mode);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to lock sid[%d], :%d", sid, rc);
               goto error;
            }
            context->close();
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::_lockAndGetCSByLid(requestContext *context,
                                                   UINT32 logicalId,
                                                   OSS_LATCH_MODE mode,
                                                   collectionSpace **out)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalId, "can not be invalid");
      SDB_ASSERT(nullptr != out, "can not be null");
      SDB_ASSERT(!context->isSpaceIdLocked(), "can not be locked");
   
      do
      {
         SPACE_ID sid = INVALID_SPACE_ID;
         BOOLEAN sidLocked = FALSE;
         ossRWMutexGuard guard(&_latch, SHARED);
         collectionSpace *obj = _getCSByLid(logicalId);
         if (nullptr == obj)
         {
            rc = SDB_DMS_CS_NOTEXIST;
            goto error;
         }
         
         sid = obj->getSpaceId();
         rc = context->tryLockSpaceID(sid, mode, sidLocked);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            PD_LOG(PDERROR, "failed to try lock sid[%d], :%d", sid, rc);
            goto error;
         }

         guard.autoUnlock();

         if (sidLocked)
         {
            *out = obj;
            break;
         }
         else
         {
            rc = context->lockSpaceID(sid, mode);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to lock sid[%d], :%d", sid, rc);
               goto error;
            }
            context->close();
         }
      } while (TRUE);
      
   done:
      return rc;
   error:
      goto done;
   }

   collectionSpace *dataManagementService::_getCSByUid(utilCSUniqueID uniqueID)
   {
      SDB_ASSERT(UTIL_IS_VALID_CSUNIQUEID(uniqueID), "can not be invalid");
      collectionSpace *obj = nullptr;
      auto itr = _uidIndex.find(uniqueID);
      if (_uidIndex.end() != itr)
      {
         obj = itr->second;
      }

      return obj;
   }

   collectionSpace *dataManagementService::_getCSByName(const strSlice &csName)
   {
      SDB_ASSERT(!csName.empty(), "can not be empty");
      collectionSpace *obj = nullptr;
      auto itr = _nameIndex.find(csName.str());
      if (_nameIndex.end() != itr)
      {
         obj = itr->second;
      }

      return obj;
   }

   collectionSpace *dataManagementService::_getCSByLid(UINT32 lid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCLID != lid, "can not be invalid");
      collectionSpace *obj  = nullptr;
      auto itr = _mainIndex.find(lid);
      if (_mainIndex.end() != itr)
      {
         obj = itr->second.get();
      }
      return obj;
   }

   collectionSpace *dataManagementService::_upperBound(UINT32 logicalId)
   {
      collectionSpace *obj = nullptr;
      if (DMS_INVALID_LOGICCSID == logicalId)
      {
         if (!_mainIndex.empty())
         {
            obj = _mainIndex.begin()->second.get();
         }
      }
      else
      {
         auto itr = _mainIndex.upper_bound(logicalId);
         if (_mainIndex.end() != itr)
         {
            obj = itr->second.get();
         }
      }

      return obj;
   }

   INT32 dataManagementService::loadCollectionSpaces(requestContext *context,
                                                     const ossPoolList<SPACE_ID> &sidList)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
      UINT32 maxLogicalId = DMS_INVALID_LOGICCSID;

      for (ossPoolList<SPACE_ID>::const_iterator itr = sidList.begin();
           itr != sidList.end(); ++itr)
      {
         spaceIDLockHelper lh(context);
         _CS_UNIQUE_PTR obj;

         storageUnit *su  = nullptr;
         rc = _sus.get(*itr, &su);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get storage unit obj[%d], rc:%d", *itr, rc);
            goto error;
         }

         obj.reset(SDB_OSS_NEW collectionSpace());
         if (!obj)
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

         if (DMS_INVALID_LOGICCSID == maxLogicalId ||
             maxLogicalId < obj->getLogicalID())
         {
            maxLogicalId = obj->getLogicalID();
         }

         rc = _insertIntoFormalIndex(std::move(obj));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to insert obj[%d] into index:%d", *itr, rc);
            goto error;
         }
      }

      _nextLogicalID = DMS_INVALID_LOGICCSID == maxLogicalId ?
                       0 : maxLogicalId + 1;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::loadStorageUnits(requestContext *context,
                                                 ossPoolList<SPACE_ID> &sidList)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be null");
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
         storageUnit *su  = nullptr;

         if (!fs::is_directory(dir_iter->status()))
         {
            continue;
         }

         if (!parseSpaceDirName(nameSlice, &sid))
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

         rc = su->open(sid);
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

      if (_suAllocator.size() <= sid)
      {
         _suAllocator.resize(sid + 1, TRUE);
      }

      if (!_suAllocator.test(sid))
      {
         PD_LOG(PDERROR, "sid[%d] already been occupied");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      _suAllocator.reset(sid);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 dataManagementService::reserveCSForCreating(const strSlice &csName,
                                                     utilCSUniqueID uniqueID,
                                                     UINT32 &logicalID,
                                                     SPACE_ID &sid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!csName.empty(), "can not be empty");
      UINT32 newSpaceId = 0;
      ossPoolString name;
      name.assign(csName.str(), csName.strLen());
      boost::dynamic_bitset<>::size_type t = boost::dynamic_bitset<>::npos;

      ossScopedRWLock guard(&_latch, EXCLUSIVE);

      if (_nextLogicalID == DMS_INVALID_LOGICCSID)
      {
         PD_LOG(PDERROR, "logical cs id has hit the max value");
         rc = SDB_DMS_SU_OUTRANGE;
         goto error;
      }

      if (0 < _nameIndex.count(csName.str()) ||
          0 < _unformalNameIndex.count(name))
      {
         rc = SDB_DMS_CS_EXIST;
         goto error;
      }
      else if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         if (0 < _uidIndex.count(uniqueID) ||
             0 < _unformalUidIndex.count(uniqueID))
         {
            rc = SDB_DMS_CS_EXIST;
            goto error;
         }
      }

      do
      {
         t = _suAllocator.find_first();
         if (t != boost::dynamic_bitset<>::npos)
         {
            newSpaceId = t;
            _suAllocator.reset(t);
            break;
         }
         else if (_suAllocator.size() == MAX_SU_COUNT)
         {
            PD_LOG(PDERROR, "hit the max count of available space");
            rc = SDB_VESSEL_OUT_OF_RESOURCE;
            goto error;
         }
         else
         {
            boost::dynamic_bitset<>::block_type block(0);
            _suAllocator.append(~block);
         }
      } while (TRUE);
      
      SDB_ASSERT(newSpaceId <= MAX_SPACE_ID, "impossible");
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         _unformalUidIndex.insert(uniqueID);
      }
      _unformalNameIndex.insert(std::move(name));
      logicalID = _nextLogicalID++;
      sid = newSpaceId;

   done:
      return rc;
   error:
      goto done;
   }

   void dataManagementService::_endToCreateCS(std::unique_ptr<collectionSpace> &&obj)
   {
      SDB_ASSERT(!_unformalNameIndex.empty(), "can not be empty");
      SDB_ASSERT(!!obj, "can not be null");
      const CHAR *name = obj->getCSName();
      UINT32 uniqueId = obj->getUniqueID();
      ossScopedRWLock guard(&_latch, EXCLUSIVE);
      _unformalNameIndex.erase(name);
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueId))
      {
         _unformalUidIndex.erase(uniqueId);
      }

      INT32 rc = _insertIntoFormalIndex(std::move(obj));
      if (SDB_OK != rc)
      {
         PD_LOG(PDSEVERE, "failed to insert obj[%s] to formal index:%d",
                name, rc);
         ossPanic();
      }
      
      return;
   }

   void dataManagementService::clearReservedCSInfo(const strSlice &csName,
                                                   utilCSUniqueID uniqueID,
                                                   UINT32 logicalID,
                                                   SPACE_ID sid)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      ossScopedRWLock guard(&_latch, EXCLUSIVE);
      SDB_ASSERT(sid < _suAllocator.size(), "out of bound");

      _unformalNameIndex.erase(csName.str());
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         _unformalUidIndex.erase(uniqueID);
      }
      _suAllocator.set(sid);
      if (DMS_INVALID_LOGICCSID != logicalID &&
          _nextLogicalID == (logicalID + 1))
      {
         --_nextLogicalID;
      }
      return;
   }


   INT32 dataManagementService::_insertIntoFormalIndex(std::unique_ptr<collectionSpace> &&obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!!obj, "can not be invalid");
      SDB_ASSERT(obj->isOpen(), "can not be invalid");

      SPACE_ID sid = obj->getSpaceId();
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      UINT32 uid = obj->getUniqueID();
      const CHAR *name = obj->getCSName();
      SDB_ASSERT(nullptr != name, "can not be null");
      BOOLEAN rollbackUid = FALSE;
      BOOLEAN rollbackName = FALSE;
      
      if (!_nameIndex.insert(std::make_pair(name, obj.get())).second)
      {
         PD_LOG(PDERROR, "duplicated name [%s]", name);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }
      rollbackName = TRUE;

      if (UTIL_IS_VALID_CSUNIQUEID(uid))
      {
         if (!_uidIndex.insert(std::make_pair(uid,  obj.get())).second)
         {
            PD_LOG(PDERROR, "duplicated unique id [%d]", uid);
            rc = SDB_VESSEL_DUPLICATED_KEY;
            goto error;
         }
      }
      rollbackUid = TRUE;

      if (!_mainIndex.insert(std::make_pair(obj->getLogicalID(), std::move(obj))).second)
      {
         PD_LOG(PDERROR, "duplicated space id[%d]", sid);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }
   done:
      return rc;
   error:
      if (rollbackName)
      {
         _nameIndex.erase(name);
      }
      if (rollbackUid)
      {
         _uidIndex.erase(uid);
      }
      goto done;
   }

   PAGE_SNAPSHOT_VERION dataManagementService::getOnlinePageSnapshotVersion()
   {
      return 1;
   }

   INT32 dataManagementService::isSnapshotEffective(SPACE_ID sid,
                                                    PAGE_SNAPSHOT_VERION psv,
                                                    BOOLEAN &effective)
   {
      effective = FALSE;
      return SDB_OK;
   }

   INT32 dataManagementService::getMmapPagePtr(const GLOBAL_PAGE_ID &gpid,
                                               mmapPagePointer &ptr,
                                               const UINT32 *pageSize)const
   {
      INT32 rc = SDB_OK;
      storageUnit *su  = nullptr;
      ptr.reset();

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

      rc = _sus.get(gpid.getSpaceId(), &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get storage unit[%d], rc:%d", gpid.space(), rc);
         goto error;
      }

      if (nullptr != pageSize)
      {
         UINT32 psz = su->getStoragePageSize(gpid.getSpaceType());
         if (psz != *pageSize)
         {
            PD_LOG(PDERROR, "page size[%d] does not match storage page size[%d",
                   *pageSize, psz);
            rc = SDB_INVALID_OPERATION;
            goto error;
         }
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

   storageFileCluster *dataManagementService::getLobdFileCluster(SPACE_ID sid)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can nto be invalid");
      SDB_ASSERT(isOpen(), "can not be invalid");
      storageFileCluster *fcluster = nullptr;
      storageUnit *su = getStorageUnit(sid);
      if (OSS_UNLIKELY(nullptr != su))
      {
         fcluster = su->getLobSpace().getFileCluster();
      }

      return fcluster;
   }

   storageFileCluster *dataManagementService::getStorageFileClsuter(SPACE_ID sid, SPACE_TYPE stype)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can nto be invalid");
      SDB_ASSERT(SPACE_TYPE_MAIN_DATA == stype || SPACE_TYPE_IDX == stype, "can not be invalid");
      SDB_ASSERT(isOpen(), "can not be invalid");

      storageFileCluster *dpc = nullptr;
      storageUnit *su = getStorageUnit(sid);
      if (OSS_LIKELY(nullptr != su))
      {
         dpc = SPACE_TYPE_MAIN_DATA == stype ?
               su->getMainDataSpace().getFileCluster() :
               su->getIndexSpace().getFileCluster();
      }

      return dpc;
   }

   storageUnit *dataManagementService::getStorageUnit(SPACE_ID sid)
   {
      SDB_ASSERT(INVALID_SPACE_ID != sid, "can not be invalid");
      SDB_ASSERT(isOpen(), "must be open");
      storageUnit *su  = nullptr;
      INT32 rc = _sus.get(sid, &su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get su[%d], rc:%d", sid, rc);
      }

      return su;
   }

   void dataManagementService::_prepareToDropCS(collectionSpace *obj)
   {
      SDB_ASSERT(nullptr != obj, "can not be empty");
      
      ossPoolString name(obj->getCSName());
      utilCSUniqueID uniqueID = obj->getUniqueID();
      UINT32 lid = obj->getLogicalID();
      
      ossScopedRWLock guard(&_latch, EXCLUSIVE);
      
      SDB_ASSERT(0 == _unformalNameIndex.count(name), "impossible");
      if (UTIL_IS_VALID_CSUNIQUEID(uniqueID))
      {
         SDB_ASSERT(0 == _unformalUidIndex.count(uniqueID), "impossible");
         _unformalUidIndex.insert(uniqueID);
         _uidIndex.erase(uniqueID);
      }

      _nameIndex.erase(name.c_str());
      _unformalNameIndex.insert(std::move(name));
      
      auto itr = _mainIndex.find(lid);
      SDB_ASSERT(_mainIndex.end() != itr, "impossible");
      itr->second.release();
      _mainIndex.erase(itr);
      
      return;
   }
}//namespace vessel
}//namespace engine