/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dpsTransLock.hpp

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

#ifndef DPSTRANSLOCK_HPP_
#define DPSTRANSLOCK_HPP_

#include "dms.hpp"
#include "ossLatch.hpp"
#include "dpsTransLockDef.hpp"
#include <vector>

namespace engine
{
   class _pmdEDUCB;
   class dpsLockBucket;


   #define MAX_LOCKBUCKET_NUM             ( 1000 )

   /*
      dpsTransLock define
   */
   class dpsTransLock : public SDBObject
   {
   typedef std::vector< dpsLockBucket *> LockBucketLst;
   public:
      dpsTransLock();

      ~dpsTransLock();

      // get record-X-lock: also get the space-S-lock and collection-IX-lock
      // get collection-X-lock: also get the space-S-lock
      INT32 acquireX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // get record-S-lock: also get the space-S-lock and collection-IS-lock
      // get collection-S-lock: also get the space-S-lock
      INT32 acquireS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // also get the space-S-lock
      INT32 acquireIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // also get the space-S-lock
      INT32 acquireIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // release record-lock: also release the space-lock and collection-lock
      // release collection-lock: also release the space-lock
      void release( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      void releaseAll( _pmdEDUCB *eduCB );

      // not get the lock only test if the lock can be got.
      // test record-S-lock: also test the space-S-lock and collection-IS-lock
      // test collection-S-lock: also test the space-S-lock
      INT32 testS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // also test the space-S-lock
      INT32 testIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // not get the lock only test if the lock can be got.
      // test record-X-lock: also test the space-S-lock and collection-IX-lock
      // test collection-X-lock: also test the space-S-lock
      INT32 testX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // also test the space-S-lock
      INT32 testIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // try to get record-X-lock: also try to get the space-S-lock and 
      // collection-IX-lock
      // try to get collection-X-lock: also try to get the space-S-lock
      INT32 tryX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // try to get record-S-lock: also try to get the space-S-lock and 
      // collection-IS-lock
      // try to get collection-S-lock: also try to get the space-S-lock
      INT32 tryS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // also get the space-S-lock
      INT32 tryIX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // also get the space-S-lock
      INT32 tryIS( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      // try to get record-X-lock: also try to get the space-S-lock and 
      // collection-IX-lock
      // if get lock failed then append to wait-queue but not wait
      INT32 tryOrAppendX( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      INT32 wait( _pmdEDUCB *eduCB, const dpsTransLockId &lockId );

      BOOLEAN hasWait( const dpsTransLockId &lockId );


   private:
      INT32 upgrade( _pmdEDUCB *eduCB,
                     const dpsTransLockId &lockId,
                     dpsTransCBLockInfo *pLockInfo,
                     DPS_TRANSLOCK_TYPE lockType );

      INT32 testUpgrade( _pmdEDUCB *eduCB,
                         const dpsTransLockId &lockId,
                         dpsTransCBLockInfo *pLockInfo,
                         DPS_TRANSLOCK_TYPE lockType );

      INT32 tryUpgrade( _pmdEDUCB *eduCB,
                        const dpsTransLockId &lockId,
                        dpsTransCBLockInfo *pLockInfo,
                        DPS_TRANSLOCK_TYPE lockType );

      INT32 upgradeCheck( DPS_TRANSLOCK_TYPE srcType,
                          DPS_TRANSLOCK_TYPE dstType );

      INT32 getBucket( const dpsTransLockId &lockId,
                       dpsLockBucket *&lockBucket );

      UINT32 getBucketNo( const dpsTransLockId &lockId );

      INT32 tryUpgradeOrAppendHead( _pmdEDUCB *eduCB,
                                    const dpsTransLockId &lockId,
                                    dpsTransCBLockInfo *pLockInfo,
                                    DPS_TRANSLOCK_TYPE lockType ) ;

   private:
      LockBucketLst           _bucketLst;
      ossSpinXLatch           _LstMutex;

   } ;

   class DPS_TRANS_WAIT_LOCK
   {
   public:

      DPS_TRANS_WAIT_LOCK( _pmdEDUCB *eduCB, const dpsTransLockId & lockId ) ;

      DPS_TRANS_WAIT_LOCK( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID,
                           const _dmsRecordID *recordID ) ;

      ~DPS_TRANS_WAIT_LOCK() ;

   private:
      _pmdEDUCB         *_eduCB ;       
   };

}

#endif //DPSTRANSLOCK_HPP_

