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

   Source File Name = dummyTransLockConsole.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
                                 _dpsITransLockCallback *callback);

      virtual INT32 testAcquire(IExecutor *executor,
                                 const dpsTransLockId &lockId,
                                 const DPS_TRANSLOCK_TYPE &mode,
                                 BOOLEAN preemptMode,
                                 dpsTransRetInfo *pdpsTxResInfo,
                                 _dpsITransLockCallback *callback,
                                 BOOLEAN intentLock) ;

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
