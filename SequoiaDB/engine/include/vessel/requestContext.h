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

   Source File Name = requestContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_REQUEST_CONTEXT_H_
#define VESSEL_REQUEST_CONTEXT_H_

#include "vessel/vesselIdDef.h"
#include "ossLatch.hpp"
#include "ossLikely.hpp"
#include "vessel/pageDef.h"
#include "vessel/lpsCheckpointBlocker.h"
#include "vessel/objectLatchMap.hpp"
#include "dms.hpp"
#include "sdbInterface.hpp"
#include "vessel/objectIdentifier.h"

/*
#define LOG_ERR_AND_REPORT(context, rc, fmt, ...) \
   do\
   {\
      PD_LOG(PDERROR, fmt, ##__VA_ARGS__); \
      if (OSS_LIKELY(context->isOpen()))\
      {\
         context->getSession()->setLastError(rc, fmt, ##__VA_ARGS__);\
      }\
   } while (0)
*/
namespace engine
{
namespace vessel
{

   static const UINT32 CONTEXT_DEFAULT_BUFFER_POOL_SIZE = 8192;
   class instanceEnv;
   class outerResource;
   class atomicOperationList;

   class requestContext : public SDBObject
   {
      public:
         OSS_INLINE requestContext(){}
         requestContext(const requestContext &) = delete;
         requestContext &operator=(const requestContext &) = delete;
         virtual ~requestContext();

      public:
         void open(IExecutor *executor,
                   instanceEnv *env,
                   outerResource *outer);
         virtual void close()
         {
            _close();
         }

         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _executor;
         }

         OSS_INLINE IExecutor *getExecutor()const
         {
            return _executor;
         }

         OSS_INLINE instanceEnv *getEnv()const
         {
            return _env;
         }

         OSS_INLINE outerResource *getOuterResource()const
         {
            return _outerResource;
         }

      public:
         CHAR *allocateBuffer(UINT32 size);
         void releaseBuffer(CHAR *buffer, UINT32 size);

      public:

         INT32 lockSpaceID(SPACE_ID sid,
                           OSS_LATCH_MODE mode);

         INT32 tryLockSpaceID(SPACE_ID sid,
                              OSS_LATCH_MODE mode,
                              BOOLEAN &locked);

         void unlockSpaceID();

         BOOLEAN isSpaceIdLocked(OSS_LATCH_MODE *mode=NULL)const;


         void cacheSpaceInfo(UINT32 logicalId,
                             utilCSUniqueID uniqueId,
                             const CHAR *name);

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _spaceContext.sid;
         }

         OSS_INLINE UINT32 getLogicalCSID()const
         {
            return _spaceContext.lid;
         }

         OSS_INLINE const strSlice &getCSName()const
         {
            return _spaceContext.name;
         }
      public:
         /// space must be locked first
         void lockMB(CL_MB_ID mbID,
                      ossRWMutex *mutex,
                      OSS_LATCH_MODE mode);

         BOOLEAN tryLockMB(CL_MB_ID mbID,
                           ossRWMutex *mutex,
                           OSS_LATCH_MODE mode);

         /// must lock space first
         void cacheMbInfo(UINT32 logicalId,
                          utilCLInnerID innerId,
                          const CHAR *name);

         OSS_INLINE UINT32 getLogicalCLID()const
         {
            return _clContext.lid;
         }
         OSS_INLINE const strSlice &getCLName()const
         {
            return _clContext.name;
         }

         void unlockMB();

