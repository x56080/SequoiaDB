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
#include "vessel/smpAccessor.h"
#include "vessel/spaceManagementPage.h"
#include "vessel/csgpAccessor.h"
#include "vessel/storageUnit.h"
#include "dpsOp2Record.hpp"
#include "vessel/redoLogUtil.h"
#include "vessel/outerResource.h"
#include "dpsLogRecord.hpp"
#include "vessel/idMapPage.h"
#include "vessel/impAccessor.h"
#include "vessel/collection.h"
#include "vessel/crpAccessor.h"
#include "vessel/listCLCursor.h"
#include "vessel/IRedoLogger.h"
#include "vessel/fsmFile.h"
#include "vessel/bitMapUtils.h"
#include "vessel/fsmFile.h"

namespace engine
{
namespace vessel
{
   collectionSpace::collectionSpace()
   {
      
   }

   collectionSpace::~collectionSpace()
   {
      fini();
   }

   INT32 collectionSpace::create(requestContext *context,
                                 const strSlice &name,
                                 UINT32 logicalID,
                                 storageUnit *su,
                                 const createCSOptions &options)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isClosed(), "do not recreate");

      if (OSS_UNLIKELY(NULL == context ||
                       EXCLUSIVE != context->getSpaceIDLockedMode() ||
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
      else if (OSS_UNLIKELY(!isClosed()))
      {
         fini();
      }

      rc = _ms.create(context, su, name, logicalID, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data space:%d", rc);
         goto error;
      }

      ///TODO, rollback when failed.
      rc = _cowEnv.open(context, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cow env:%d", rc);
         goto error;
      }

      _recordInMem.version = CMR_VERSION_1;
      _recordInMem.status = CMR_STATUS_ONLINE;
      _recordInMem.flags = 0;
      _recordInMem.logicalID = logicalID;
      _recordInMem.uniqueID = options.uniqueID;
      ossMemcpy(_recordInMem.name, name.str(), name.strLen());

      /// The name file is only for easier viewing. Even if failed
      /// to write file, it does not affect the result.
      su->ensureCSNameFile(context, strSlice(_recordInMem.name));
      _status = OPEN;
   done:
      return rc;
   error:
      fini();
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
      SDB_ASSERT(isClosed(), "do not recreate");
      if (OSS_UNLIKELY(NULL == context ||
                       NULL == su ||
                       !su->isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isClosed()))
      {
         fini();
      }

      rc = _ms.open(context, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open main data space:%d", rc);
         goto error;
      }

      rc = _cowEnv.open(context, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init cow env:%d", rc);
         goto error;
      }

      //rc = _is.open(context, su);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open index space:%d", rc);
         goto error;
      }

      rc = _ms.readGlobalMetaData(context, FALSE, _recordInMem);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to read global meta record:%d", rc);
         goto error;
      }

      rc = initCollectionsFromDisk(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init collections from disk:%d", rc);
         goto error;
      }

