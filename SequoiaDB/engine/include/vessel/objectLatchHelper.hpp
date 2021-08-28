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

   Source File Name = objectLatchHelper.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OBJECT_LATCH_HELPER_H_
#define VESSEL_OBJECT_LATCH_HELPER_H_

#include "vessel/objectLatchMap.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   template<class KEY>
   class objectLatchHelper : public SDBObject
   {
      public:
         objectLatchHelper(){}
         ~objectLatchHelper(){}

      public:
         void releaseAll(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                         objectSharedLatchContext<KEY> &context)
         {
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;
            ossSharedLatchMode mode;
            while (context.pop(obj, mode))
            {
               obj.getValue().unlockWith(mode);
               latchMap.release(obj);
            }
            return;
         }

         void releaseAll(sharedObjectMap<KEY, ossSpinXLatch> &latchMap,
                         ossPoolVector<typename sharedObjectMap<KEY, ossSpinXLatch>::object> &context)
         {
            typename ossPoolVector<typename sharedObjectMap<KEY, ossSpinXLatch>::object>::reverse_iterator itr =
                                                                               context.rbegin();
            for (; itr != context.rend(); ++itr)
            {
               itr->getValue().release();
               latchMap.release(*itr);
            }
            context.clear();
         }

         INT32 tryLock(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                       objectSharedLatchContext<KEY> &context,
                       const KEY &k,
                       ossSharedLatchMode mode,
                       BOOLEAN &locked)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(!mode.isNone(), "can not be none");
            locked = FALSE;
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;
            rc = latchMap.ensure(k, obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
               goto error;
            }

            locked = obj.getValue().tryLockWith(mode);
            if (locked)
            {
               rc = context.push(obj, mode);
               if (SDB_OK != rc)
               {
                  obj.getValue().unlockWith(mode);
                  latchMap.release(obj);
                  locked = FALSE;
                  PD_LOG(PDERROR, "failed to push obj into context:%d", rc);
                  goto error;
               }
            }
            else
            {
               latchMap.release(obj);
            }
         done:
            return rc;
         error:
            goto done;
         }

         INT32 wait(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                    const KEY &k,
                    ossSharedLatchMode mode)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(!mode.isNone(), "can not be none");
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;
            rc = latchMap.ensure(k, obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
               goto error;
            }

            obj.getValue().lockWith(mode);
            obj.getValue().unlockWith(mode);
         done:
            if (obj.isValid())
            {
               latchMap.release(obj);
            }
            return rc;
         error:
            goto done;
         }

         INT32 waitFor(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                       const KEY &k,
                       ossSharedLatchMode mode,
                       UINT32 millis,
                       BOOLEAN &timeout)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(!mode.isNone(), "can not be none");
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;
            rc = latchMap.ensure(k, obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
               goto error;
            }

            if (!obj.getValue().tryLockWith(mode, millis))
            {
               timeout = TRUE;
            }
            else
            {
               obj.getValue().unlockWith(mode);
               timeout = FALSE;
            }
         done:
            if (obj.isValid())
            {
               latchMap.release(obj);
            }
            return rc;
         error:
            goto done;
         }

         INT32 tryLockWhenExists(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                                 objectSharedLatchContext<KEY> &context,
                                 const KEY &k,
                                 ossSharedLatchMode mode,
                                 BOOLEAN &notExists,
                                 BOOLEAN &locked)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(!mode.isNone(), "can not be none");
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;
            rc = latchMap.get(k, obj);
            if (SDB_OK != rc)
            {
               goto error;
            }
            else if (!obj.isValid())
            {
               notExists = TRUE;
               locked = FALSE;
            }
            else if (obj.getValue().tryLockWith(mode))
            {
               rc = context.push(obj, mode);
               if (SDB_OK != rc)
               {
                  obj.getValue().unlockWith(mode);
                  latchMap.release(obj);
                  PD_LOG(PDERROR, "failed to push obj to context:%d", rc);
                  goto error;
               }
               notExists = FALSE;
               locked = TRUE;
            }
            else
            {
               latchMap.release(obj);
               notExists = FALSE;
               locked = FALSE;
            }
         done:
            return rc;
         error:
            goto error;
         }

         void unlockLast(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                         objectSharedLatchContext<KEY> &context)
         {
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;
            ossSharedLatchMode mode;
            if (context.pop(obj, mode))
            {
               obj.getValue().unlockWith(mode);
               latchMap.release(obj);
            }
            return;
         }
   };//class objectLatchHelper
} // namespace vessel

} // namespace engine


#endif//VESSEL_OBJECT_LATCH_HELPER_H_