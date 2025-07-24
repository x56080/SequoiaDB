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

   Source File Name = objectLatchHelper.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
                         objectSharedLatchContext<KEY, ossSharedLatch> &context)
         {
            typename objectSharedLatchContext<KEY, ossSharedLatch>::item i;
            while (context.popBack(i))
            {
               SDB_ASSERT(i.isValid(), "can not be invalid");
               i.obj.getValue().unlockWith(i.mode);
               latchMap.release(i.obj);
            }
            return;
         }

         void releaseAll(sharedObjectMap<KEY, ossSpinSLatchPOSIX> &latchMap,
                         objectSharedLatchContext<KEY, ossSpinSLatchPOSIX> &context)
         {
            typename objectSharedLatchContext<KEY, ossSpinSLatchPOSIX>::item i;
            while (context.popBack(i))
            {
               SDB_ASSERT(i.isValid(), "can not be invalid");
               if (i.mode.isShared())
               {
                  i.obj.getValue().release_shared();
               }
               else if (i.mode.isExclusive())
               {
                  i.obj.getValue().release();
               }
               else
               {
                  SDB_ASSERT(FALSE, "invalid mode");
               }
               latchMap.release(i.obj);
            }
            return;
         }

         void autoUnlock(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                         objectSharedLatchContext<KEY, ossSharedLatch> &context,
                         const KEY &key)
         {
            typename objectSharedLatchContext<KEY, ossSharedLatch>::item i;
            if (!context.findAndPop(key, i))
            {
               PD_LOG(PDERROR, "object latch key not found");
               SDB_ASSERT(FALSE, "object latch key not found");
            }
            else
            {
               SDB_ASSERT(i.isValid(), "can not be invalid");
               i.obj.getValue().unlockWith(i.mode);
               latchMap.release(i.obj);
            }
            return;
         }

         void autoUnlock(sharedObjectMap<KEY, ossSpinSLatchPOSIX> &latchMap,
                         objectSharedLatchContext<KEY, ossSpinSLatchPOSIX> &context,
                         const KEY &key)
         {
            typename objectSharedLatchContext<KEY, ossSpinSLatchPOSIX>::item i;
            if (!context.findAndPop(key, i))
            {
               PD_LOG(PDERROR, "object latch key not found");
               SDB_ASSERT(FALSE, "object latch key not found");
            }
            else
            {
               SDB_ASSERT(i.isValid(), "can not be invalid");
               if (i.mode.isShared())
               {
                  i.obj.getValue().release_shared();
               }
               else if (i.mode.isExclusive())
               {
                  i.obj.getValue().release();
               }
               else
               {
                  SDB_ASSERT(FALSE, "invalid mode");
               }
               latchMap.release(i.obj);
            }
            return;
         }

         INT32 lock(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                    objectSharedLatchContext<KEY, ossSharedLatch> &context,
                    const KEY &k,
                    const ossSharedLatchMode &mode)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(k.isValid(), "can not be invalid");
            SDB_ASSERT(!mode.isNone(), "can not be none");
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;

#if defined (_DEBUG)
            if (context.test(k, NULL))
            {
               PD_LOG(PDERROR, "key[%s] already locked", k.toString().c_str());
               SDB_ASSERT(FALSE, "do not relock");
               rc = SDB_INVALID_OPERATION;
               goto error;
            }
#endif//_DEBUG

            rc = latchMap.ensure(k, obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
               goto error;
            }

            obj.getValue().lockWith(mode);
            context.pushBack(obj, mode);
         done:
            return rc;
         error:
            goto done;
         }

         INT32 lockFromUpgradeToExclusive(objectSharedLatchContext<KEY, ossSharedLatch> &context,
                                          const KEY &k)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(k.isValid(), "can not be invalid");
            typename objectSharedLatchContext<KEY, ossSharedLatch>::item item;
            ossSharedLatchMode *mode = NULL;
            item = context.findToUpdate(k, &mode);
            if (!item.isValid())
            {
               rc = SDB_VESSEL_KEY_NOT_FOUND;
               goto error;
            }
            else if (!item.mode.isUpgrade())
            {
               rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
               goto error;
            }

            item.obj.getValue().unlockUpgradeAndLock();
            mode->setExclusive();
         done:
            return rc;
         error:
            goto done;
         }

         INT32 tryLockFromUpgradeToExclusive(objectSharedLatchContext<KEY, ossSharedLatch> &context,
                                             const KEY &key,
                                             BOOLEAN &locked)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(key.isValid(), "can not be invalid");
            locked = FALSE;
            typename objectSharedLatchContext<KEY, ossSharedLatch>::item item;
            ossSharedLatchMode *mode = NULL;
            item = context.findToUpdate(key, &mode);
            if (!item.isValid())
            {
               rc = SDB_VESSEL_KEY_NOT_FOUND;
               goto error;
            }
            else if (!item.mode.isUpgrade())
            {
               rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
               goto error;
            }

            locked = item.obj.getValue().tryUnlockUpgradeAndLock();
            if (locked)
            {
               mode->setExclusive();
            }
         done:
            return rc;
         error:
            goto done;
         }

         INT32 tryLockFromSharedToExclusive(objectSharedLatchContext<KEY, ossSharedLatch> &context,
                                            const KEY &key,
                                            BOOLEAN &locked)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(key.isValid(), "can not be invalid");
            locked = FALSE;
            typename objectSharedLatchContext<KEY, ossSharedLatch>::item item;
            ossSharedLatchMode *mode = NULL;
            item = context.findToUpdate(key, &mode);
            if (!item.isValid())
            {
               rc = SDB_VESSEL_KEY_NOT_FOUND;
               goto error;
            }
            else if (!item.mode.isShared())
            {
               rc = SDB_VESSEL_FORBIDDEN_OP_WLT;
               goto error;
            }

            locked = item.obj.getValue().tryUnlockSharedAndLock();
            if (locked)
            {
               mode->setExclusive();
            }
         done:
            return rc;
         error:
            goto done;
         }

         INT32 lock(sharedObjectMap<KEY, ossSpinSLatchPOSIX> &latchMap,
                    objectSharedLatchContext<KEY, ossSpinSLatchPOSIX> &context,
                    const KEY &k,
                    const ossSharedLatchMode &mode)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(k.isValid(), "can not be invalid");
            SDB_ASSERT(mode.isShared() || mode.isExclusive(), "must be shared/exclusive");
            typename sharedObjectMap<KEY, ossSpinSLatchPOSIX>::object obj;

