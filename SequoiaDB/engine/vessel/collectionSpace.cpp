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

   Source File Name = collectionSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/collectionSpace.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "ossUtil.hpp"
#include "vessel/requestContext.h"
#include "vessel/instanceEnv.h"
#include "vessel/storageUnit.h"
#include "dpsLogRecord.hpp"
#include "vessel/collection.h"
#include "vessel/listCLCursor.h"
#include "vessel/clMetaBlockPageIniter.h"
#include "vessel/csMetaBlockPageAccessor.h"
#include "vessel/storageFileMaintainer.h"
#include "vessel/csMetaBlockPageIniter.h"
#include "vessel/csIndexMetaStorage.h"
#include "ixm_common.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 ALLOCATOR_PAGE_CAPAITY = 512;

   collectionSpace::collectionSpace()
   {
      
   }

   collectionSpace::~collectionSpace()
   {
      fini();
   }

   INT32 collectionSpace::create(requestContext *context,
                                 const strSlice &name,
                                 storageUnit *su,
                                 DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj optionsObj = bson::BSONObj();
      slice optionsSlice;
      OSS_LATCH_MODE mode = SHARED;
      csMetaBlock mb;
      SDB_ASSERT(!isOpen(), "do not recreate");
      
      if (OSS_UNLIKELY(nullptr == context ||
                       !context->isSpaceIdLocked(&mode) ||
                       EXCLUSIVE != mode ||
                       DMS_COLLECTION_SPACE_NAME_SZ < name.strLen() ||
                       name.empty() ||
                       nullptr == su ||
                       !su->isOpen() ||
                       DPS_INVALID_LSN_OFFSET == lsn))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;
      _su = su;

      rc = initInMemStructures();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem structures:%d", rc);
         goto error;
      }

      mb.version = CS_META_BLOCK_VERSION_1;
      mb.status = CS_STATUS_ONLINE;
      mb.type = CS_TYPE_NORMAL;
      mb.flags = 0;
      ossMemcpy(mb.name, name.str(), name.strLen() + 1);

      rc = _initMetaBlock(context, mb, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init meta block on disk:%d", rc);
         goto error;
      }

      _initProperties(mb);

      createCSNameFile();
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void collectionSpace::close()
   {
      fini();
      return;
   }

   INT32 collectionSpace::open(requestContext *context,
                               storageUnit *su)
   {
      INT32 rc = SDB_OK;
      csMetaBlock mb;
      
      SDB_ASSERT(!isOpen(), "do not reopen");
      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == su ||
                       !su->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == su->getSpaceID(), "must be same");

      _isOpen = TRUE;
      _su = su;

      rc = _loadMetaBlock(context, mb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to load meta block:%d", rc);
         goto error;
      }
      else if (!mb.isValid())
      {
         PD_LOG(PDERROR, "invalid meta block loaded");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = initInMemStructures();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem structures:%d", rc);
         goto error;
      }

      _initProperties(mb);

      rc = _loadMaxIndexLid();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "load max index logical id failed, rc:%d", rc);
         goto error;
      }

      rc = initCollectionsFromDisk(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init collections from disk:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      close();
      goto done;
   }

   void collectionSpace::fini()
   {
      _isOpen = FALSE;
      _su = nullptr;
      _properties.reset();
      _allocator.clearAll();
      _collections.fini();
      _clNameIndex.clear();
      _innerIdIndex.clear();
      _unformalNameIndex.clear();
      _unformalInnerIdIndex.clear();
      _maxIndexLid = INVALID_LOGICAL_INDEX_ID;
      return;
   }

   void collectionSpace::destroy(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      if (isOpen())
      {
         OSS_LATCH_MODE mode = SHARED;
         SDB_ASSERT(context->isSpaceIdLocked(&mode) && EXCLUSIVE == mode, "can not be invalid");
         SDB_ASSERT(context->getSpaceID() == getSpaceId(), "must be same");
         context->getEnv()->ioBufferPool.discard(getSpaceId());
         context->getEnv()->lobcBufferPool.discard(getSpaceId());

         if (INVALID_LOGICAL_INDEX_ID != _maxIndexLid)
         {
            csIndexMetaStorage ms(getLogicalID());
            rc = ms.destroy();
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "destroy cs[%d] index meta data failed, rc:%d",
                     getLogicalID(), rc);
            }
         }
         fini();
      }
      
      return;
   }

   INT32 collectionSpace::createCL(requestContext *context,
                                   const strSlice &clName, 
                                   utilCLInnerID clInnerId,
                                   const createCLOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      CL_MB_ID mbID = INVALID_CL_MB_ID;
      UINT32 logicalID = DMS_INVALID_LOGICCLID;
      _CL_HOLDER *holder = nullptr;
      collection *obj = nullptr;
      BOOLEAN locked = FALSE;
      
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            clName.empty() ||
                            DMS_COLLECTION_NAME_SZ < clName.strLen() ||
                            !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = reserveCL(context, clName, clInnerId, mbID, logicalID);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = getCollectionHolder(mbID, &holder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get collection holder:%d", rc);
         goto error;
      }
      SDB_ASSERT(nullptr != holder && !holder->isFree(), "impossible");

      context->lockMB(mbID, holder->getMutexPtr(), EXCLUSIVE);
      locked = TRUE;
      obj = holder->get();
      SDB_ASSERT(nullptr != obj, "impossible");

      rc = ensureCLMetaBlockPage(context, mbID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure cl meta block page:%d", rc);
         goto error;
      }

      rc = obj->create(context, clName, clInnerId,
                       logicalID, this, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cl[%s] on disk:%d", clName.str(), rc);
         holder->free();
         goto error;
      }

      context->unlockMB();
      locked = FALSE;
      endCreatingCL(obj);
      
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockMB();
      }
      if (INVALID_CL_MB_ID != mbID)
      {
         releaseCL(clName, clInnerId, mbID);
      }
      goto done;
   }

   INT32 collectionSpace::removeCL(requestContext *context,
                                   const collectionId &identifier)
   {
      INT32 rc = SDB_OK;
      collection *cl = nullptr;
      ossPoolString clName;
      strSlice clNameSlice;
      utilCLInnerID innerId = UTIL_UNIQUEID_NULL;
      CL_MB_ID mbID = INVALID_CL_MB_ID;
      BOOLEAN locked = FALSE;

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

      SDB_ASSERT(context->isSpaceIdLocked(), "must be locked");
      SDB_ASSERT(!context->isMbLocked(), "can not be locked");

      rc = getCollectionById(context, identifier, EXCLUSIVE, &cl);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      clName.assign(cl->getName());
      clNameSlice.reset(clName.c_str(), clName.size());
      innerId = cl->getInnerID();
      mbID = context->getMBID();

      prepareToRemoveCL(cl);

      rc = cl->destroy(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to remove cl[%s], rc:%d", clName.c_str(), rc);
         SDB_ASSERT(FALSE, "TOOD");
         //goto error;
      }
      cl = nullptr;

      releaseCL(clNameSlice, innerId, mbID);
   done:
      if (locked)
      {
         context->unlockMB();
      }
      return rc;
   error:
      goto done;
   }

   UINT32 collectionSpace::getCollectionCount()
   {
      ossSLatchGuard guard(&_latch, SHARED);
      return _clNameIndex.size();
   }

   INT32 collectionSpace::listCollections(requestContext *context,
                                          listCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(!context->isMbLocked(), "can not be locked");
      UINT32 count = 0;
      
      if (OSS_UNLIKELY(nullptr == context ||
                       nullptr == cursor))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!cursor->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         bson::BSONObj entry;
         _CL_HOLDER *holder = nullptr;
         ossSLatchGuard guard(&_latch, SHARED);
         CL_MB_ID mbID = _upperBoundCL(cursor->getScanned());
         if (INVALID_CL_MB_ID == mbID)
         {
            break;
         }

         rc = getCollectionHolder(mbID, &holder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get cl holder[%d], rc:%d", mbID, rc);
            goto error;
         }

         rc = holder->get()->dump(entry);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to dump collection:%d", rc);
            goto error;
         }

         rc = cursor->pushData(entry.objsize(), entry.objdata());
         if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
         {
            SDB_ASSERT(0 < count, "impossible");
            rc = SDB_OK;
            break;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to push data into cursor:%d", rc);
            goto error;
         }

         cursor->setScanned(mbID);
         ++count;
         if (cursor->noMorePushThisLoop())
         {
            break;
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

   INT32 collectionSpace::dump(bson::BSONObj &record)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      record = dumpCollectionSpace(getSpaceId(),
                                   getCSName(),
                                   getUniqueID(),
                                   getLogicalID(),
                                   getStatus(),
                                   _su->getManifest().dataArgs.pageSize,
                                   _su->getManifest().dataArgs.getSegmentSize(),
                                   0,0,0,0);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getCollectionByName(requestContext *context,
                                              const strSlice &clName, 
                                              OSS_LATCH_MODE mode,
                                              collection **obj)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            clName.empty() ||
                            nullptr == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      *obj = nullptr;

      do
      {
         BOOLEAN mbLocked = FALSE;
         _CL_HOLDER *holder = nullptr;
         ossSLatchGuard guard(&_latch, SHARED);
         CL_MB_ID mbID = _findCLByName(clName);
         if (INVALID_CL_MB_ID == mbID)
         {
            rc = SDB_DMS_NOTEXIST;
            goto error;
         }

         rc = getCollectionHolder(mbID, &holder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get cl holder[%d], rc:%d", mbID, rc);
            goto error;
         }

         mbLocked = context->tryLockMB(mbID, holder->getMutexPtr(), mode);
         guard.unlock();

         if (mbLocked)
         {
            SDB_ASSERT(!holder->isFree(), "impossible");
            *obj = holder->get();
            break;
         }
         else
         {
            if (SHARED == mode)
            {
               holder->mutex().lock_r();
               holder->mutex().release_r();
            }
            else
            {
               holder->mutex().lock_w();
               holder->mutex().release_w();
            }
            continue;
         }
      } while (TRUE);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getCollectionByCLInnerID(requestContext *context,
                                                   utilCLInnerID innerID,
                                                   OSS_LATCH_MODE mode,
                                                   collection **obj)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !UTIL_IS_VALID_CL_INNERID(innerID) ||
                            nullptr == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      *obj = nullptr;

      do
      {
         BOOLEAN mbLocked = FALSE;
         _CL_HOLDER *holder = nullptr;
         ossSLatchGuard guard(&_latch, SHARED);
         CL_MB_ID mbID = _findCLByInnerId(innerID);
         if (INVALID_CL_MB_ID == mbID)
         {
            rc = SDB_DMS_NOTEXIST;
            goto error;
         }

         rc = getCollectionHolder(mbID, &holder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get cl holder[%d], rc:%d", mbID, rc);
            goto error;
         }

         mbLocked = context->tryLockMB(mbID, holder->getMutexPtr(), mode);
         guard.unlock();

         if (mbLocked)
         {
            SDB_ASSERT(!holder->isFree(), "impossible");
            *obj = holder->get();
            break;
         }
         else
         {
            if (SHARED == mode)
            {
               holder->mutex().lock_r();
               holder->mutex().release_r();
            }
            else
            {
               holder->mutex().lock_w();
               holder->mutex().release_w();
            }
            continue;
         }
      } while (TRUE);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getCollectionByLogicalId(requestContext *context,
                                                   UINT32 logicalId,
                                                   OSS_LATCH_MODE mode,
                                                   collection **obj)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            DMS_INVALID_LOGICCLID == logicalId ||
                            nullptr == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      *obj = nullptr;

      do
      {
         BOOLEAN mbLocked = FALSE;
         _CL_HOLDER *holder = nullptr;
         ossSLatchGuard guard(&_latch, SHARED);
         CL_MB_ID mbID = _findCLByLogicalId(logicalId);
         if (INVALID_CL_MB_ID == mbID)
         {
            rc = SDB_DMS_NOTEXIST;
            goto error;
         }

         rc = getCollectionHolder(mbID, &holder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get cl holder[%d], rc:%d", mbID, rc);
            goto error;
         }

         mbLocked = context->tryLockMB(mbID, holder->getMutexPtr(), mode);
         guard.unlock();

         if (mbLocked)
         {
            SDB_ASSERT(!holder->isFree(), "impossible");
            *obj = holder->get();
            break;
         }
         else
         {
            if (SHARED == mode)
            {
               holder->mutex().lock_r();
               holder->mutex().release_r();
            }
            else
            {
               holder->mutex().lock_w();
               holder->mutex().release_w();
            }
            continue;
         }
      } while (TRUE);

   done:
      return rc;
   error:
      goto done;   
   }

   INT32 collectionSpace::getCollectionById(requestContext *context,
                                             const collectionId &id,
                                             OSS_LATCH_MODE mode,
                                             collection **obj)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      _CL_HOLDER *holder = nullptr;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(nullptr == context ||
                            !id.isValid() ||
                            context->isMbLocked() ||
                            nullptr == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionHolder(id.getMbId(), &holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      context->lockMB(id.getMbId(), holder->getMutexPtr(), mode);
      locked = TRUE;

      if (holder->isFree())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      if (id.getLid() != holder->get()->getLogicalID() ||
          id.getInnerId() != holder->get()->getInnerID())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      SDB_ASSERT(holder->get()->isOpen(), "impossible");

      *obj = holder->get();
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockMB();
      }
      goto done;
   }

   INT32 collectionSpace::initCollectionsFromDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _su, "can not be null");

      UINT32 clmbpCapacity = 0;
      const storageCoreArgs &args = _su->getManifest().dataArgs;
      mainDataSpace *mds = &(_su->getMainDataSpace());
      clMetaBlock block;
      UINT32 totalCLmbpCount = 0;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      rc = getCapacityOfCLMetaBlockPage(args.pageSize, clmbpCapacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of cl meta block page:%d", rc);
         goto error;
      }

      totalCLmbpCount = MAX_CL_MB_COUNT / clmbpCapacity;
      if (0 != MAX_CL_MB_COUNT % clmbpCapacity)
      {
         ++totalCLmbpCount;
      }

      for (UINT32 i = 0; i < totalCLmbpCount; ++i)
      {
         PAGE_ID lpid = CL_META_BLOCK_PAGE_MIN_LPID + i;
         logicalPageBuffer lpb;
         lpageDescriptor desc;

         rc = mds->testLogicalPageMapping(lpid, desc);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to test if lpid[%d] mapped:%d", lpid, rc);
            goto error;
         }
         else if (!desc.isValid())
         {
            continue;
         }

         rc = mds->getLogicalPageBuffer(context, lpid, mode, lpb);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d", lpid, rc);
            goto error;
         }

         rc = lpb.validatePage(PAGE_TYPE_CL_META);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "cl meta block page[%d] may be broken:%d", lpid, rc);
            goto error;
         }

         for (UINT32 j = 0; j < clmbpCapacity; ++j)
         {
            UINT32 tmp = i * clmbpCapacity + j;
            if (MAX_CL_MB_COUNT <= tmp)
            {
               break;
            }

            if (!getCLMetaBlockIfValid(lpb.getRuntimeBuffer().getPageHead(),
                                       j, block))
            {
               continue;
            }

            rc = initCollection(context, &block);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to init collection[%d]:%d", i, rc);
               goto error;
            }
         }
      }

   done:
      return rc;
   error:
      _clNameIndex.clear();
      _innerIdIndex.clear();
      goto done;
   }

   INT32 collectionSpace::initCollection(requestContext *context,
                                         const clMetaBlock *block)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != block, "can not be null");
      SDB_ASSERT(block->isValid(), "can not be invalid");
      SDB_ASSERT(!context->isMbLocked(), "can not be locked");

      _CL_HOLDER *holder = nullptr;
      collection *cl = nullptr;

      rc = ensureCLObj(block->mbID, &holder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure collection holder:%d", rc);
         goto error;
      }

      context->lockMB(block->mbID, &holder->mutex(), EXCLUSIVE);
      cl = holder->get();
      SDB_ASSERT(nullptr != cl, "impossible");
      rc = cl->initWhenOpen(context, *block, this);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cl obj:%d", rc);
         goto error;
      }

      rc = insertIntoFormalIndexes(cl);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert cl obj into index:%d", rc);
         goto error;
      }

   done:
      context->unlockMB();
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::ensureCLObj(CL_MB_ID mbID,
                                      _CL_HOLDER **holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(_collections.isInitialized(), "must be inited");

      _CL_HOLDER_GROUP *group = nullptr;
      UINT32 i = mbID / _CL_HOLDER_GROUP::CAPACITY;
      INT32 pos = mbID % _CL_HOLDER_GROUP::CAPACITY;
      _CL_HOLDER *h = nullptr;

      if (nullptr != holder)
      {
         *holder = nullptr;
      }

      rc = _collections.ensure(i, &group);
      if (SDB_OK != rc)
      {
         goto error;
      }

      h = group->getPtr(pos);
      if (nullptr == h->ensure())
      {
         PD_LOG(PDERROR, "failed to ensure cl obj:%d", rc);
         goto error;
      }

      group->clear(pos);
      if (group->none())
      {
         _allocator.clear(i);
      }

      if (nullptr != holder)
      {
         *holder = h;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getCollectionHolder(CL_MB_ID mbID,
                                              _CL_HOLDER **holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(nullptr != holder, "can not be null");
      SDB_ASSERT(_collections.isInitialized(), "must be inited");

      _CL_HOLDER_GROUP *group = nullptr;
      UINT32 i = mbID / _CL_HOLDER_GROUP::CAPACITY;
      UINT32 pos = mbID % _CL_HOLDER_GROUP::CAPACITY;

      rc = _collections.get(i, &group);
      if (SDB_VESSEL_RESOURCES_NOT_INIT == rc)
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }
      else if (SDB_OK != rc)
      {
         goto error;
      }

      *holder = group->getPtr(pos);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::ensureCLMetaBlockPage(requestContext *context,
                                                CL_MB_ID mbID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      atomicOperationList *oplist = nullptr;
      clMetaBlockPageIniter initer;
      BOOLEAN locked = FALSE;
      mainDataSpace &mds = _su->getMainDataSpace();
      UINT32 pageSize = _su->getManifest().dataArgs.pageSize;
      PAGE_ID lpid = getMbpLpidOfCollection(pageSize, mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to get cl meta block page lpid of mb[%d]", mbID);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = context->lockLpid(mds.getSpaceType(), lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get exlusive latch of lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      locked = TRUE;

      if (context->isInProcessingOplist())
      {
         context->swtichOplist(NULL, &oplist);
      }

      rc = mds.ensureReservedPageMapped(context, lpid, &initer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure lpid[%d] mapped:%d", lpid, rc);
         goto error;
      }
   done:
      if (nullptr != oplist)
      {
         context->attachOplist(oplist);
      }
      if (locked)
      {
         context->unlockLpid(mds.getSpaceType(), lpid);
      }
      return rc;
   error:
      goto done; 
   }

   INT32 collectionSpace::reserveCL(requestContext *context,
                                    const strSlice &clName,
                                    utilCLInnerID innerID,
                                    CL_MB_ID &mbID,
                                    UINT32 &logicalID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(nullptr != context, "can not be null");
      SDB_ASSERT(!clName.empty(), "can not be empty");
      SDB_ASSERT(nullptr != _su, "can not be null");

      csMetaBlock mb;
      logicalID = DMS_INVALID_LOGICCLID;
      mbID = INVALID_CL_MB_ID;

      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      if ((_properties.maxCLLogicalID + 1) == DMS_INVALID_LOGICCLID)
      {
         PD_LOG(PDERROR, "logical id hits the max value");
         rc = SDB_DMS_NOSPC;
         goto error;
      }
      
      if (existsInFormalIndexes(clName, innerID))
      {
         PD_LOG(PDDEBUG, "collection name[%s] or inner id[%d] duplicated",
                clName.str(), innerID);
         rc = SDB_DMS_EXIST;
         goto error;
      }
      else if (existsInUnformalIndexes(clName, innerID))
      {
         PD_LOG(PDDEBUG, "collection name[%s] or inner id[%d] duplicated in unformal indexes",
                clName.str(), innerID);
         rc = SDB_DMS_EXIST;
         goto error;
      }

      rc = reserveCLObj(mbID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve cl obj:%d", rc);
         goto error;
      }
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "impossible");
      
      ++_properties.maxCLLogicalID;
      _exportMetaBlock(mb);
      rc = _updateMetaBlock(context, mb, MAX_CL_LOGICAL_ID);
      if (SDB_OK != rc)
      {
         --_properties.maxCLLogicalID;
         PD_LOG(PDERROR, "failed to set max cl logical id on cs meta block, rc:%d", rc);
         goto error;
      }
      logicalID = _properties.maxCLLogicalID;

      ///WARNING: do not goto error from here.
      _unformalNameIndex.insert(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _unformalInnerIdIndex.insert(innerID);
      }

   done:
      return rc;
   error:
      if (INVALID_CL_MB_ID != mbID)
      {
         releaseCLObj(mbID);
         mbID = INVALID_CL_MB_ID;
      }
      goto done;
   }

   void collectionSpace::releaseCL(const strSlice &clName,
                                   utilCLInnerID innerID,
                                   CL_MB_ID mbID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");

      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      _unformalNameIndex.erase(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _unformalInnerIdIndex.erase(innerID);
      }
      releaseCLObj(mbID);
      return;
   }

   void collectionSpace::endCreatingCL(collection *obj)
   {
      SDB_ASSERT(nullptr != obj, "can not be null");
      strSlice nameSlice;
      nameSlice.reset(obj->getName());
      SDB_ASSERT(!nameSlice.empty(), "can not be empty");

      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      _unformalNameIndex.erase(nameSlice.str());
      if (UTIL_IS_VALID_CL_INNERID(obj->getInnerID()))
      {
         _unformalInnerIdIndex.erase(obj->getInnerID());
      }
      INT32 rc = insertIntoFormalIndexes(obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to insert cl[%s] into indexes:%d",
                nameSlice.str(), rc);
         ossPanic();
      }
      return;
   }


   INT32 collectionSpace::initInMemStructures()
   {
      INT32 rc = SDB_OK;
      static_assert(65535 == MAX_CL_MB_COUNT, "must be 65535");

      static constexpr UINT32 CHUNK_SIZE = 16;
      static constexpr UINT32 CAPACITY = (MAX_CL_MB_COUNT +1) / _CL_HOLDER_GROUP::CAPACITY;

      _allocator.setAll();
      rc = _collections.init(CAPACITY, CHUNK_SIZE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init collection array:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::insertIntoFormalIndexes(collection *obj)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != obj, "can not be null");
      const CHAR *name = obj->getName();
      utilCLInnerID innerId = obj->getInnerID();
      UINT32 logicalId = obj->getLogicalID();
      if (!_clNameIndex.insert(std::make_pair(name, obj->getMBID())).second)
      {
         PD_LOG(PDERROR, "duplicated cl name:%s", name);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      if (UTIL_IS_VALID_CL_INNERID(innerId))
      {
         if (!_innerIdIndex.insert(std::make_pair(innerId, obj->getMBID())).second)
         {
            _clNameIndex.erase(name);
            PD_LOG(PDERROR, "duplicated inner id:%d", innerId);
            rc = SDB_VESSEL_DUPLICATED_KEY;
            goto error;
         }
      }

      if (!_lidIndex.insert(std::make_pair(logicalId, obj->getMBID())).second)
      {
         _clNameIndex.erase(name);
         if (UTIL_IS_VALID_CL_INNERID(innerId))
         {
            _innerIdIndex.erase(innerId);
         }
         PD_LOG(PDERROR, "duplicated logical id[%d]", logicalId);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error; 
      }

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN collectionSpace::existsInFormalIndexes(const strSlice &clName,
                                                  utilCLInnerID innerID)const
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      BOOLEAN r = FALSE;
      if (0 < _clNameIndex.count(clName.str()))
      {
         r = TRUE;
         goto done;
      }
      else if (UTIL_IS_VALID_CL_INNERID(innerID) &&
               0 < _innerIdIndex.count(innerID))
      {
         r = TRUE;
         goto done;
      }
   done:
      return r;
   }

   BOOLEAN collectionSpace::existsInUnformalIndexes(const strSlice &clName,
                                                    utilCLInnerID innerID)const
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      BOOLEAN r = FALSE;
      if (_unformalNameIndex.count(clName.str()))
      {
         r = TRUE;
         goto done;
      }
      else if (UTIL_IS_VALID_CL_INNERID(innerID) &&
               0 < _unformalInnerIdIndex.count(innerID))
      {
         r = TRUE;
         goto done;
      }
   done:
      return r;
   }
   
   INT32 collectionSpace::createCSNameFile()const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _su && _su->isOpen(), "can not be invalid");
      SDB_ASSERT(!_properties.name.empty(), "can not be empty");
   
      ossPoolString fullPath;
      OSSFILE file;
      CHAR nameBuffer[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {};
      strSlice nameSlice(_properties.name.c_str(), _properties.name.size());
      UINT32 flags = OSS_READWRITE|OSS_EXCLUSIVE|OSS_REPLACE;

      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _su->getSpaceID());

      fullPath = sfm.buildFullPath(SPACE_TYPE_MAIN_DATA, CSNAME_FILE_NAME);
      if (OSS_UNLIKELY(fullPath.empty()))
      {
         PD_LOG(PDERROR, "failed to build full path of name file");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemcpy(nameBuffer, nameSlice.str(), nameSlice.strLen());
      nameBuffer[nameSlice.strLen()] = '\n';

      rc = ossOpen(fullPath.c_str(), flags, OSS_RU|OSS_WU|OSS_RG, file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create file[%s], rc:%d", fullPath.c_str(), rc);
         goto error;
      }

      rc = ossWriteN(&file, nameBuffer, nameSlice.strLen() + 1);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write file[%s], rc:%d", fullPath.c_str(), rc);
         goto error;
      }

      rc = ossFdatasync(&file);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to fdatasync file[%s], rc:%d", fullPath.c_str(), rc);
         goto error;
      }

      ossClose(file);
      ossChmod(fullPath.c_str(), OSS_RU);
   done:
      return rc;
   error:
      if (file.isOpened())
      {
         ossClose(file);
         ossDelete(fullPath.c_str());
      }
      goto done;
   }

   INT32 collectionSpace::removeCSNameFile()const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != _su && _su->isOpen(), "can not be invalid");

      ossPoolString fullPath;
      const storagePathOptions &po = GET_THREAD_CONTEXT()->getEnv()->options.path;
      storageFileMaintainer sfm(&po, _su->getSpaceID());

      fullPath = sfm.buildFullPath(SPACE_TYPE_MAIN_DATA, CSNAME_FILE_NAME);

      if (!fullPath.empty())
      {
         PD_LOG(PDINFO, "removing cs name file:%s", fullPath.c_str());
         rc = ossDelete(fullPath.c_str());
         if (SDB_FNE == rc)
         {
            rc = SDB_OK;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to remove file:%s, rc:%d", fullPath.c_str(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpace::prepareToRemoveCL(collection *cl)
   {
      SDB_ASSERT(nullptr != cl, "can not be invalid");
      ossSLatchGuard guard(&_latch, EXCLUSIVE);

      BOOLEAN res = FALSE;
      res = _unformalNameIndex.insert(cl->getName()).second;
      SDB_ASSERT(res, "must be true");
      _clNameIndex.erase(cl->getName());

      if (UTIL_IS_VALID_CL_INNERID(cl->getInnerID()))
      {
         res = _unformalInnerIdIndex.insert(cl->getInnerID()).second;
         SDB_ASSERT(res, "must be true");
         _innerIdIndex.erase(cl->getInnerID());
      }

      _lidIndex.erase(cl->getLogicalID());
   }

   INT32 collectionSpace::reserveCLObj(CL_MB_ID &mbID)
   {
      INT32 rc = SDB_OK;
      INT32 groupId = -1;
      _CL_HOLDER_GROUP *group = nullptr;
      INT32 pos = -1;
      _CL_HOLDER *holder = nullptr;
      mbID = INVALID_CL_MB_ID;

      do
      {
         groupId = _allocator.findFirst();
         if (groupId < 0)
         {
            PD_LOG(PDERROR, "no free mb id any more");
            rc = SDB_VESSEL_OUT_OF_MBID_RESOURCE;
            goto error;
         }
         else
         {
            rc = _collections.ensure(groupId, &group);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure holder group:%d", rc);
               goto error;
            }

            pos = group->findFirst();
            if (pos < 0)
            {
               _allocator.clear(groupId);
               group = nullptr;
               groupId = -1;
               continue;
            }
            else if (static_cast<UINT32>(groupId + 1)== _allocator.getSize() &&
                     static_cast<UINT32>(pos + 1) == _CL_HOLDER_GROUP::CAPACITY)
            {
               /// the last holder can not be reserved.
               /// it is out of valid mb id space(65535)
               _allocator.clear(groupId);
               PD_LOG(PDERROR, "no free mb id any more");
               rc = SDB_VESSEL_OUT_OF_MBID_RESOURCE;
               goto error;
            }
            else
            {
               holder = group->getPtr(pos);
               if (nullptr == holder->ensure())
               {
                  PD_LOG(PDERROR, "failed to allocate mem.");
                  rc = SDB_OOM;
                  goto error;
               }

               group->clear(pos);
               if (group->none())
               {
                  _allocator.clear(groupId);
               }
               break;
            }
         }
      } while (TRUE);

      SDB_ASSERT(0 <= pos && 0 <= groupId, "can not be invalid");
      mbID = (groupId * _CL_HOLDER_GROUP::CAPACITY) + pos;
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpace::releaseCLObj(CL_MB_ID mbID)
   {
      if (OSS_LIKELY(INVALID_CL_MB_ID != mbID))
      {
         INT32 groupId = mbID / _CL_HOLDER_GROUP::CAPACITY;
         INT32 pos = mbID % _CL_HOLDER_GROUP::CAPACITY;
         _CL_HOLDER_GROUP *group = nullptr;
         INT32 rc = _collections.get(groupId, &group);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get holder group[%d], rc:%d", groupId, rc);
            goto done;
         }

         if (!_allocator.test(groupId))
         {
            _allocator.set(groupId);
         }
         group->getPtr(pos)->free();
         group->set(pos);
      }
      else
      {
         SDB_ASSERT(FALSE, "invalid mb id");
      }

   done:
      return;
   }

   INT32 collectionSpace::_initMetaBlock(requestContext *context,
                                         const csMetaBlock &block,
                                         DPS_LSN_OFFSET lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(DPS_INVALID_LSN_OFFSET != lsn, "can not be invalid");
      SDB_ASSERT(nullptr != _su && _su->isOpen(), "can not be invalid");

      constexpr PAGE_ID lpid = CS_META_BLOCK_PAGE_LPID;
      csMetaBlockPageIniter initer;
      initer.set(&block);
      initer.setLSN(lsn);
      mainDataSpace &mds = _su->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      BOOLEAN locked = FALSE;

      rc = context->lockLpid(SPACE_TYPE_MAIN_DATA, lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock meta block lpid:%d", rc);
         goto error;
      }
      locked = TRUE;

      rc = mds.ensureReservedPageMapped(context, lpid, &initer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure meta block page:%d", rc);
         goto error;
      }
   done:
      if (locked)
      {
         context->unlockLpid(SPACE_TYPE_MAIN_DATA, lpid);
      }
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::_updateMetaBlock(requestContext *context,
                                           const csMetaBlock &block,
                                           UINT64 mask)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(nullptr != _su && _su->isOpen(), "can not be invalid");

      constexpr PAGE_ID lpid = CS_META_BLOCK_PAGE_LPID;
      logicalPageBuffer lpb;
      csMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _su->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = accessor.update(context, &lpb, block, mask);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to update meta block:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::_loadMetaBlock(requestContext *context,
                                         csMetaBlock &block)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(nullptr != _su && _su->isOpen(), "can not be invalid");

      constexpr PAGE_ID lpid = CS_META_BLOCK_PAGE_LPID;
      logicalPageBuffer lpb;
      csMetaBlockPageAccessor accessor;
      mainDataSpace &mds = _su->getMainDataSpace();
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      rc = mds.getLogicalPageBuffer(context, lpid, mode, lpb);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get buffer of lpid[%d], rc:%d",
                lpid, rc);
         goto error;
      }

      rc = accessor.read(context, &lpb, block);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read meta block:%d", rc);
         goto error;
      }
   done:
      lpb.fini();
      return rc;
   error:
      goto done;
   }

   void collectionSpace::_initProperties(const csMetaBlock &block)
   {
      SDB_ASSERT(block.isValid(), "can not be invalid");
      SDB_ASSERT(nullptr != _su && _su->isOpen(), "can not be invalid");
      _properties.csid = _su->getIdentifier();
      _properties.status = (CS_STATUS)block.status;
      _properties.type = (CS_TYPE)block.type;
      _properties.flags = block.flags;
      _properties.name.assign(block.name);
      _properties.maxCLLogicalID = block.maxCLLogicalID;
      return;
   }

   void collectionSpace::_exportMetaBlock(csMetaBlock &block)
   {
      block.version = CS_META_BLOCK_VERSION_1;
      block.status = _properties.status;
      block.type = _properties.type;
      block.flags = _properties.flags;
      ossMemset(block.name, 0, sizeof(block.name));
      ossMemcpy(block.name, _properties.name.c_str(), _properties.name.size());
      block.maxCLLogicalID = _properties.maxCLLogicalID;
      return;  
   }

   INT32 collectionSpace::allocateNextIndexLid(UINT32 &indexLid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "can not be invalid");
      bson::BSONObj manifest;
      csIndexMetaStorage ms(getLogicalID());
      SDB_ASSERT(ms.isValid(), "can not be invalid");
      std::unique_lock<std::mutex> lck(_lidMutex);
      indexLid = INVALID_LOGICAL_INDEX_ID;
      UINT32 nextIndexId = _maxIndexLid + 1;

      if (INVALID_LOGICAL_INDEX_ID == (nextIndexId))
      {
         rc = SDB_DMS_MAX_INDEX;
         PD_LOG(PDERROR, "hit max number of index");
         goto error;
      }

      manifest = BSON(IXM_MAX_LOGICAL_ID << (nextIndexId));
      rc = ms.upsert(manifest);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "upsert index manifest failed, rc:%d", rc);
         goto error;
      }

      indexLid = nextIndexId;
      _maxIndexLid = nextIndexId;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::_loadMaxIndexLid()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(DMS_INVALID_LOGICCSID != getLogicalID(), "can not be invalid");
      csIndexMetaStorage ms(getLogicalID());
      SDB_ASSERT(ms.isValid(), "can not be invalid");
      UINT32 maxLid = INVALID_LOGICAL_INDEX_ID;

      rc = ms.getMaxIndexLid(maxLid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get max index logical id failed, rc:%d", rc);
         goto error;
      }
      _maxIndexLid = maxLid;

   done:
      return rc;
   error:
      goto done;
   }

   CL_MB_ID collectionSpace::_upperBoundCL(UINT32 logicalId)const
   {
      CL_MB_ID mbid = INVALID_CL_MB_ID;
      if (DMS_INVALID_LOGICCLID == logicalId)
      {
         if (!_lidIndex.empty())
         {
            mbid = _lidIndex.cbegin()->second;
         }
      }
      else
      {
         auto itr = _lidIndex.upper_bound(logicalId);
         if (_lidIndex.end() != itr)
         {
            mbid = itr->second;
         }
      }

      return mbid;
   }

   CL_MB_ID collectionSpace::_findCLByName(const strSlice &clName) const
   {
      SDB_ASSERT(!clName.empty(), "can not be invalid");
      CL_MB_ID mbid = INVALID_CL_MB_ID;
      auto itr = _clNameIndex.find(clName.str());
      if (_clNameIndex.end() != itr)
      {
         mbid = itr->second;
      }
      return mbid;
   }

   CL_MB_ID collectionSpace::_findCLByInnerId(utilCLInnerID innerId) const
   {
      SDB_ASSERT(UTIL_IS_VALID_CL_INNERID(innerId), "can not be invalid");
      CL_MB_ID mbid = INVALID_CL_MB_ID;
      auto itr = _innerIdIndex.find(innerId);
      if (_innerIdIndex.end() != itr)
      {
         mbid = itr->second;
      }
      return mbid;
   }

   CL_MB_ID collectionSpace::_findCLByLogicalId(UINT32 logicalId) const
   {
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalId, "can not be invalid");
      CL_MB_ID mbid = INVALID_CL_MB_ID;
      auto itr = _lidIndex.find(logicalId);
      if (_lidIndex.end() != itr)
      {
         mbid = itr->second;
      }
      return mbid;
   }
}//namespace vessel
}//namespace engine