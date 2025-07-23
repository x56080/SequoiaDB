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

   Source File Name = dpsTransExecutor.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_TRANS_EXECUTOR_HPP__
#define DPS_TRANS_EXECUTOR_HPP__

#include "sdbInterface.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransDef.hpp"
#include "dpsTransLockMgr.hpp"
#include "dpsTransArbit.hpp"
#include "monClass.hpp"
#include "monMgr.hpp"
#include "utilSegment.hpp"
#include "ossMemPool.hpp"
#include "dpsDef.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   class dpsTransLRBHeader;

   /*
      DPS_TRANS_QUE_TYPE define
   */
   enum DPS_TRANS_QUE_TYPE
   {
      DPS_QUE_NULL         = 0,
      DPS_QUE_UPGRADE,
      DPS_QUE_WAITER
   } ;

   /*
      _dpsTransConfItem define
   */
   class _dpsTransConfItem
   {
      public:
         _dpsTransConfItem() ;
         virtual ~_dpsTransConfItem() ;

      public:
         INT32                getTransIsolation() const ;
         UINT32               getTransTimeout() const ;
         BOOLEAN              isTransWaitLock() const ;
         BOOLEAN              useRollbackSegment() const ;
         BOOLEAN              isTransAutoCommit() const ;
         BOOLEAN              isTransAutoRollback() const ;
         BOOLEAN              isTransRCCount() const ;

         UINT32               getTransConfMask() const ;
         UINT32               getTransConfVer() const ;

         void                 setTransIsolation( INT32 isolation,
                                                 BOOLEAN enableMask = TRUE ) ;
         void                 setTransTimeout( UINT32 timeout,
                                               BOOLEAN enableMask = TRUE ) ;
         void                 setTransWaitLock( BOOLEAN waitLock,
                                                BOOLEAN enableMask = TRUE ) ;
         void                 setUseRollbackSemgent( BOOLEAN use,
                                                     BOOLEAN enableMask = TRUE ) ;
         void                 setTransAutoCommit( BOOLEAN autoCommit,
                                                  BOOLEAN enableMask = TRUE ) ;
         void                 setTransAutoRollback( BOOLEAN autoRollback,
                                                    BOOLEAN enableMask = TRUE ) ;
         void                 setTransRCCount ( BOOLEAN rcCount,
                                                BOOLEAN enableMask = TRUE ) ;

         void                 reset() ;
         void                 resetConfMask() ;
         void                 resetConfMask( UINT32 bitMask ) ;

         void                 updateByMask( const _dpsTransConfItem &rhs ) ;
         void                 copyFrom( const _dpsTransConfItem &rhs ) ;

         void                 toBson( BSONObjBuilder &builder ) const ;
         void                 fromBson( const BSONObj &obj ) ;

      protected:
         INT32                   _transIsolation ;
         UINT32                  _transTimeout ;      /// Unit:ms
         // if transaction wait for lock
         BOOLEAN                 _transWaitLock ;
         // if transaction use old copy in rollback segment
         BOOLEAN                 _useRollbackSegment ;
         // insert/update/delete/query operator auto use transaction
         BOOLEAN                 _transAutoCommit ;
         // when transaction operator failed, wether rollback auto
         BOOLEAN                 _transAutoRollback ;
         // whether to use RC isolation to process count()
         BOOLEAN                 _transRCCount ;

         UINT32                  _transConfMask ;
         UINT32                  _transConfVer ;

   } ;
   typedef _dpsTransConfItem dpsTransConfItem ;

   /*
      _dpsTransMBStat define
    */
   class _dpsTransMBStat : public SDBObject
   {
      protected :
         _dpsTransMBStat ()
         : _totalRecords(),
           _incDelta( 0 ),
           _decDelta( 0 )
         {
         }

      public :
         _dpsTransMBStat ( ossAtomic64 * totalRecords,
                           UINT64 incDelta,
                           UINT64 decDelta )
         : _totalRecords( totalRecords ),
           _incDelta( incDelta ),
           _decDelta( decDelta )
         {
         }

         _dpsTransMBStat ( const _dpsTransMBStat & stat )
         : _totalRecords( stat._totalRecords ),
           _incDelta( stat._incDelta ),
           _decDelta( stat._decDelta )
         {
            SDB_ASSERT( NULL != _totalRecords,
                        "atomic total records should not be NULL" ) ;
         }

         ~_dpsTransMBStat ()
         {
         }

      public :
         _dpsTransMBStat & operator = ( const _dpsTransMBStat & stat )
         {
            SDB_ASSERT( NULL != stat._totalRecords,
                        "atomic total records should not be NULL" ) ;
            _totalRecords = stat._totalRecords ;
            _incDelta = stat._incDelta ;
            _decDelta = stat._decDelta ;
            return ( *this ) ;
         }

      public :
         OSS_INLINE void increase ( UINT64 delta )
         {
            _incDelta += delta ;
         }

         OSS_INLINE void decrease ( UINT64 delta )
         {
            _decDelta += delta ;
         }

         OSS_INLINE UINT64 getTotalRecords () const
         {
            SDB_ASSERT( NULL != _totalRecords,
                        "atomic total records should not be NULL" ) ;
            if ( _incDelta > _decDelta )
            {
               return ( _incDelta - _decDelta ) + _totalRecords->fetch() ;
            }
            else if ( _incDelta < _decDelta )
            {
               return _totalRecords->fetch() - ( _decDelta - _incDelta ) ;
            }
            return _totalRecords->fetch() ;
         }

         OSS_INLINE void commit ()
         {
            SDB_ASSERT( NULL != _totalRecords,
                        "atomic total records should not be NULL" ) ;
            if ( _incDelta > _decDelta )
            {
               _totalRecords->add( _incDelta - _decDelta ) ;
            }
            else if ( _incDelta < _decDelta )
            {
               _totalRecords->sub( _decDelta - _incDelta ) ;
            }
         }

         OSS_INLINE void rollback ()
         {
            SDB_ASSERT( NULL != _totalRecords,
                        "atomic total records should not be NULL" ) ;
         }

      protected :
         ossAtomic64 * _totalRecords ;
         UINT64        _incDelta ;
         UINT64        _decDelta ;
   } ;

   typedef class _dpsTransMBStat dpsTransMBStat ;

   /*
      _dpsTransExecutor define
   */
   class _dpsTransExecutor : public _dpsTransConfItem
   {
      struct cmpCSCLLock
      {
         bool operator() ( const dpsTransLockId& lhs,
                           const dpsTransLockId& rhs ) const
         {
            if ( lhs.csID() < rhs.csID() )
            {
               return TRUE ;
            }
            else if ( lhs.csID() > rhs.csID() )
            {
               return FALSE ;
            }
            if ( lhs.clID() < rhs.clID() )
            {
               return TRUE ;
            }
            else if ( lhs.clID() > rhs.clID() )
            {
               return FALSE ;
            }
            return FALSE ;
         }
      };

      // Only CS and CL lock should be inserted in this map. If other locks
      // are to be inserted, the cmpCSCLLock compare function needs to be updated
      typedef ossPoolMap< dpsTransLockId,
                          dpsTransLRB*,
                          cmpCSCLLock >                  DPS_LOCKID_MAP ;
      typedef DPS_LOCKID_MAP::iterator                   DPS_LOCKID_MAP_IT ;
      typedef DPS_LOCKID_MAP::const_iterator             DPS_LOCKID_MAP_CIT ;

      typedef ossPoolMap<DPS_LSN_OFFSET,dmsRecordID>     MAP_LSN_2_RECORD ;
      typedef MAP_LSN_2_RECORD::iterator                 MAP_LSN_2_RECORD_IT ;
      typedef MAP_LSN_2_RECORD::const_iterator           MAP_LSN_2_RECORD_CIT ;

      typedef ossPoolMap< utilCLUniqueID, dpsTransMBStat >  TRANS_MB_STAT_MAP ;
      typedef TRANS_MB_STAT_MAP::iterator                   TRANS_MB_STAT_MAP_IT ;
      typedef TRANS_MB_STAT_MAP::const_iterator             TRANS_MB_STAT_MAP_CIT ;

      friend class _pmdEDUCB ;

      public:
         _dpsTransExecutor( monMonitorManager *monMgr ) ;
         virtual ~_dpsTransExecutor() ;

         void     clearAll() ;
         void     assertLocks() ;

      public:

         void                 setWaiterInfo( dpsTransLRB * lrb,
                                             DPS_TRANS_QUE_TYPE type,
                                             LOCKMGR_TYPE managerType  ) ;
         void                 clearWaiterInfo( LOCKMGR_TYPE managerType ) ;

         dpsTransLRB*         getWaiterLRB( LOCKMGR_TYPE managerType ) const ;
         DPS_TRANS_QUE_TYPE   getWaiterQueType( LOCKMGR_TYPE managerType ) const ;

         void                 setLastLRB( dpsTransLRB *lrb,
                                          LOCKMGR_TYPE managerType ) ;
         void                 clearLastLRB( LOCKMGR_TYPE managerType ) ;
         dpsTransLRB *        getLastLRB( LOCKMGR_TYPE managerType ) const ;

         BOOLEAN              addLock( const dpsTransLockId &lockID,
                                       dpsTransLRB * lrb,
                                       LOCKMGR_TYPE managerType ) ;
         BOOLEAN              findLock( const dpsTransLockId &lockID,
                                        dpsTransLRB * &lrb,
                                        LOCKMGR_TYPE managerType,
                                        BOOLEAN needLock = FALSE ) ;
         BOOLEAN              removeLock( const dpsTransLockId &lockID,
                                          LOCKMGR_TYPE managerType ) ;
         void                 clearLock( LOCKMGR_TYPE managerType ) ;

         /*
            lockID is invalid, means count all the cs/cl lock
            lockType is -1, means all the lock type
         */
         UINT32               countLock( const dpsTransLockId &lockID,
                                         UINT8 lockType = DPS_TRANSLOCK_IX,
                                         LOCKMGR_TYPE managerType = LOCKMGR_TRANS_LOCK,
                                         BOOLEAN needLock = FALSE ) ;

         void                 incLockCount( LOCKMGR_TYPE managerType ) ;
         void                 decLockCount( LOCKMGR_TYPE managerType ) ;
         void                 clearLockCount( LOCKMGR_TYPE managerType ) ;
         UINT32               getLockCount( LOCKMGR_TYPE managerType ) const ;

         BOOLEAN              hasLockWait() const { return _lockWaitStarted ; }
         void                 finishLockWait() ;
         ossTickDelta         getLockWaitTime() const { return _lockWaitTime ; }

         void                 acquireLRBAccessingLock(
                                               LOCKMGR_TYPE lockMgrType ) ;
         void                 releaseLRBAccessingLock(
                                               LOCKMGR_TYPE lockMgrType ) ;
         void                 setAccessingLRB( LOCKMGR_TYPE lockMgrType,
                                               dpsTransLRB *LRB ) ;
         dpsTransLRB *        getAccessingLRB( LOCKMGR_TYPE lockMgrType ) ;

         /*
            Transaction Related
         */
         void                 setUseTransLock( BOOLEAN use ) ;
         BOOLEAN              useTransLock() const ;

         /*
            LSN to record map functions
         */
         const MAP_LSN_2_RECORD*    getRecordMap() const ;
         void                       putRecord( DPS_LSN_OFFSET lsnOffset,
                                               const dmsRecordID &item ) ;
         void                       delRecord( DPS_LSN_OFFSET lsnOffset ) ;
         BOOLEAN                    getRecord( DPS_LSN_OFFSET lsnOffset,
                                               dmsRecordID &item,
                                               BOOLEAN withDel = FALSE ) ;
         void                       clearRecordMap() ;
         BOOLEAN                    isRecordMapEmpty() const ;
         UINT32                     getRecordMapSize() const ;

         // for transaction meta-block statistics
         void commitMBStats () ;
         void rollbackMBStats () ;
         void clearMBStats () ;

         OSS_INLINE BOOLEAN isMBStatsEmpty () const
         {
            return _transMBStatMap.empty() ;
         }

         BOOLEAN incMBTotalRecords ( utilCLUniqueID clUniqueID,
                                     ossAtomic64 * totalRecords,
                                     UINT64 delta ) ;
         BOOLEAN decMBTotalRecords ( utilCLUniqueID clUniqueID,
                                     ossAtomic64 * totalRecords,
                                     UINT64 delta ) ;
         BOOLEAN getMBTotalRecords ( utilCLUniqueID clUniqueID,
                                     UINT64 & totalRecords ) const ;

         // clear arbitration records
         void     clearArbit() ;

         // arbitrate current transaction against given write transaction
         // input:
         //    - writeTransID: transaction ID of write transaction
         //    - writeTransStatus: transaction status of write transaction
         // output:
         //    - visible: indicate if current transaction could see changes
         //               from write transaction
         // NOTE: only when write transaction is committed, current transaction
         //       could see the changes from write transaction
         // return:
         //    - SDB_OK: succeed to arbitrate
         //    - other errors: failed to arbitrate
         INT32    arbit( const DPS_TRANS_ID &writeTransID,
                         DPS_TRANS_STATUS writeTransStatus,
                         BOOLEAN &visible ) ;

         // find arbitration records for given write transaction
         // input:
         //    - writeTransID: transaction ID of write transaction
         // output:
         //    - visible: indicate if current transaction could see changes
         //               from write transaction
         // return:
         //    - TRUE: record exists ( had been arbitrated before )
         //    - FALSE: record does not exist
         BOOLEAN  findArbit( const DPS_TRANS_ID &writeTransID,
                             BOOLEAN &visible ) ;

         // save arbitration result for given write transaction
         // input:
         //    - writeTransID: transaction ID of write transaction
         //    - writeTransStatus: transaction status of write transaction
         //    - visible: indicate if current transaction could see changes
         //               from write transaction
         // return:
         //    - SDB_OK: succeed to arbitrate
         //    - other errors: failed to arbitrate
         INT32    saveArbit( const DPS_TRANS_ID &writeTransID,
                             DPS_TRANS_STATUS writeTransStatus,
                             BOOLEAN visible ) ;

         // get time error of transaction
         // NOTE: all transaction times ( begin, pre-commit and commit )
         //       will reuse the time error of transaction begin
         OSS_INLINE UINT32 getTimeError() const
         {
            return _beginTime.getTimeError() ;
         }

         // get begin time of transaction
         OSS_INLINE const stpLogicalTimeUS &getBeginTime() const
         {
            return _beginTime ;
         }

         // set begin time of transaction
         OSS_INLINE void setBeginTime( const stpLogicalTimeUS &beginTime )
         {
            _beginTime = beginTime ;
         }

         // get pre-commit time of transaction
         OSS_INLINE const stpLogicalTimeUS &getPreCommitTime() const
         {
            return _preCommitTime ;
         }

         // set pre-commit time of transaction
         OSS_INLINE void setPreCommitTime( const stpLogicalTimeUS &preCommitTime )
         {
            // no time error for pre-commit time, will reuse time error of
            // transaction begin time
            _preCommitTime = preCommitTime ;
            _preCommitTime.setTimeError( _beginTime.getTimeError() ) ;
         }

         // get commit time of transaction
         OSS_INLINE const stpLogicalTimeUS &getCommitTime() const
         {
            return _commitTime ;
         }

         // set commit time of transaction
         OSS_INLINE void setCommitTime( const stpLogicalTimeUS &commitTime )
         {
            // no time error for commit time, will reuse time error of
            // transaction begin time
            _commitTime = commitTime ;
            _commitTime.setTimeError( _beginTime.getTimeError() ) ;
         }

         // set expireTran cache
         OSS_INLINE void setExpireTranCache( DPS_TRANSID_SN expireTran )
         {
            _expireTranCache = expireTran ;
         }

         // get expireTran cache
         OSS_INLINE DPS_TRANSID_SN getExpireTranCache() const
         {
            return _expireTranCache ;
         }

         // check if given transaction passed cached expireTran
         OSS_INLINE BOOLEAN isVersionExpired( const DPS_TRANS_ID &transID ) const
         {
            BOOLEAN expired = FALSE ;
            DPS_TRANSID_SN transSN= transID.getGlobSN() ;
            if ( DPS_INVALID_TRANSID_SN != _expireTranCache )
            {
               expired = ( transSN < _expireTranCache ) ;
            }
            return expired ;
         }

         // check if transaction passed doing arbitration time
         // - before that time, current transaction needs arbitrate for all
         //   records created or updated by doing transactions
         // - after that time, the doing transactions could not be able to
         //   commit by that time, so the records created or updated by them
         //   won't be seen by this transaction
         OSS_INLINE BOOLEAN isPassedDoingArbit() const
         {
            return _passedDoingArbit ;
         }

         // set transaction passed doing arbitration time
         OSS_INLINE void setPassedDoingArbit( BOOLEAN passed )
         {
            _passedDoingArbit = passed ;
         }

         void regReadTranTime() ;

         // reset transaction times ( begin time, pre-commit time and
         // pass arbitration time flag )
         void resetTransTime() ;

      protected:
         void                 initTransConf( INT32 isolation,
                                             UINT32 timeout,
                                             BOOLEAN waitLock,
                                             BOOLEAN autoCommit,
                                             BOOLEAN autoRollback,
                                             BOOLEAN useRBS,
                                             BOOLEAN rcCount,
                                             BOOLEAN globTrans ) ;

         BOOLEAN              updateTransConf( INT32 isolation,
                                               UINT32 timeout,
                                               BOOLEAN waitLock,
                                               BOOLEAN autoCommit,
                                               BOOLEAN autoRollback,
                                               BOOLEAN useRBS,
                                               BOOLEAN rcCount,
                                               BOOLEAN globTrans ) ;

         void     addReservedSpace( const UINT64 len ) ;

         UINT64   getReservedSpace() const ;

         void     resetLogSpace() ;

         void _initMBStat ( utilCLUniqueID clUniqueID,
                            ossAtomic64 * totalRecords,
                            UINT64 incDelta,
                            UINT64 decDelta ) ;

      public:
         /*
            Interface
         */
         virtual EDUID        getEDUID() const = 0 ;
         virtual UINT32       getTID() const = 0 ;
         virtual void         wakeup() = 0 ;
         virtual INT32        wait( INT64 timeout ) = 0 ;
         virtual IExecutor*   getExecutor() = 0 ;
         virtual BOOLEAN      isInterrupted () = 0 ;

      protected:
         dpsTransLRB *           _waiter[ LOCKMGR_TYPE_MAX ] ;
         DPS_TRANS_QUE_TYPE      _waiterQueType[ LOCKMGR_TYPE_MAX ] ;
         dpsTransLRB *           _lastLRB[ LOCKMGR_TYPE_MAX ] ;

         ossSpinXLatch           _mapMutex ;
         DPS_LOCKID_MAP          _mapCSCLLockID[ LOCKMGR_TYPE_MAX ] ;
         UINT32                  _lockCount[ LOCKMGR_TYPE_MAX ] ;

         ossSpinSLatch           _accessingLRBMutex ;
         // LOCKMGR_TRANS_LOCK
         dpsTransLRB *           _accessingTransLRB[ LOCKMGR_TYPE_MAX ] ;

         /*
            LSN to record info
         */
         MAP_LSN_2_RECORD        _mapLSN2Record ;

         // record counts of collection during transaction
         TRANS_MB_STAT_MAP       _transMBStatMap ;

         // records for global transaction arbitration
         dpsTransArbit           _transArbit ;

         // logical time of transaction begin
         stpLogicalTimeUS        _beginTime ;
         // logical time of transaction pre-commit
         stpLogicalTimeUS        _preCommitTime ;
         // logical time of transaction commit
         stpLogicalTimeUS        _commitTime ;

         // to avoid use atomic value or locks when doing visibility checks
         // agains global expireTran, we cache expireTran in local transaction,
         // so visibility checks could use this cache without any locks
         // NOTE: this will be updated at the beginning of each read operators
         //       in transaction
         DPS_TRANSID_SN          _expireTranCache ;

         // indicate if transaction has passed doing arbitration time
         // - before that time, current transaction needs arbitrate for all
         //   records created or updated by doing transactions
         // - after that time, the doing transactions could not be able to
         //   commit by that time, so the records created or updated by them
         //   won't be seen by this transaction
         BOOLEAN                 _passedDoingArbit ;
         // indicate if transaction registered for read transaction
         BOOLEAN                 _regReadTranTime ;

      private:
         BOOLEAN                 _useTransLock ;
         // undo LR space reserved by this transaction
         UINT64                  _reservedLogSpace ;

         monMonitorManager      *_monMgr ;
         monClassLock           *_monLock ;
         BOOLEAN                 _lockWaitStarted ;
         ossTick                 _lockWaitStartTimer ;
         ossTickDelta            _lockWaitTime ;
   } ;
   typedef _dpsTransExecutor dpsTransExecutor ;

}

#endif // DPS_TRANS_EXECUTOR_HPP__

