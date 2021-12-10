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

   Source File Name = dummyTransLockConsole.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "dummyTransLockConsole.h"
#include "pdTrace.hpp"
#include "test_def.h"


INT32 dummyTransLockConsole::acquire(IExecutor *executor,
                                    const dpsTransLockId &lockId,
                                    const DPS_TRANSLOCK_TYPE &mode,
                                    _IContext * pContext,
                                    dpsTransRetInfo *pdpsTxResInfo,
                                    _dpsITransLockCallback *callback)
{
   INT32 rc = SDB_OK;
   ossSharedLatchMode m;
   test_executor *te = dynamic_cast<test_executor *>(executor);
   if (NULL == te)
   {
      PD_LOG(PDERROR, "failed to cast to test_executor");
      rc = SDB_VESSEL_INTERNAL_ERR;
      goto error;
   }

   if (DPS_TRANSLOCK_S == mode)
   {
      m.setShared();
   }
   else if (DPS_TRANSLOCK_U == mode)
   {
      m.setUpgrade();
   }
   else if (DPS_TRANSLOCK_X == mode)
   {
      m.setExclusive();
   }
   else
   {
      PD_LOG(PDERROR, "invalid mode:%d", mode);
      rc = SDB_INVALIDARG;
      goto error;
   }

   {
      _LOCK_MAP::object o;
      rc = _lm.ensure(lockId, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure lock obj:%d", rc);
         goto error;
      }

      o.getValue().lockWith(m);
      te->_locked[lockId] = m;
   }
done:
   return rc;
error:
   goto done;
}

void dummyTransLockConsole::release(IExecutor *executor,
                                    const dpsTransLockId &lockId,
                                    BOOLEAN bForceRelease,
                                    _dpsITransLockCallback * callback)
{
   test_executor *te = dynamic_cast<test_executor *>(executor);
   if (NULL == te)
   {
      PD_LOG(PDERROR, "failed to cast to test_executor");
      SDB_ASSERT(FALSE, "failed to cast to test_executor");
   }

   ossPoolMap<_dpsTransLockId, ossSharedLatchMode>::const_iterator itr = te->_locked.find(lockId);
   SDB_ASSERT(itr != te->_locked.end(), "id not found");

   _LOCK_MAP::object o = _lm.get(lockId);
   SDB_ASSERT(o.isValid(), "obj not found");
   o.getValue().unlockWith(itr->second);
   te->_locked.erase(itr);
   _lm.release(o);
error:
   return;
}

void dummyTransLockConsole::releaseAll(IExecutor *executor,
                                       _dpsITransLockCallback *callback)
{
   test_executor *te = dynamic_cast<test_executor *>(executor);
   if (NULL == te)
   {
      PD_LOG(PDERROR, "failed to cast to test_executor");
      SDB_ASSERT(FALSE, "failed to cast to test_executor");
   }

   ossPoolMap<_dpsTransLockId, ossSharedLatchMode>::const_iterator itr = te->_locked.begin();
   for (; itr != te->_locked.end(); ++itr)
   {
      _LOCK_MAP::object o = _lm.get(itr->first);
      if (o.isValid())
      {
         o.getValue().unlockWith(itr->second);
         _lm.release(o);
      }
   }

   te->_locked.clear();
   return;
}      

INT32 dummyTransLockConsole::tryAcquire(IExecutor *executor,
                                       const dpsTransLockId &lockId,
                                       const DPS_TRANSLOCK_TYPE &mode,
                                       dpsTransRetInfo *pdpsTxResInfo,
                                       _dpsITransLockCallback *callback,
                                       BOOLEAN &locked)
{
   INT32 rc = SDB_OK;
   locked = FALSE;
   ossSharedLatchMode m;
   test_executor *te = dynamic_cast<test_executor *>(executor);
   if (NULL == te)
   {
      PD_LOG(PDERROR, "failed to cast to test_executor");
      rc = SDB_VESSEL_INTERNAL_ERR;
      goto error;
   }

   if (DPS_TRANSLOCK_S == mode)
   {
      m.setShared();
   }
   else if (DPS_TRANSLOCK_U == mode)
   {
      m.setUpgrade();
   }
   else if (DPS_TRANSLOCK_X == mode)
   {
      m.setExclusive();
   }
   else
   {
      PD_LOG(PDERROR, "invalid mode:%d", mode);
      rc = SDB_INVALIDARG;
      goto error;
   }

   {
      _LOCK_MAP::object o;
      rc = _lm.ensure(lockId, o);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure lock obj:%d", rc);
         goto error;
      }


      locked = o.getValue().tryLockWith(m);
      if (locked)
      {
         te->_locked[lockId] = m;
      }
      else
      {
         _lm.release(o);
      }
   }
done:
   return rc;
error:
   goto done;
}

INT32 dummyTransLockConsole::testAcquire(IExecutor *executor,
                                 const dpsTransLockId &lockId,
                                 const DPS_TRANSLOCK_TYPE &mode,
                                 BOOLEAN preemptMode,
                                 dpsTransRetInfo *pdpsTxResInfo,
                                 _dpsITransLockCallback *callback,
                                 BOOLEAN intentLock,
                                 BOOLEAN &compatible)
{
   SDB_ASSERT(FALSE, "not implemented");
   return SDB_VESSEL_INTERNAL_ERR;
}
