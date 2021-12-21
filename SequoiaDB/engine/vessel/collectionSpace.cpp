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

   Source File Name = collectionSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
#include "vessel/IRedoLogger.h"
#include "vessel/crpIniter.h"
#include "vessel/collectionRecordPage.h"

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

   collectionSpaceId collectionSpace::getIdentifier()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      return collectionSpaceId(_recordInMem.logicalID,
                               _recordInMem.uniqueID,
                               getSpaceId());
   }

   INT32 collectionSpace::create(requestContext *context,
                                 const strSlice &name,
                                 utilCSUniqueID uniqueId,
                                 UINT32 logicalID,
                                 storageUnit *su,
                                 const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj optionsObj;
      slice optionsSlice;
      OSS_LATCH_MODE mode = SHARED;
      SDB_ASSERT(!isOpen(), "do not recreate");
      
      if (OSS_UNLIKELY(NULL == context ||
                       !context->isSpaceIdLocked(&mode) ||
                       EXCLUSIVE != mode ||
                       DMS_COLLECTION_SPACE_NAME_SZ < name.strLen() ||
                       name.empty() ||
                       DMS_INVALID_LOGICCSID == logicalID ||
                       NULL == su ||
                       !su->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _isOpen = TRUE;
      _su = su;
      _nextCLLogicalId = 0;

      rc = initInMemStructures();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem structures:%d", rc);
         goto error;
      }

      _recordInMem.version = CMR_VERSION_1;
      _recordInMem.status = CMR_STATUS_ONLINE;
      _recordInMem.type = CMR_TYPE_NORMAL;
      _recordInMem.flags = 0;
      _recordInMem.logicalID = logicalID;
      _recordInMem.uniqueID = uniqueId;
      ossMemcpy(_recordInMem.name, name.str(), name.strLen() + 1);

      optionsObj = options.toBson();
      optionsSlice.reset(optionsObj.objsize(), optionsObj.objdata());
      rc = su->getMainDataSpace().initMetaPageWhenCreateCS(context,
                                                           _recordInMem,
                                                           optionsSlice);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init meta page when creating:%d", rc);
         goto error;
      }

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
      
      SDB_ASSERT(!isOpen(), "do not reopen");
      if (OSS_UNLIKELY(NULL == context ||
                       NULL == su ||
                       !su->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(context->getSpaceID() == su->getSpaceID(), "must be same");

      _isOpen = TRUE;
      _su = su;

      rc = su->getMainDataSpace().readMetaRecordWhenOpen(context, _recordInMem);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read meta data when open:%d", rc);
         goto error;
      }

      rc = initInMemStructures();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init in-mem structures:%d", rc);
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
      _su = NULL;
      _recordInMem.reset();
      _allocator.fini();
      _collections.fini();
      _nextCLLogicalId = 0;
      _clNameIndex.clear();
      _innerIdIndex.clear();
      _unformalNameIndex.clear();
      _unformalInnerIdIndex.clear();
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
      collectionObjHolder *holder = NULL;
      collection *obj = NULL;
      BOOLEAN locked = FALSE;
      
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            clName.empty() ||
                            DMS_COLLECTION_NAME_SZ < clName.strLen() ||
                            !options.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = precreateCL(clName, clInnerId, mbID, logicalID);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = ensureCollectionHolder(mbID, &holder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure cl obj[%d] holder:%d", mbID, rc);
         goto error;
      }

      context->lockMB(mbID, &(holder->getLatch()), EXCLUSIVE);
      locked = TRUE;

      SDB_ASSERT(holder->isFree(), "must be free");
      obj = holder->ensureObj();
      if (NULL == obj)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = ensureCollectionRecordPage(context, mbID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure cl record page:%d", rc);
         goto error;
      }

      rc = obj->create(context, clName, clInnerId,
                       logicalID, this, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cl[%s] on disk:%d", clName.str(), rc);
         holder->releaseObj();
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
         rollbackPrecreating(clName, clInnerId, mbID, logicalID);
      }
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

      bson::BSONObj record;
      ossSLatchGuard guard(&_latch, SHARED, FALSE);
      
      if (OSS_UNLIKELY(NULL == context && NULL == cursor))
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
         BOOLEAN mbLocked = FALSE;
         if (!guard.isLocked())
         {
            guard.lock();
         }

         collectionObjHolder *holder = NULL;
         CL_MB_ID mbID = INVALID_CL_MB_ID;
         strSlice nameSlice(cursor->getCLName());

         if (!upperBoundCLName(nameSlice, mbID))
         {
            cursor->setEOC();
            goto done;
         }

         rc = getCollectionHolder(mbID, &holder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get cl holder[%d], rc:%d", mbID, rc);
            goto error;
         }

         mbLocked = context->tryLockMB(mbID, &(holder->getLatch()), SHARED);

         if (mbLocked)
         {
            if (holder->isFree())
            {
               PD_LOG(PDERROR, "unexpected free holder[%d]", mbID);
               rc = SDB_VESSEL_INTERNAL_ERR;
               goto error;
            }

            if (cursor->isPushed(holder->getObj()->getLogicalID()))
            {
               cursor->setCLName(holder->getObj()->getName());
               context->unlockMB();
               continue;
            }

            rc = holder->getObj()->dump(context, record);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to dump collection:%d", rc);
               goto error;
            }

            rc = cursor->pushData(record.objsize(), record.objdata());
            if (SDB_VESSEL_CURSOR_NO_SPACE == rc)
            {
               context->unlockMB();
               rc = SDB_OK;
               break;
            }
            else if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to push record to cursor:%d", rc);
               goto error;
            }

            cursor->markLIdPushed(holder->getObj()->getLogicalID());
            cursor->setCLName(holder->getObj()->getName());
            context->unlockMB();
            if (cursor->noMorePushThisLoop())
            {
               break;
            }
            else
            {
               continue;
            }
         }
         else /// failed to lock mb
         {
            guard.unlock();
            holder->getLatch().lock_r();
            holder->getLatch().release_r();
            continue;
         }
         
      } while (TRUE);
      
   done:
      if (context->isMbLocked())
      {
         context->unlockMB();
      }
      guard.unlock();
      return rc;
   error:
      
      goto done;
   }

   INT32 collectionSpace::dump(requestContext *context,
                               bson::BSONObj &record)
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
                                   _su->getMainDataSpace().getStorageCoreArgs().pageSize,
                                   _su->getMainDataSpace().getStorageCoreArgs().getSegmentSize(),
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
      BOOLEAN mbLocked = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            clName.empty() ||
                            NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      do
      {
         collectionObjHolder *holder = NULL;
         CL_MB_ID mbID = INVALID_CL_MB_ID;
         ossSLatchGuard guard(&_latch, SHARED);
         if (!find(clName, mbID))
         {
            rc = SDB_DMS_NOTEXIST;
            goto error;
         }

         rc = getCollectionHolder(mbID, &holder);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get collection holder:%d", rc);
            goto error;
         }

         mbLocked = context->tryLockMB(mbID, &(holder->getLatch()), mode);

         if (!mbLocked)
         {
            guard.unlock();
            if (SHARED == mode)
            {
               holder->getLatch().lock_r();
               holder->getLatch().release_r();
            }
            else
            {
               holder->getLatch().lock_w();
               holder->getLatch().release_w();
            }
            continue;
         }

         if (holder->isFree())
         {
            PD_LOG(PDERROR, "unexpected free holder[%d]", mbID);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         SDB_ASSERT(holder->getObj()->isOpen(), "impossible");
         *obj = holder->getObj();
         break;
      } while (TRUE);
   done:
      return rc;
   error:
      if (mbLocked)
      {
         context->unlockMB();
      }
      goto done;
   }

   INT32 collectionSpace::getCollectionByMBID(requestContext *context,
                                              CL_MB_ID mbID,
                                              UINT32 logicalID,
                                              OSS_LATCH_MODE mode,
                                              collection **obj)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      collectionObjHolder *holder = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            INVALID_CL_MB_ID == mbID ||
                            NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionHolder(mbID, &holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      context->lockMB(mbID, &(holder->getLatch()), mode);
      locked = TRUE;

      if (holder->isFree())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      if (DMS_INVALID_LOGICCLID != logicalID &&
          holder->getObj()->getLogicalID() != logicalID)
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      SDB_ASSERT(holder->getObj()->isOpen(), "impossible");

      *obj = holder->getObj();
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockMB();
      }
      goto done;
   }

   INT32 collectionSpace::getCollectionById(requestContext *context,
                                             const collectionId &id,
                                             OSS_LATCH_MODE mode,
                                             collection **obj)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      collectionObjHolder *holder = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == context ||
                            !id.isValid() ||
                            context->isMbLocked() ||
                            NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = getCollectionHolder(id.getMbId(), &holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      context->lockMB(id.getMbId(), &(holder->getLatch()), mode);
      locked = TRUE;

      if (holder->isFree())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      if (id.getLid() != holder->getObj()->getLogicalID() ||
          id.getInnerId() != holder->getObj()->getInnerID())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      SDB_ASSERT(holder->getObj()->isOpen(), "impossible");

      *obj = holder->getObj();
   done:
      return rc;
   error:
      if (locked)
      {
         context->unlockMB();
      }
      goto done;
   }

   INT32 collectionSpace::createCheckpoint(requestContext *context,
                                           BOOLEAN forceFullCheckpoint)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _su->getMainDataSpace().createCheckpoint(context, forceFullCheckpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checkpoint on mds:%d", rc);
         goto error;
      }

      rc = _su->getIndexSpace().createCheckpoint(context, forceFullCheckpoint);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create checkpoint on is:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::initCollectionsFromDisk(requestContext *context)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su, "can not be null");
      UINT32 crpCapacity = 0;
      const storageCoreArgs &args = _su->getMainDataSpace().getStorageCoreArgs();
      mainDataSpace *mds = &(_su->getMainDataSpace());
      collectionRecord record;
      UINT32 totalCrpCount = 0;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_SHARED);

      rc = getCapacityOfCLRecordPage(args.pageSize, crpCapacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      totalCrpCount = MAX_CL_MB_COUNT / crpCapacity;
      if (0 != MAX_CL_MB_COUNT % crpCapacity)
      {
         ++totalCrpCount;
      }

      for (UINT32 i = 0; i < totalCrpCount; ++i)
      {
         PAGE_ID lpid = COLLECTION_RECORD_PAGE_MIN_LPID + i;
         logicalPageBuffer lpb;
         BOOLEAN mapped = FALSE;

         rc = mds->isLogicalPageMapped(context, lpid, mapped);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to test if lpid[%d] mapped:%d", lpid, rc);
            goto error;
         }
         else if (!mapped)
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
            PD_LOG(PDERROR, "collection record page[%d] may be broken:%d", lpid, rc);
            goto error;
         }

         for (UINT32 j = 0; j < crpCapacity; ++j)
         {
            UINT32 tmp = i * crpCapacity + j;
            if (MAX_CL_MB_COUNT <= tmp)
            {
               break;
            }

            if (!getCollectionRecordIfValid(lpb.getRuntimeBuffer().getPageHead(),
                                            j, record))
            {
               continue;
            }

            rc = _allocator.ensureBitmapPageCount(tmp / ALLOCATOR_PAGE_CAPAITY + 1);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure bitmap page count:%d", rc);
               goto error;
            }

            rc = initCollection(context, &record);
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
      _nextCLLogicalId = 0;
      goto done;
   }

   INT32 collectionSpace::initCollection(requestContext *context,
                                         const collectionRecord *record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != record, "can not be null");
      SDB_ASSERT(record->isValid(), "can not be invalid");
      SDB_ASSERT(!context->isMbLocked(), "can not be locked");

      collectionObjHolder *holder = NULL;
      collection *cl = NULL;

      rc = _allocator.occupy(record->mbID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to occupy mbid[%d], rc:%d", record->mbID, rc);
         goto error;
      }

      rc = ensureCollectionHolder(record->mbID, &holder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure collection holder:%d", rc);
         goto error;
      }

      SDB_ASSERT(holder->isFree(), "impossible be free");
      cl = holder->ensureObj();
      if (NULL == cl)
      {
         PD_LOG(PDERROR, "failed to ensure cl obj[%d]", record->mbID);
         rc = SDB_OOM;
         goto error;
      }

      context->lockMB(record->mbID, &holder->getLatch(), EXCLUSIVE);
      rc = cl->initWhenOpen(context, *record, this);
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

      if (_nextCLLogicalId <= record->logicalCLID)
      {
         _nextCLLogicalId = record->logicalCLID + 1;
      }
      
   done:
      context->unlockMB();
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::ensureCollectionHolder(CL_MB_ID mbID,
                                                 collectionObjHolder **holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(NULL != holder, "can not be null");
      SDB_ASSERT(_collections.isInitialized(), "must be inited");

      collectionObjHolderGroup *group = NULL;
      UINT32 i = mbID / collectionObjHolderGroup::CAPACITY;

      rc = _collections.ensure(i, &group);
      if (SDB_OK != rc)
      {
         goto error;
      }

      *holder = &(group->holders[mbID & (collectionObjHolderGroup::CAPACITY - 1)]);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::getCollectionHolder(CL_MB_ID mbID,
                                              collectionObjHolder **holder)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(NULL != holder, "can not be null");
      SDB_ASSERT(_collections.isInitialized(), "must be inited");

      collectionObjHolderGroup *group = NULL;
      UINT32 i = mbID / collectionObjHolderGroup::CAPACITY;

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

      *holder = &(group->holders[mbID & (collectionObjHolderGroup::CAPACITY - 1)]);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::ensureCollectionRecordPage(requestContext *context,
                                                     CL_MB_ID mbID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");

      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
      atomicOperationList *oplist = NULL;
      crpIniter initer;
      BOOLEAN locked = FALSE;
      mainDataSpace &mds = _su->getMainDataSpace();
      UINT32 pageSize = mds.getStorageCoreArgs().pageSize;
      PAGE_ID lpid = getCrpLpidOfCollection(pageSize, mbID);
      if (INVALID_PAGE_ID == lpid)
      {
         PD_LOG(PDERROR, "failed to crp lpid of mb[%d]", mbID);
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
      if (NULL != oplist)
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

   INT32 collectionSpace::precreateCL(const strSlice &clName,
                                      utilCLInnerID innerID,
                                      CL_MB_ID &mbID,
                                      UINT32 &logicalID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(!clName.empty(), "can not be empty");
      UINT32 m = INVALID_CL_MB_ID;
      
      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      if (_nextCLLogicalId == DMS_INVALID_LOGICCLID)
      {
         PD_LOG(PDERROR, "logical id hits the max value");
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
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

      do
      {
         rc = _allocator.allocateBits(1, &m, 1);
         if (SDB_VESSEL_OUT_OF_RESOURCE == rc)
         {
            PD_LOG(PDERROR, "no free mb id any more");
            rc = SDB_VESSEL_OUT_OF_MBID_RESOURCE;
            goto error;
         }
         else if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to find free mb id in allocator:%d", rc);
            goto error;
         }
         else if (MAX_CL_MB_COUNT <= m)
         {
            m = INVALID_CL_MB_ID;
            continue;
         }

         break;
      } while(TRUE);

      _unformalNameIndex.insert(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _unformalInnerIdIndex.insert(innerID);
      }

      SDB_ASSERT(m < MAX_CL_MB_COUNT, "impossible");
      mbID = m;
      logicalID = _nextCLLogicalId++;

   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpace::rollbackPrecreating(const strSlice &clName,
                                             utilCLInnerID innerID,
                                             CL_MB_ID mbID,
                                             UINT32 logicalID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");

      ossSLatchGuard guard(&_latch, EXCLUSIVE);
      _unformalNameIndex.erase(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _unformalInnerIdIndex.erase(innerID);
      }
      _allocator.release(mbID);
      if (logicalID + 1 == _nextCLLogicalId)
      {
         --_nextCLLogicalId;
      }
      return;
   }

   void collectionSpace::endCreatingCL(collection *obj)
   {
      SDB_ASSERT(NULL != obj, "can not be null");
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
      SDB_ASSERT(65535 == MAX_CL_MB_COUNT, "must be 65535");
      inMemBitmap::options o;

      static constexpr UINT32 CHUNK_SIZE = 16;
      static constexpr UINT32 CAPACITY = (MAX_CL_MB_COUNT +1) / collectionObjHolderGroup::CAPACITY;

      o.bitmapBeginPage = 0;
      o.maxBitmapPageCount = (MAX_CL_MB_COUNT + 1) / ALLOCATOR_PAGE_CAPAITY;
      rc = _allocator.initWithNoLatch(ALLOCATOR_PAGE_CAPAITY, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init allocator:%d", rc);
         goto error;
      }

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
      SDB_ASSERT(NULL != obj, "can not be null");
      const CHAR *name = obj->getName();
      utilCLInnerID innerId = obj->getInnerID();
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

   BOOLEAN collectionSpace::upperBoundCLName(const strSlice &clName,
                                             CL_MB_ID &mbID)const
   {
      BOOLEAN r = FALSE;
      NAME_INDEX::const_iterator itr = _clNameIndex.upper_bound(clName.str());
      if (itr != _clNameIndex.end())
      {
         mbID = itr->second;
         r = TRUE;
      }

   done:
      return r;
   }

   BOOLEAN collectionSpace::find(const strSlice &clName,
                                 CL_MB_ID &mbID)const
   {
      BOOLEAN r = FALSE;
      NAME_INDEX::const_iterator itr = _clNameIndex.find(clName.str());
      if (itr != _clNameIndex.end())
      {
         mbID = itr->second;
         r = TRUE;
      }

   done:
      return r;
   }

   BOOLEAN collectionSpace::find(utilCLInnerID innerID,
                                 CL_MB_ID &mbID)const
   {
      SDB_ASSERT(UTIL_IS_VALID_CL_INNERID(innerID), "can not be invalid");
      BOOLEAN r = FALSE;
      ID_INDEX::const_iterator itr = _innerIdIndex.find(innerID);
      if (_innerIdIndex.end() != itr)
      {
         mbID = itr->second;
         r = TRUE;
      }
      return r;
   }
   
   INT32 collectionSpace::createCSNameFile()const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != _su && _su->isOpen(), "can not be invalid");
   
      ossPoolString fullPath;
      OSSFILE file;
      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {};
      CHAR nameBuffer[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {};
      strSlice nameSlice(_recordInMem.name);
      strSlice suffix(SIMPLE_FILE_SUFFIX_CSNAME);
      UINT32 flags = OSS_READWRITE|OSS_EXCLUSIVE|OSS_REPLACE;


      rc = _su->getDirPathOfType(SPACE_TYPE_MAIN_DATA, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get main data space dir:%d", rc);
         goto error;
      }

      if (nameSlice.empty() ||
          DMS_COLLECTION_SPACE_NAME_SZ < nameSlice.strLen())
      {
         PD_LOG(PDERROR, "invalid collection space name");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      if (!vesselFileName::buildSimpleName(_su->getSpaceID(),
                                           suffix, MAX_FILE_NAME_LEN + 1,
                                           fileName))
      {
         PD_LOG(PDERROR, "failed to build csname file");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      ossMemcpy(nameBuffer, nameSlice.str(), nameSlice.strLen());
      nameBuffer[nameSlice.strLen()] = '\n';

      fullPath.append(OSS_FILE_SEP);
      fullPath.append(fileName);

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
      SDB_ASSERT(NULL != _su && _su->isOpen(), "can not be invalid");

      CHAR fileName[MAX_FILE_NAME_LEN + 1] = {};
      ossPoolString fullPath;
      strSlice suffix(SIMPLE_FILE_SUFFIX_CSNAME);

      if (!vesselFileName::buildSimpleName(_su->getSpaceID(),
                                           suffix, MAX_FILE_NAME_LEN + 1, fileName))
      {
         PD_LOG(PDERROR, "failed to build csname file");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = _su->getDirPathOfType(SPACE_TYPE_MAIN_DATA, fullPath);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get main data space dir:%d", rc);
         goto error;
      }

      fullPath.append(OSS_FILE_SEP);
      fullPath.append(fileName);

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
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine