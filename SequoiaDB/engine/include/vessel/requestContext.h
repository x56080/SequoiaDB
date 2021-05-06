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
#include "vessel/lpidContext.h"

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

   static const UINT32 CONTEXT_DEFAULT_BUFFER_POOL_SIZE = 65536;
   class instanceEnv;
   class outerResource;

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

         INT32 lockSpaceID(SPACE_ID sid, OSS_LATCH_MODE mode);
         INT32 lockSpaceIDWithTimeout(SPACE_ID sid, OSS_LATCH_MODE mode, INT32 millis, BOOLEAN &locked);
         INT32 tryLockSpaceID(SPACE_ID sid, OSS_LATCH_MODE mode, BOOLEAN &locked);
         INT32 unlockSpaceID();

         OSS_INLINE BOOLEAN getSpaceIDLocked()const
         {
            return _spaceIDLocked;
         }

         OSS_INLINE OSS_LATCH_MODE getSpaceIDLockedMode()const
         {
            return _spaceIDLockMode;
         }

         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _spaceID;
         }
      public:
         INT32 attachMB(CL_MB_ID mbID, ossSpinSLatch *clLatch);
         INT32 detachMB();

         INT32 lockMB(CL_MB_ID mbID, ossSpinSLatch *clLatch, OSS_LATCH_MODE mode);
         INT32 tryLockMB(CL_MB_ID mid, ossSpinSLatch *clLatch, OSS_LATCH_MODE mode, BOOLEAN &locked);
         INT32 unlockMB();

         OSS_INLINE BOOLEAN mbLocked()const
         {
            return _mbIDLocked;
         }
         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _mbID;
         }
         OSS_INLINE OSS_LATCH_MODE getMBLockMode()const
         {
            return _mbIDLockMode;
         }

      public:
         /// no timeout. no recursive locking.
         INT32 lockLpid(FILE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode);
         INT32 unlockLpid(FILE_TYPE type, PAGE_ID lpid);
         BOOLEAN testLpidLockMode(FILE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode)const;
         BOOLEAN testLpidLocked(FILE_TYPE type, PAGE_ID lpid);

      private:
         void _close();

      private:
         OSS_INLINE void reset()
         {
            _session = NULL;
            _env = NULL;
            _outerResource = NULL;
            _spaceID = INVALID_SPACE_ID;
            _spaceIDLocked = FALSE;
            _spaceIDLockMode = SHARED;
            _mbID = INVALID_CL_MB_ID;
            _mbIDLockMode = SHARED;
            _mbIDLocked = FALSE;
            _clLatch = NULL;
            _lpidContext.reset();
            _bufAllocated = 0;
            return;
         }

      private:
         ISession *_session = NULL;
         instanceEnv *_env = NULL;
         outerResource *_outerResource = NULL;
         SPACE_ID _spaceID = INVALID_SPACE_ID;
         BOOLEAN _spaceIDLocked = FALSE;
         OSS_LATCH_MODE _spaceIDLockMode = SHARED;
         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         OSS_LATCH_MODE _mbIDLockMode = SHARED;
         BOOLEAN _mbIDLocked = FALSE;
         ossSpinSLatch *_clLatch = NULL;
         lpidContext _lpidContext;
         UINT32 _bufAllocated = 0;
         CHAR _staticBuf[CONTEXT_DEFAULT_BUFFER_POOL_SIZE];
         
   };//class requestContext
}
}

#endif//VESSEL_REQUEST_CONTEXT_H_