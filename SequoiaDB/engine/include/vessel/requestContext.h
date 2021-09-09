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
#include "dms.hpp"
#include "vessel/collectionHandle.h"

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
   class spaceIDLocker;

   class collectionSpaceContext : public SDBObject
   {
      public:
         collectionSpaceContext(){}
         ~collectionSpaceContext();
         collectionSpaceContext(const collectionSpaceContext &) = delete;
         collectionSpaceContext &operator=(const collectionSpaceContext &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return INVALID_SPACE_ID != _sid;
         }

         INT32 lockSid(SPACE_ID sid,
                       OSS_LATCH_MODE mode,
                       spaceIDLocker *locker);

         INT32 tryLockSid(SPACE_ID sid,
                          OSS_LATCH_MODE mode,
                          spaceIDLocker *locker,
                          BOOLEAN &locked);

         void close(spaceIDLocker *locker);

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }

         OSS_LATCH_MODE getLockingMode()const;

         OSS_INLINE void setLogicalCSID(UINT32 lcsid)
         {
            _lid = lcsid;
         }
         OSS_INLINE UINT32 getLogicalCSID()const
         {
            return _lid;
         }
         OSS_INLINE void setCSName(const strSlice &name)
         {
            _csName = name;
         }
         OSS_INLINE const strSlice &getCSName()const
         {
            return _csName;
         }

      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         OSS_LATCH_MODE _mode = SHARED;
         UINT32 _lid = DMS_INVALID_LOGICCSID;
         strSlice _csName;

   };//class collectionSpaceContext

   class collectionContext : public SDBObject
   { 
      public:
         collectionContext(){}
         ~collectionContext();
         collectionContext(const collectionContext &) = delete;
         collectionContext &operator=(const collectionContext &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return INVALID_CL_MB_ID != _mbID;
         }

         void lockMB(CL_MB_ID mbID,
                      ossRWMutex *latch,
                      OSS_LATCH_MODE mode);

         BOOLEAN tryLockMB(CL_MB_ID mbID,
                         ossRWMutex *latch,
                         OSS_LATCH_MODE mode);
                         
         void close();

         BOOLEAN isMbIDLocked(OSS_LATCH_MODE *mode=NULL)const;

         void initUnderLock(UINT32 logicalCLID, const strSlice &clName);

         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _mbID;
         }

         OSS_INLINE void setLogicalCLID(UINT32 lclid)
         {
            _lid = lclid;
         }

         OSS_INLINE UINT32 getLogicalCLID()const
         {
            return _lid;
         }

         OSS_INLINE void setCLName(const strSlice &clName)
         {
            _name = clName;
         }
         OSS_INLINE const strSlice &getCLName()const
         {
            return _name;
         }
      private:
         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         OSS_LATCH_MODE _mode = SHARED;
         ossRWMutex *_mbLatch = NULL;
         UINT32 _lid = DMS_INVALID_LOGICCLID;
         strSlice _name;

   };//class collectionContext

   class requestContext : public SDBObject
   {
      public:
         OSS_INLINE requestContext(){}
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

         BOOLEAN isSpaceIdLocked(OSS_LATCH_MODE *mode=NULL)const;

         void initSpaceContextUnderLock(UINT32 lcsid,
                                        const strSlice &csName);

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _spaceContext.getSpaceID();
         }

         OSS_INLINE UINT32 getLogicalCSID()const
         {
            return _spaceContext.getLogicalCSID();
         }

         OSS_INLINE const strSlice &getCSName()const
         {
            return _spaceContext.getCSName();
         }
      public:
         /// space must be locked first
         void lockMB(CL_MB_ID mbID,
                      ossRWMutex *latch,
                      OSS_LATCH_MODE mode);

         BOOLEAN tryLockMB(CL_MB_ID mbID,
                           ossRWMutex *latch,
                           OSS_LATCH_MODE mode);

         void initCollectionContextUnderLock(UINT32 logicalCLID,
                                             const strSlice &clName);

         OSS_INLINE UINT32 getLogicalCLID()const
         {
            return _clContext.getLogicalCLID();
         }
         OSS_INLINE const strSlice &getCLName()const
         {
            return _clContext.getCLName();
         }

         void unlockMB();

         BOOLEAN isMbLocked(OSS_LATCH_MODE *mode=NULL)const;

         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _clContext.getMBID();
         }

         collectionHandle getCollectionHandle()const;

      public:
         /// no timeout. no recursive locking.
         INT32 lockLpid(SPACE_TYPE type,
                        PAGE_ID lpid,
                        const ossSharedLatchMode &mode);

         void unlockLpid(SPACE_TYPE type, PAGE_ID lpid);

         /// test locking in current context
         BOOLEAN testLpidLocked(SPACE_TYPE type,
                                PAGE_ID lpid,
                                ossSharedLatchMode *mode);

         objectSharedLatchContext<logicalIdLatchKey> &
         getLpidLatchContext()
         {
            return _lpidLatchContext;
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

      private:
         void _close();

      private:
         ISession *_session = NULL;
         instanceEnv *_env = NULL;
         outerResource *_outerResource = NULL;

         collectionSpaceContext _spaceContext;
         collectionContext _clContext;
         objectSharedLatchContext<logicalIdLatchKey> _lpidLatchContext;
         lpsCheckpointBlocker _blocker;

         UINT32 _bufAllocated = 0;
         CHAR _staticBuf[CONTEXT_DEFAULT_BUFFER_POOL_SIZE];

         atomicOperationList *_oplist = NULL;
   };//class requestContext
}
}

#endif//VESSEL_REQUEST_CONTEXT_H_