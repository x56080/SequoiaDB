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

   Source File Name = dummyTransLockConsole.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DUMMY_TRANS_LOCK_CONSOLE_H_
#define VESSEL_DUMMY_TRANS_LOCK_CONSOLE_H_

#include "interface/ITransLockConsole.h"
#include "vessel/sharedObjectMap.hpp"
#include "ossSharedLatch.hpp"
#include <gtest/gtest.h>

using namespace engine;
using namespace engine::vessel;

class dummyTransLockConsole : public ITransLockConsole
{
   public:
      dummyTransLockConsole(){}
      virtual ~dummyTransLockConsole(){}

   public:
      virtual INT32 acquire(IExecutor *executor,
                              const dpsTransLockId &lockId,
                              const DPS_TRANSLOCK_TYPE &mode,
                              _IContext * pContext,
                              dpsTransRetInfo *pdpsTxResInfo,
                              _dpsITransLockCallback *callback);

      virtual void release(IExecutor *executor,
                           const dpsTransLockId &lockId,
                           BOOLEAN bForceRelease,
                           _dpsITransLockCallback * callback);

      virtual void releaseAll(IExecutor *executor,
                              _dpsITransLockCallback *callback);

      virtual INT32 tryAcquire(IExecutor *executor,
                                 const dpsTransLockId &lockId,
                                 const DPS_TRANSLOCK_TYPE &mode,
                                 dpsTransRetInfo *pdpsTxResInfo,
                                 _dpsITransLockCallback *callback,
                                 BOOLEAN &locked);

      virtual INT32 testAcquire(IExecutor *executor,
                                 const dpsTransLockId &lockId,
                                 const DPS_TRANSLOCK_TYPE &mode,
                                 BOOLEAN preemptMode,
                                 dpsTransRetInfo *pdpsTxResInfo,
                                 _dpsITransLockCallback *callback,
                                 BOOLEAN intentLock,
                                 BOOLEAN &compatible) ;

      static dummyTransLockConsole *instance()
      {
         static dummyTransLockConsole console;
         return &console;
      }

   public:
      typedef class sharedObjectMap<dpsTransLockId, ossSharedLatch> _LOCK_MAP;
      _LOCK_MAP _lm;
};//class dummyTransLockConsole


#endif//VESSEL_DUMMY_TRANS_LOCK_CONSOLE_H_
