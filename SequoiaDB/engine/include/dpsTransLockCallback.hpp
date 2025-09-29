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

   Source File Name = dpsTransLockCallback.hpp

   Descriptive Name = DPS Transaction Lock Callback Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          19/01/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_TRANS_LOCK_CALLBACK_HPP__
#define DPS_TRANS_LOCK_CALLBACK_HPP__

#include "oss.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransDef.hpp"
#include "dpsTransLRB.hpp"

namespace engine
{

   /*
      _dpsITransLockCallback interface
   */
   class _dpsITransLockCallback : public SDBObject
   {
   public:
      _dpsITransLockCallback()
      {
      }

      virtual ~_dpsITransLockCallback()
      {
      }

      /// Interface
      virtual void afterLockAcquire( const dpsTransLockId &lockId,
                                     INT32 irc,
                                     DPS_TRANSLOCK_TYPE requestLockMode,
                                     UINT32 refCounter,
                                     DPS_TRANSLOCK_OP_MODE_TYPE opMode,
                                     const dpsTransLRBHeader *pLRBHeader,
                                     dpsLRBExtData *pExtData ) = 0 ;

      virtual void beforeLockRelease( const dpsTransLockId &lockId,
                                      DPS_TRANSLOCK_TYPE lockMode,
                                      UINT32 refCounter,
                                      const dpsTransLRBHeader *pLRBHeader,
                                      dpsLRBExtData *pExtData ) = 0 ;

      virtual INT32 afterLockEscalated( const dpsTransLockId &lockId,
                                        DPS_TRANSLOCK_OP_MODE_TYPE opMode ) = 0 ;

   } ;

}

#endif // DPS_TRANS_LOCK_CALLBACK_HPP__