         BOOLEAN isMbLocked(OSS_LATCH_MODE *mode=NULL)const;

         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _clContext.mbID;
         }

         globalCollectionId getGlobalCollectionId()const;

      public:
         /// no timeout. no recursive locking.
         INT32 lockLpid(SPACE_TYPE type,
                        PAGE_ID lpid,
                        const ossSharedLatchMode &mode);

         INT32 tryLockLpid(SPACE_TYPE type,
                           PAGE_ID lpid,
                           const ossSharedLatchMode &mode,
                           BOOLEAN &locked);

         void unlockLpid(SPACE_TYPE type, PAGE_ID lpid);

         /// test locking in current context
         BOOLEAN testLpidLocked(SPACE_TYPE type,
                                PAGE_ID lpid,
                                ossSharedLatchMode *mode);

         LPID_LATCH_CONTEXT &getLpidLatchContext()
         {
            return _lpidLatchContext;
         }

      public:

         INT32 lockRid(const recordID &rid,
                       const ossSharedLatchMode &mode);
         INT32 tryLockRid(const recordID &rid,
                          const ossSharedLatchMode &mode,
                          BOOLEAN &locked);
         void unlockRid(const recordID &rid);
         void unlockRids();

         const RID_LATCH_CONTEXT &getRidLatchContext()const
         {
            return _ridLatchContext;
         }
      public:
         INT32 blockCheckpoint(SPACE_TYPE type,
                               ossRWMutex *mutex);

         INT32 tryToBlockCheckpoint(SPACE_TYPE type,
                                    ossRWMutex *mutex,
                                    BOOLEAN &blocked);

         void unblockCheckpoint();

      public:
         OSS_INLINE void attachOplist(atomicOperationList *oplist)
         {
            _oplist = oplist;
         }
         OSS_INLINE void detachOplist()
         {
            _oplist = NULL;
         }
         OSS_INLINE atomicOperationList *getOplist()
         {
            return _oplist;
         }
         OSS_INLINE BOOLEAN isOplistAttached()const
         {
            return NULL != _oplist;
         }
         void swtichOplist(atomicOperationList *newOplist,
                           atomicOperationList **oldOplist);
         BOOLEAN isInProcessingOplist()const;

      public:
         DPS_TRANS_ID getTransIDWithoutTag()const
         {
            return _executor->getTransID().getOrigTransID();
         }

      private:
         void _close();

         enum _UNLOCK_LVL
         {
            _UNLOCK_LVL_RID = 1,
            _UNLOCK_LVL_MB = 2,
            _UNLOCK_LVL_SPACE = 3,
         };//enum _UNLOCK_LVL

         void _unlockAndClear(_UNLOCK_LVL lvl);

      private:
         struct _collectionSpaceContext : public SDBObject
         {
            SPACE_ID sid = INVALID_SPACE_ID;
            OSS_LATCH_MODE mode = SHARED;
            UINT32 lid = DMS_INVALID_LOGICCSID;
            utilCSUniqueID uniqueId = UTIL_UNIQUEID_NULL;
            strSlice name; 

            OSS_INLINE BOOLEAN isLocking()const
            {
               return INVALID_SPACE_ID != sid;
            }
            OSS_INLINE void reset()
            {
               sid = INVALID_SPACE_ID;
               mode = SHARED;
               lid = DMS_INVALID_LOGICCSID;
               uniqueId = UTIL_UNIQUEID_NULL;
               name.reset();
            }
         };//struct _collectionSpaceContext

         struct _collectionContext : public SDBObject
         {
            OSS_INLINE void reset()
            {
               mbID = INVALID_CL_MB_ID;
               mode = SHARED;
               mutex = NULL;
               lid = DMS_INVALID_LOGICCLID;
               innerId = UTIL_UNIQUEID_NULL;
               name.reset();
            }
            OSS_INLINE BOOLEAN isLocking()const
            {
               return NULL != mutex;
            }

            CL_MB_ID mbID = INVALID_CL_MB_ID;
            OSS_LATCH_MODE mode = SHARED;
            ossRWMutex *mutex = NULL;
            UINT32 lid = DMS_INVALID_LOGICCLID;
            utilCLInnerID innerId = UTIL_UNIQUEID_NULL;
            strSlice name;
         };//struct _collectionContext

      private:
         IExecutor *_executor = NULL;
         instanceEnv *_env = NULL;
         outerResource *_outerResource = NULL;

         _collectionSpaceContext _spaceContext;
         _collectionContext _clContext;
         LPID_LATCH_CONTEXT _lpidLatchContext;
         RID_LATCH_CONTEXT _ridLatchContext;
         lpsCheckpointBlocker _blocker;

         UINT32 _bufAllocated = 0;
         CHAR _staticBuf[CONTEXT_DEFAULT_BUFFER_POOL_SIZE];

         atomicOperationList *_oplist = NULL;
   };//class requestContext
}
}

#endif//VESSEL_REQUEST_CONTEXT_H_