#if defined (_DEBUG)
            if (context.test(k, NULL))
            {
               PD_LOG(PDERROR, "key[%s] already locked", k.toString().c_str());
               SDB_ASSERT(FALSE, "do not relock");
               rc = SDB_INVALID_OPERATION;
               goto error;
            }
#endif//_DEBUG

            rc = latchMap.ensure(k, obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
               goto error;
            }

            if (mode.isShared())
            {
               obj.getValue().get_shared();
            }
            else
            {
               obj.getValue().get();
            }
            context.pushBack(obj, mode);
         done:
            return rc;
         error:
            goto done;
         }
         

         INT32 tryLock(sharedObjectMap<KEY, ossSharedLatch> &latchMap,
                       objectSharedLatchContext<KEY, ossSharedLatch> &context,
                       const KEY &k,
                       const ossSharedLatchMode &mode,
                       BOOLEAN &locked)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(k.isValid(), "can not be invalid");
            SDB_ASSERT(!mode.isNone(), "can not be none");
            locked = FALSE;
            typename sharedObjectMap<KEY, ossSharedLatch>::object obj;

#if defined (_DEBUG)
            if (context.test(k, NULL))
            {
               PD_LOG(PDERROR, "key[%s] already locked", k.toString().c_str());
               SDB_ASSERT(FALSE, "do not relock");
               rc = SDB_INVALID_OPERATION;
               goto error;
            }
#endif//_DEBUG

            rc = latchMap.ensure(k, obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
               goto error;
            }

            locked = obj.getValue().tryLockWith(mode);
            if (locked)
            {
               context.pushBack(obj, mode);
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

         INT32 tryLock(sharedObjectMap<KEY, ossSpinSLatchPOSIX> &latchMap,
                       objectSharedLatchContext<KEY, ossSpinSLatchPOSIX> &context,
                       const KEY &k,
                       const ossSharedLatchMode &mode,
                       BOOLEAN &locked)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(k.isValid(), "can not be invalid");
            SDB_ASSERT(mode.isShared() || mode.isExclusive(), "must be shared/exclusive");
            locked = FALSE;
            typename sharedObjectMap<KEY, ossSpinSLatchPOSIX>::object obj;

#if defined (_DEBUG)
            if (context.test(k, NULL))
            {
               PD_LOG(PDERROR, "key[%s] already locked", k.toString().c_str());
               SDB_ASSERT(FALSE, "do not relock");
               rc = SDB_INVALID_OPERATION;
               goto error;
            }
#endif//_DEBUG

            rc = latchMap.ensure(k, obj);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to ensure latch obj:%d", rc);
               goto error;
            }

            if (mode.isShared())
            {
               locked = obj.getValue().try_get_shared();
            }
            else
            {
               locked = obj.getValue().try_get();
            }

            if (locked)
            {
               context.pushBack(obj, mode);
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

         void testNotExistsOrWait(sharedObjectMap<KEY, ossSpinSLatchPOSIX> &latchMap,
                                  const KEY &k,
                                  const ossSharedLatchMode &mode)
         {
            SDB_ASSERT(k.isValid(), "can not be invalid");
            SDB_ASSERT(mode.isShared() || mode.isExclusive(), "must be shared/exclusive");
            typename sharedObjectMap<KEY, ossSpinSLatchPOSIX>::object obj = latchMap.get(k);

            if (obj.isValid())
            {
               if (mode.isShared())
               {
                  obj.getValue().get_shared();
                  obj.getValue().release_shared();
               }
               else
               {
                  obj.getValue().get();
                  obj.getValue().release();
               }

               latchMap.release(obj);
            }

            return;
         }

   };//class objectLatchHelper
} // namespace vessel

} // namespace engine


#endif//VESSEL_OBJECT_LATCH_HELPER_H_