      _status = OPEN;
   done:
      return rc;
   error:
      close();
      goto done;
   }

   void collectionSpace::fini()
   {
      _status = CLOSED;
      _recordInMem.reset();
      _maxCLLogicalID = DMS_INVALID_LOGICCLID;
      _clNameIndex.clear();
      _clIdIndex.clear();
      _creatingNameIndex.clear();
      _creatingIdIndex.clear();
      _collectionAllocator.fini();
      _ms.close();
      //_is.close();
      _cowEnv.close();
      return;
   }

   SPACE_ID collectionSpace::getSpaceID()const
   {
      if (OSS_LIKELY(isOpen()))
      {
         return _ms.getSU()->getSpaceID();
      }
      return INVALID_SPACE_ID;
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
      collectionAllocator::collectionHolder *holder = NULL;

      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(context->getSpaceIDLocked(), "must be locked");
      SDB_ASSERT(getSpaceID() == context->getSpaceID(), "must be same");
      SDB_ASSERT(!context->mbLocked(), "can not be locked");

      if (OSS_UNLIKELY(NULL == context ||
                       clName.empty()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!options.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = precreateCL(clName, clInnerId, logicalID);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = _collectionAllocator.allocateNewMB(mbID, &holder);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to allocate new mb:%d", rc);
         goto error;
      }

      rc = context->lockMB(mbID, holder->getMutex(), EXCLUSIVE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      SDB_ASSERT(!holder->isFree(), "can not be free");

      rc = holder->getCollection()->create(context, clName, clInnerId,
                                           logicalID, this, options);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create cl[%s] on disk:%d", clName.str(), rc);
         goto error;
      }

      moveToFormalIndex(holder);
      context->unlockMB();
   done:
      return rc;
   error:
      context->unlockMB();
      if (INVALID_CL_MB_ID != mbID)
      {
         _collectionAllocator.releaseMB(mbID);
      }
      if (DMS_INVALID_LOGICCLID != logicalID)
      {
         rollbackPrecreating(clName, clInnerId, logicalID);
      }
      goto done;
   }

   UINT32 collectionSpace::getCollectionCount()
   {
      ossScopedLock lock(&_latch, SHARED);
      return _clNameIndex.size();
   }

   INT32 collectionSpace::listCollections(requestContext *context,
                                          listCLCursor *cursor)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      static const UINT32 NAME_BUFF_SIZE = DMS_COLLECTION_NAME_SZ + 1;
      CHAR clName[NAME_BUFF_SIZE] = {0};
      collectionAllocator::collectionHolder *holder = NULL;
      listCollectionsRecord record;
      BOOLEAN locked = FALSE;
      ISession *session = NULL;
      UINT32 loop = 0;
      static const UINT32 quitCheck = 16;
      
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

      session = context->getSession();

      do
      {
         UINT32 lid = DMS_INVALID_LOGICCLID;
         strSlice nameSlice(cursor->getCLName());
         if (loop++ == quitCheck)
         {
            if (session->quit())
            {
               rc = SDB_APP_INTERRUPT;
               goto error;
            }
            loop = 0;
         }

         if (!upperBoundCLName(nameSlice, NAME_BUFF_SIZE, clName, lid, &holder))
         {
            cursor->pushEnd();
            goto done;
         }

         rc = context->lockMB(holder->getMBId(), holder->getMutex(), SHARED);
         if (OSS_UNLIKELY(SDB_OK != rc))
         {
            rc = SDB_VESSEL_INTERNAL_ERR;
            PD_LOG(PDERROR, "failed to get mb lock:%d", rc);
            goto error;
         }
         locked = TRUE;

         /// some one dropped cl before we locked.
         /// just continue.
         if (holder->isFree())
         {
            context->unlockMB();
            locked = FALSE;
            cursor->setCLName(clName);
            continue;
         }

         /// some one dropped cl and mb was reallocated
         /// just continue;
         if (lid != holder->getCollection()->getLogicalID())
         {
            context->unlockMB();
            locked = FALSE;
            cursor->setCLName(clName);
            continue;
         }

         /// some one renamed cl.
         /// just continue;
         if (cursor->isPushed(holder->getCollection()->getLogicalID()))
         {
            context->unlockMB();
            locked = FALSE;
            cursor->setCLName(clName);
            continue;
         }

         holder->getCollection()->dump(context, record);
         context->unlockMB();
         locked = FALSE;
         holder = NULL;
         SDB_ASSERT(INVALID_CL_MB_ID != record.mbID, "impossible");

         rc = cursor->push(sizeof(listCollectionsRecord), (const CHAR *)(&record));
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
            cursor->setCLName(clName);
            cursor->markLIdPushed(lid);
            if (cursor->hasNoSpaceToPush(sizeof(listCollectionsRecord)))
            {
               break;
            }
            continue;
         }
         
      } while (TRUE);
      
   done:
      if (locked)
      {
         context->unlockMB();
      }
      return rc;
   error:
      
      goto done;
   }

   INT32 collectionSpace::ensureFsmFile(requestContext *context,
                                        fsmFile **file)
   {
      INT32 rc = SDB_OK;
      rc = _ms.getSU()->ensureFsmFile(context, file);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpace::dump(requestContext *context,
                               listCollectionSpaceRecord &record)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      UINT32 pageSize = 0;
      UINT32 pageCountPerSeg = 0;
      if (OSS_UNLIKELY(NULL == context ||
                       !context->getSpaceIDLocked() ||
                       context->getSpaceID() != getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _ms.getSU()->getCoreArgs(FILE_TYPE_DD, &pageSize, &pageCountPerSeg, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }

      record.version = getVersion();
      ossStrcpy(record.name, getCSName());
      record.uniqueID = getUniqueID();
      record.spaceID = getSpaceID();
      record.status = getStatus();
      record.flags = getFlags();
      record.dataPageSize = pageSize;
      record.dataPageCountPerSeg = pageCountPerSeg;
      rc = _ms.getSU()->getCoreArgs(FILE_TYPE_IDX_D, &pageSize, &pageCountPerSeg, NULL);
      if (SDB_OK != rc)
      {
         goto error;
      }
      record.idxPageSize = pageSize;
      record.idxPageCountPerSeg = pageCountPerSeg;
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
      collectionAllocator::collectionHolder *holder = NULL;
      UINT32 logicalID = DMS_INVALID_LOGICCLID;
      BOOLEAN locked = FALSE;

      if (OSS_UNLIKELY(NULL == context ||
                       !context->getSpaceIDLocked() ||
                       getSpaceID() != context->getSpaceID() ||
                       clName.empty() ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!findCollection(clName, logicalID, &holder))
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");

      rc = context->lockMB(holder->getMBId(), holder->getMutex(), mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      /// some one dropped cl
      if (holder->isFree())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      /// some one dropped cl and mb was reallocated.
      if (logicalID != holder->getCollection()->getLogicalID())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      *obj = holder->getCollection();
   done:
      return rc;
   error:
      if (locked)
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
      collectionAllocator::collectionHolder *holder = NULL;
      BOOLEAN locked = FALSE;
      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_CL_MB_ID == mbID ||
                       !context->getSpaceIDLocked() ||
                       getSpaceID() != context->getSpaceID() ||
                       NULL == obj))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _collectionAllocator.getHolder(mbID, holder);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = context->lockMB(mbID, holder->getMutex(), mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
      locked = TRUE;

      if (holder->isFree())
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      if (DMS_INVALID_LOGICCLID != logicalID &&
          holder->getCollection()->getLogicalID() != logicalID)
      {
         rc = SDB_DMS_NOTEXIST;
         goto error;
      }

      *obj = holder->getCollection();
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
      
      impAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = FALSE;
      PAGE_ID pid = INVALID_PAGE_ID;
      PAGE_ID systemImp = _ms.getSystemImpPid();
      UINT32 pageSize = 0;
      UINT32 capacity = 0;
      UINT32 freeCount = 0;
      UINT32 crpScanned = 0;
      UINT32 maxCrpCount = 0;
      UINT32 impCapacity = 0;

      rc = _ms.getDataPageSize(pageSize);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (!get64AlignedIMPCapacity(pageSize, impCapacity))
      {
         PD_LOG(PDERROR, "failed to get imp capacity");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = getCapacityOfCLRecordPage(pageSize, capacity);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get capacity of cl record page:%d", rc);
         goto error;
      }

      maxCrpCount = MAX_CL_MB_COUNT / capacity;
      if (0 != (MAX_CL_MB_COUNT % capacity))
      {
         ++maxCrpCount;
      }

      rc = accessor.initWithOptions(context, FILE_TYPE_DM,
                                    systemImp, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }

      rc = accessor.getFreeCount(context, freeCount);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get free count of imp:%d", rc);
         goto error;
      }

      if (impCapacity < freeCount)
      {
         PD_LOG(PDERROR, "invalid free count:%d", freeCount);
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      for (UINT32 i = 0; i < maxCrpCount && crpScanned < (impCapacity - freeCount); ++i)
      {
         rc = accessor.getPidByOffset(i, &pid, NULL);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get next pid from imp:%d", rc);
            goto error;
         }

         /// we cannot guarantee that no holes in page.
         if (INVALID_PAGE_ID == pid)
         {
            continue;
         }

         rc = initCollectionsFromOneDiskPage(context, capacity, pid);
         if (SDB_OK != rc)
         {
            goto error;
         }
         
         ++crpScanned;
      }
   
   done:
      accessor.fini(context);
      return rc;
   error:
      _clNameIndex.clear();
      _clIdIndex.clear();
      _collectionAllocator.fini();
      _maxCLLogicalID = DMS_INVALID_LOGICCLID;
      goto done;
   }

   INT32 collectionSpace::initCollectionsFromOneDiskPage(requestContext *context,
                                                         UINT32 capacity,
                                                         PAGE_ID pid)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(INVALID_PAGE_ID != pid, "can not be invalid");
      crpAccessor accessor;
      pageAccessor::options o;
      o.cacheMode = FALSE;
      collectionRecord record;

      rc = accessor.initWithOptions(context, FILE_TYPE_DD,
                                    pid, o, _ms.getSU());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accesor of pid[%d], rc:%d", pid, rc);
         goto error;
      }

      for (UINT32 i = 0; i < capacity; ++i)
      {
         collectionAllocator::collectionHolder *holder = NULL;
         strSlice nameSlice;

         rc = accessor.getClRecordBySlot(i, record);
         if (SDB_DMS_NOTEXIST == rc)
         {
            rc = SDB_OK;
            continue;
         }
         else if (SDB_OK != rc)
         {
            goto error;
         }

         if (!record.isValid())
         {
            PD_LOG(PDERROR, "bitmap may crashed on cl record page[%d,%d]", pid, i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         nameSlice.reset(record.name);
         if (existsInFormalIndex(nameSlice, record.innerID))
         {
            PD_LOG(PDERROR, "duplicate cl name[%s] or inner id[%d] when loading",
                   record.name, record.innerID);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = _collectionAllocator.occupyMB(record.mbID, &holder);
         if (SDB_OK != rc)
         {
            goto error;
         }

         rc = holder->getCollection()->initWhenOpen(context, record, this);
         if (SDB_OK != rc)
         {
            goto error;
         }

         insertIntoFormalIndex(holder);
         if (DMS_INVALID_LOGICCLID == _maxCLLogicalID)
         {
            _maxCLLogicalID = record.logicalCLID;
         }
         else if (_maxCLLogicalID < record.logicalCLID)
         {
            _maxCLLogicalID =  record.logicalCLID;
         }
         else if (_maxCLLogicalID == record.logicalCLID)
         {
            PD_LOG(PDERROR, "duplicated cl logical id[%d] found", _maxCLLogicalID);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }
      }
   done:
      accessor.fini(context);
      return rc;
   error:
      goto done;
   }

   void collectionSpace::moveToFormalIndex(collectionAllocator::collectionHolder *holder)
   {
      SDB_ASSERT(NULL != holder && !holder->isFree(), "can not be invalid");
      const CHAR *clName = holder->getCollection()->getName();
      utilCLInnerID innerID = holder->getCollection()->getInnerID();
      UINT32 logicalID = holder->getCollection()->getLogicalID();
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      _UID_HOLDER_PAIR p(holder, logicalID);
      ossScopedLock lock(&_latch, EXCLUSIVE);

      _creatingNameIndex.erase(clName);
      _clNameIndex[clName] = p;
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _creatingIdIndex.erase(innerID);
         _clIdIndex[innerID] = p;
      }
      return;
   }

   BOOLEAN collectionSpace::insertIntoFormalIndex(collectionAllocator::collectionHolder *holder)
   {
      SDB_ASSERT(NULL != holder && !holder->isFree(), "can not be empty");
      collection *cl = holder->getCollection();
      const CHAR *name = cl->getName();
      utilCLInnerID innerID = cl->getInnerID();
      UINT32 logicalID = cl->getLogicalID();
      _UID_HOLDER_PAIR p(holder, logicalID);
      BOOLEAN r = FALSE;
      ossScopedLock lock(&_latch, EXCLUSIVE);

      if (!_clNameIndex.insert(std::make_pair(name, p)).second)
      {
         goto done;
      }
      else if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         if (!_clIdIndex.insert(std::make_pair(innerID, p)).second)
         {
            _clNameIndex.erase(name);
            goto done;
         }
      }

      r = TRUE;
   done:
      return r;
   }

   void collectionSpace::eraseFromIndex(const strSlice &clName,
                                        utilCLInnerID innerID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      ossScopedLock lock(&_latch, EXCLUSIVE);
      _clNameIndex.erase(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _clIdIndex.erase(innerID);
      }

      return;
   }

   BOOLEAN collectionSpace::existsInFormalIndex(const strSlice &clName, utilCLInnerID innerID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      ossScopedLock lock(&_latch, SHARED);
      return (0 < _clNameIndex.count(clName.str()) ||
             (UTIL_IS_VALID_CL_INNERID(innerID) &&
              0 < _clIdIndex.count(innerID)));
             
   }


   INT32 collectionSpace::upperBoundCLName(const strSlice &clName,
                                           UINT32 bufferSize,
                                           CHAR *nameBuffer,
                                           UINT32 &logicalID,
                                           collectionAllocator::collectionHolder **holder)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT((DMS_COLLECTION_NAME_SZ + 1) <= bufferSize, "buffer size not enough");
      SDB_ASSERT(NULL != nameBuffer, "can not be null");
      SDB_ASSERT(NULL != holder, "can not be null");
      
      ossScopedLock lock(&_latch, SHARED);
      NAME_INDEX::const_iterator itr = _clNameIndex.begin();
      if (!clName.empty())
      {
         itr = _clNameIndex.upper_bound(clName.str());
      }

      if (_clNameIndex.end() != itr)
      {
         ossStrcpy(nameBuffer, itr->first);
         *holder = itr->second.holder;
         logicalID = itr->second.logicalID;
         r = TRUE;
      }
      
   done:
      return r;
   }

   BOOLEAN collectionSpace::findCollection(const strSlice &clName,
                                           UINT32 &logicalID,
                                           collectionAllocator::collectionHolder **holder)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      SDB_ASSERT(NULL != holder, "can not be null");
      ossScopedLock lock(&_latch, SHARED);
      NAME_INDEX::const_iterator itr = _clNameIndex.find(clName.str());
      if (_clNameIndex.end() != itr)
      {
         SDB_ASSERT(itr->second.isValid(), "must be valid");
         logicalID = itr->second.logicalID;
         *holder = itr->second.holder;
         return TRUE;
      }
      return FALSE;
   }

   BOOLEAN collectionSpace::findCollection(utilCLInnerID innerID,
                                           UINT32 &logicalID,
                                           collectionAllocator::collectionHolder **holder)
   {
      SDB_ASSERT(UTIL_IS_VALID_CL_INNERID(innerID), "can not be invalid");
      SDB_ASSERT(NULL != holder, "can not be null");
      ossScopedLock lock(&_latch, SHARED);
      ID_INDEX::const_iterator itr = _clIdIndex.find(innerID);
      if (_clIdIndex.end() != itr)
      {
         SDB_ASSERT(itr->second.isValid(), "must be valid");
         logicalID = itr->second.logicalID;
         *holder = itr->second.holder;
         return TRUE;
      }
      return FALSE;
   }

   INT32 collectionSpace::precreateCL(const strSlice &clName,
                                      utilCLInnerID innerID,
                                      UINT32 &logicalID)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(!clName.empty(), "can not be empty");
      ossScopedLock guard(&_latch, EXCLUSIVE);
      if (MAX_CL_MB_COUNT == _clNameIndex.size())
      {
         rc = SDB_DMS_NOSPC;
         goto error;
      }
      else if (0 != _clNameIndex.count(clName.str()))
      {
         rc = SDB_DMS_EXIST;
         goto error;
      }
      else if (UTIL_IS_VALID_CL_INNERID(innerID) &&
               0 < _clIdIndex.count(innerID))
      {
         rc = SDB_DMS_EXIST;
         goto error;
      }
      else if (DMS_INVALID_LOGICCLID == (_maxCLLogicalID + 1))
      {
         rc = SDB_DMS_NOSPC;
         goto error;
      }
      else if (0 < _creatingNameIndex.count(clName.str()))
      {
         rc = SDB_DMS_EXIST;
         goto error;
      }
      else if (UTIL_IS_VALID_CL_INNERID(innerID) &&
               0 < _creatingIdIndex.count(innerID))
      {
         rc = SDB_DMS_EXIST;
         goto error;
      }

      _creatingNameIndex.insert(clName.str());
      if (UTIL_IS_VALID_CL_INNERID(innerID))
      {
         _creatingIdIndex.insert(innerID);
      }

      logicalID = ++_maxCLLogicalID;
      
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpace::rollbackPrecreating(const strSlice &clName,
                                             utilCLInnerID innerID,
                                             UINT32 logicalID)
   {
      SDB_ASSERT(!clName.empty(), "can not be empty");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalID, "can not be invalid");
      ossScopedLock guard(&_latch, EXCLUSIVE);
      _creatingNameIndex.erase(clName.str());
      if (UTIL_INVALID_CL_INNER_ID != innerID)
      {
         _creatingIdIndex.erase(innerID);
      }
      if (logicalID == _maxCLLogicalID)
      {
         --_maxCLLogicalID;
      }
      return;
   }
   
}//namespace vessel
}//namespace engine