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
#include "vessel/ISession.h"
#include "vessel/pageDef.h"
#include "vessel/lpsCheckpointBlocker.h"
#include "vessel/objectLatchMap.hpp"

#define LOG_ERR_AND_REPORT(context, rc, fmt, ...) \
   do\
   {\
      PD_LOG(PDERROR, fmt, ##__VA_ARGS__); \
      if (OSS_LIKELY(context->isOpen()))\
      {\
         context->getSession()->setLastError(rc, fmt, ##__VA_ARGS__);\
      }\
   } while (0)

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
         OSS_INLINE requestContext()
         {}
         requestContext(const requestContext &) = delete;
         requestContext &operator=(const requestContext &) = delete;
         virtual ~requestContext();

      public:
         INT32 open(ISession *session,
                     instanceEnv *env,
                     outerResource *outer);
         virtual void close()
         {
            _close();
         }

         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _session;
         }

         OSS_INLINE ISession *getSession()const
         {
            return _session;
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

         OSS_INLINE BOOLEAN isSpaceIdLocked(OSS_LATCH_MODE *mode=NULL)const
         {
            BOOLEAN r = INVALID_SPACE_ID != _sid;
            if (r && NULL != mode)
            {
               *mode = _sidLockedMode;
            }
            return r;
         }

         OSS_INLINE OSS_LATCH_MODE getSpaceIDLockedMode()const
         {
            return _sidLockedMode;
         }

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }
      public:
         INT32 lockMB(CL_MB_ID mbID,
                      ossRWMutex *latch,
                      OSS_LATCH_MODE mode);

         INT32 tryLockMB(CL_MB_ID mbID,
                         ossRWMutex *latch,
                         OSS_LATCH_MODE mode,
                         BOOLEAN &locked);

         void setCLLoigcalIdUnderLock(UINT32 lid);

         OSS_INLINE UINT32 getCLLid()const
         {
            return _clLogicalId;
         }

         void unlockMB();

         OSS_INLINE BOOLEAN isMbLocked(OSS_LATCH_MODE *mode=NULL)const
         {
            BOOLEAN r = INVALID_CL_MB_ID != _mbID;
            if (r && NULL != mode)
            {
               *mode = _mbLockMode;
            }
            return r;
         }
         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _mbID;
         }
         OSS_INLINE OSS_LATCH_MODE getMBLockMode()const
         {
            return _mbLockMode;
         }

      public:
         /// no timeout. no recursive locking.
         INT32 lockLpid(SPACE_TYPE type,
                        PAGE_ID lpid,
                        OSS_SHARED_LATCH_MODE mode);

         void unlockLpid(SPACE_TYPE type, PAGE_ID lpid);
         BOOLEAN testLpidLocked(SPACE_TYPE type,
                                PAGE_ID lpid,
                                OSS_SHARED_LATCH_MODE *mode);

         INT32 unlockUpgradeLpidAndLock(SPACE_TYPE type,
                                        PAGE_ID lpid);

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

      private:
         void _close();

      private:
         ISession *_session = NULL;
         instanceEnv *_env = NULL;
         outerResource *_outerResource = NULL;

         SPACE_ID _sid = INVALID_SPACE_ID;
         OSS_LATCH_MODE _sidLockedMode = SHARED;

         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         OSS_LATCH_MODE _mbLockMode = SHARED;
         ossRWMutex *_mbLatch = NULL;
         UINT32 _clLogicalId = DMS_INVALID_LOGICCLID;

         objectSharedLatchContext<logicalIdLatchKey> _lpidLatchContext;

         UINT32 _bufAllocated = 0;
         CHAR _staticBuf[CONTEXT_DEFAULT_BUFFER_POOL_SIZE];

         atomicOperationList *_oplist = NULL;

         lpsCheckpointBlocker _blocker;
         
   };//class requestContext
}
}

#endif//VESSEL_REQUEST_CONTEXT_H_