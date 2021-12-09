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
   class runtimeMbContext;

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

         BOOLEAN isMbLocked(OSS_LATCH_MODE *mode=NULL)const;

         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _mbID; 
         }

         /// do not release rmc outside until detaching.
         void attachMbContext(runtimeMbContext *rmc);

         void detachMbContext();

         OSS_INLINE BOOLEAN isMbContextAttached()const
         {
            return NULL != _rmc;
         }
         const runtimeMbContext *getMbContext()const
         {
            return _rmc;
         }

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
                                ossSharedLatchMode *mode=NULL);

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
                               ossSharedLatchMode *mode=NULL);

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

         void _unlockAll();

      private:
         IExecutor *_executor = NULL;
         instanceEnv *_env = NULL;
         outerResource *_outerResource = NULL;

         SPACE_ID _sid = INVALID_SPACE_ID;
         OSS_LATCH_MODE _sidMode = SHARED;
         
         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         ossRWMutex *_mbMutex = NULL;
         OSS_LATCH_MODE _mbMode = SHARED;
         runtimeMbContext *_rmc = NULL;

         LPID_LATCH_CONTEXT _lpidLatchContext;
         lpsCheckpointBlocker _blocker;

         UINT32 _bufAllocated = 0;
         CHAR _staticBuf[CONTEXT_DEFAULT_BUFFER_POOL_SIZE];

         atomicOperationList *_oplist = NULL;
   };//class requestContext
}
}

#endif//VESSEL_REQUEST_CONTEXT_H_