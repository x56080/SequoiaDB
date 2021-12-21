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

   Source File Name = ITransLockConsole.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_I_OBJECT_TRANS_LOCK_CONSOLE_H_
#define SDB_I_OBJECT_TRANS_LOCK_CONSOLE_H_

#include "dpsTransLockDef.hpp"
#include "sdbInterface.hpp"
#include "dpsTransLockCallback.hpp"

namespace engine
{
   class ITransLockConsole : public SDBObject
   {
      public:
         ITransLockConsole(){}
         virtual ~ITransLockConsole(){}

      public:
         virtual INT32 acquire(IExecutor *executor,
                               const dpsTransLockId &lockId,
                               const DPS_TRANSLOCK_TYPE &mode,
                               _IContext * pContext,
                               dpsTransRetInfo *pdpsTxResInfo,
                               _dpsITransLockCallback *callback) = 0;

         virtual void release(IExecutor *executor,
                              const dpsTransLockId &lockId,
                              BOOLEAN bForceRelease,
                              _dpsITransLockCallback * callback) = 0;

         virtual void releaseAll(IExecutor *executor,
                                 _dpsITransLockCallback *callback) = 0;

         virtual INT32 tryAcquire(IExecutor *executor,
                                  const dpsTransLockId &lockId,
                                  const DPS_TRANSLOCK_TYPE &mode,
                                  dpsTransRetInfo *pdpsTxResInfo,
                                  _dpsITransLockCallback *callback) = 0;

         virtual INT32 testAcquire(IExecutor *executor,
                                   const dpsTransLockId &lockId,
                                   const DPS_TRANSLOCK_TYPE &mode,
                                   BOOLEAN preemptMode,
                                   dpsTransRetInfo *pdpsTxResInfo,
                                   _dpsITransLockCallback *callback,
                                   BOOLEAN intentLock) = 0;
   };//class class ITransLockConsole
} // namespace engine


#endif//SDB_I_OBJECT_TRANS_LOCK_CONSOLE_H_
