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

   Source File Name = dpsTransLockBucket.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSTRANSLOCKBUCKET_HPP_
#define DPSTRANSLOCKBUCKET_HPP_

#include "dpsTransLockDef.hpp"
#include "ossLatch.hpp"
#include <map>

namespace engine
{
   class dpsTransLockUnit;
   typedef std::map< dpsTransLockId, dpsTransLockUnit * > dpsTransLockUnitList;

   /*
      dpsLockBucket define
   */
   class dpsLockBucket : public SDBObject
   {
   public:
      friend class dpsTransLock;
   protected:
      dpsLockBucket();
      ~dpsLockBucket();
      INT32 acquire( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                     DPS_TRANSLOCK_TYPE lockType );
      INT32 upgrade( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                     DPS_TRANSLOCK_TYPE lockType );
      void release( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 test( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                  DPS_TRANSLOCK_TYPE lockType );

      INT32 tryAcquire( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                     DPS_TRANSLOCK_TYPE lockType );

      INT32 tryAcquireOrAppend( _pmdEDUCB *eduCB,
                                const dpsTransLockId &lockId,
                                DPS_TRANSLOCK_TYPE lockType,
                                BOOLEAN appendHead = FALSE );

      BOOLEAN hasWait( const dpsTransLockId &lockId );

      INT32 waitLockX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );


   private:
      INT32 appendToRun( _pmdEDUCB *eduCB, DPS_TRANSLOCK_TYPE lockType,
                         dpsTransLockUnit *pLockUnit,
                         const dpsTransLockId &lockId );

      void appendToWait( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                         dpsTransLockUnit *pLockUnit );

      void appendHeadToWait( _pmdEDUCB *eduCB, const dpsTransLockId &lockId,
                             dpsTransLockUnit *pLockUnit );

      void removeFromWait( _pmdEDUCB *eduCB,
                           dpsTransLockUnit *pLockUnit,
                           const dpsTransLockId &lockId );

      void removeFromRun( _pmdEDUCB *eduCB,
                          dpsTransLockUnit *pLockUnit );

      void wakeUp( _pmdEDUCB *eduCB );

      BOOLEAN isLockCompatible( DPS_TRANSLOCK_TYPE first,
                                DPS_TRANSLOCK_TYPE second );

      BOOLEAN checkCompatible( _pmdEDUCB *eduCB,
                               DPS_TRANSLOCK_TYPE lockType,
                               dpsTransLockUnit *pLockUnit,
                               const dpsTransLockId &lockID );

      INT32 waitLock( _pmdEDUCB *eduCB );


   private:
      dpsTransLockUnitList       _lockLst;
      ossSpinXLatch              _lstMutex;
      static ossSpinXLatch       _initMutex;
      static UINT32              _lockTimeout;  // The variable is shared by all lock-buckets
   };
}

#endif // DPSTRANSLOCKBUCKET_HPP_
