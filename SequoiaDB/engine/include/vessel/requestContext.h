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

   Source File Name = requestContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_REQUEST_CONTEXT_H_
#define VESSEL_REQUEST_CONTEXT_H_

#include "vessel/vesselIdDef.h"
#include "ossLatch.hpp"
#include "ossLikely.hpp"
#include "vessel/pageDef.h"
#include "vessel/objectLatchMap.hpp"
#include "dms.hpp"
#include "sdbInterface.hpp"
#include "vessel/objectIdentifier.h"
#include "vessel/shallowPointer.hpp"
#include "dpsTransLockDef.hpp"
#include "vessel/threadContext.h"

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
   class instanceEnv;
   class outerResource;
   class atomicOperationList;
   struct collectionProperties;

   class requestContext : public SDBObject
   {
      public:
         requestContext() = default;
         requestContext(const requestContext &) = delete;
         requestContext &operator=(const requestContext &) = delete;
         virtual ~requestContext();

      public:
         void close();

         IExecutor *getExecutor()const;

         instanceEnv *getEnv()const;

         outerResource *getOuterResource()const;

         DPS_TRANS_ID getOrigTransId()const;

      public:
         CHAR *allocateBuffer(UINT32 size);
         void releaseBuffer(void *buffer);

         template<class T>
         shallowArray<T> allocateArray(UINT32 size)
         {
            return GET_THREAD_CONTEXT()->allocateArray<T>(size);
         }
         
      public:

         INT32 lockSpaceID(SPACE_ID sid,
                           OSS_LATCH_MODE mode);

         INT32 tryLockSpaceID(SPACE_ID sid,
                              OSS_LATCH_MODE mode,
                              BOOLEAN &locked);

         BOOLEAN isSpaceIdLocked(OSS_LATCH_MODE *mode=nullptr)const;

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }
      public:
         /// space must be locked first
         void lockMB(CL_MB_ID mbID,
                      ossRWMutex *mutex,
                      OSS_LATCH_MODE mode);

         BOOLEAN tryLockMB(CL_MB_ID mbID,
                           ossRWMutex *mutex,
                           OSS_LATCH_MODE mode);

         void unlockMB();

         BOOLEAN isMbLocked(OSS_LATCH_MODE *mode=nullptr)const;

         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _mbID; 
         }

         void setClProperties(const collectionProperties *properties);
         void resetClProperties() {_clProperties = nullptr;}
         OSS_INLINE const collectionProperties *getClProperties()const {return _clProperties;}
         OSS_INLINE BOOLEAN isClPropertiesSet()const {return nullptr != _clProperties;}
         /// properties must be set
         UINT32 getLogicalClId()const;
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

         /// test locking in local context
         BOOLEAN testLpidLocked(SPACE_TYPE type,
                                PAGE_ID lpid,
                                ossSharedLatchMode *mode=nullptr);

         INT32 lockFromUpgradeToExclusive(SPACE_TYPE type,
                                          PAGE_ID lpid);

         /// will not release upgrade latch if failed to try
         INT32 tryLockFromUpgradeToExclusive(SPACE_TYPE type,
                                             PAGE_ID lpid,
                                             BOOLEAN &locked);

         /// will not release shared latch if failed to try
         INT32 tryLockFromSharedToExclusive(SPACE_TYPE type,
                                            PAGE_ID lpid,
                                            BOOLEAN &locked);

      public:/// attach mb first

         INT32 lockRid(const recordID &rid,
                       const ossSharedLatchMode &mode);
         INT32 tryLockRid(const recordID &rid,
                          const ossSharedLatchMode &mode,
                          BOOLEAN &locked);
         void unlockRid(const recordID &rid);
         void unlockRids();

         BOOLEAN testRidLocked(const recordID &rid,
                               ossSharedLatchMode *mode=nullptr);

         void waitRid(const recordID &rid,
                      const ossSharedLatchMode &mode);

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
            _oplist = nullptr;
         }
         OSS_INLINE atomicOperationList *getOplist()
         {
            return _oplist;
         }
         OSS_INLINE BOOLEAN isOplistAttached()const
         {
            return nullptr != _oplist;
         }
         void swtichOplist(atomicOperationList *newOplist,
                           atomicOperationList **oldOplist);
         BOOLEAN isInProcessingOplist()const;
      public:
         INT32 acquireTransLock(const recordID &rid,
                                const DPS_TRANSLOCK_TYPE &mode);
         INT32 tryAcquireTransLock(const recordID &rid,
                                   const DPS_TRANSLOCK_TYPE &mode,
                                   BOOLEAN &locked);

         void releaseTransLock(const recordID &rid);

         void releaseAllTransLock();

         INT32 waitTransLock(const recordID &rid,
                             const DPS_TRANSLOCK_TYPE &mode);
      private:
         virtual void _onClose() {}

         void _close();

      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         OSS_LATCH_MODE _sidMode = SHARED;
         
         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         ossRWMutex *_mbMutex = nullptr;
         OSS_LATCH_MODE _mbMode = SHARED;
         const collectionProperties *_clProperties = nullptr;

         LPID_LATCH_CONTEXT _lpidLatchContext;
         RID_LATCH_CONTEXT _ridLatchContext;

         atomicOperationList *_oplist = nullptr;
   };//class requestContext
}
}

#endif//VESSEL_REQUEST_CONTEXT_H_