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
         BOOLEAN              isTransAllowLockEscalation() const ;
         INT32                getTransMaxLockNum() const ;
         INT32                getTransMaxLogSpaceRatio() const ;

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
         void                 setTransAllowLockEscalation( BOOLEAN allow,
                                                           BOOLEAN enableMask = TRUE ) ;
         void                 setTransMaxLockNum( INT32 maxNum,
                                                  BOOLEAN enableMask = TRUE ) ;
         void                 setTransMaxLogSpaceRatio( INT32 maxRatio,
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

         // whether allow lock escalation when exceeds limit of max record
         // locks
         BOOLEAN                 _transAllowLockEscalation ;

         // Maximum number of record locks can be hold by a transaction
         INT32                   _transMaxLockNum ;

         // Maximum ratio of log space can be used by a transaction
         INT32                   _transMaxLogSpaceRatio ;

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
         : _globTransAvailTime( NULL ),
           _maxTransCommitTime( NULL ),
           _totalRecords( NULL ),
           _incDelta( 0 ),
           _decDelta( 0 )
         {
         }

      public :
         _dpsTransMBStat ( ossAtomic64 * globTransAvailTime,
                           ossAtomic64 * maxTransCommitTime,
                           ossAtomic64 * totalRecords,
                           UINT64 incDelta,
                           UINT64 decDelta )
         : _globTransAvailTime( globTransAvailTime ),
           _maxTransCommitTime( maxTransCommitTime ),
           _totalRecords( totalRecords ),
           _incDelta( incDelta ),
           _decDelta( decDelta )
         {
         }

         _dpsTransMBStat ( const _dpsTransMBStat & stat )
         : _globTransAvailTime( stat._globTransAvailTime ),
           _maxTransCommitTime( stat._maxTransCommitTime ),
           _totalRecords( stat._totalRecords ),
           _incDelta( stat._incDelta ),
           _decDelta( stat._decDelta )
         {
         }

         ~_dpsTransMBStat ()
         {
         }

      public :
         _dpsTransMBStat & operator = ( const _dpsTransMBStat & stat )
         {
            _globTransAvailTime = stat._globTransAvailTime ;
            _maxTransCommitTime = stat._maxTransCommitTime ;
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

         OSS_INLINE BOOLEAN getTotalRecords ( UINT64 &totalRecords ) const
         {
            if ( NULL != _totalRecords )
            {
               if ( _incDelta > _decDelta )
               {
                  totalRecords = ( _incDelta - _decDelta ) +
                                 _totalRecords->fetch() ;
               }
               else if ( _incDelta < _decDelta )
               {
                  totalRecords = _totalRecords->fetch() -
                                 ( _decDelta - _incDelta ) ;
               }
               else
               {
                  totalRecords = _totalRecords->fetch() ;
               }

               return TRUE ;
            }

            return FALSE ;
         }

         OSS_INLINE void commit ( UINT64 commitTime )
         {
            if ( NULL != _totalRecords )
            {
               if ( _incDelta > _decDelta )
               {
                  _totalRecords->add( _incDelta - _decDelta ) ;
               }
               else if ( _incDelta < _decDelta )
               {
                  _totalRecords->sub( _decDelta - _incDelta ) ;
               }
            }
            // current transaction is going to commit and release the locks on
            // collections, so update global transaction available timestamp
            // with commit timestamp of current transaction to block
            // other transactions may fetch MVCC versions before
            if ( NULL != _globTransAvailTime )
            {
               _globTransAvailTime->swapGreaterThan( commitTime ) ;
            }
            if ( NULL != _maxTransCommitTime )
            {
               _maxTransCommitTime->swapGreaterThan( commitTime ) ;
            }
         }

         OSS_INLINE void rollback ( UINT64 rollbackTime )
         {
            // current transaction is going to commit and release the locks on
            // collections, so update global transaction available timestamp
            // with commit timestamp of current transaction to block
            // other transactions may fetch MVCC versions before
            if ( NULL != _globTransAvailTime )
            {
               // if collection has max transaciton commit time, use it as
               // rollback time, before that, MVCC versions has lost
               if ( NULL != _maxTransCommitTime )
               {
                  UINT64 tmp = _maxTransCommitTime->fetch() ;
                  if ( tmp < rollbackTime )
                  {
                     rollbackTime = tmp ;
                  }
               }
               _globTransAvailTime->swapGreaterThan( rollbackTime ) ;
            }
         }

         OSS_INLINE void setTotalRecords( ossAtomic64 *totalRecords )
         {
            _totalRecords = totalRecords ;
         }

         OSS_INLINE BOOLEAN hasTotalRecords() const
         {
            return NULL != _totalRecords ? TRUE : FALSE ;
         }

         OSS_INLINE void setGlobTransAvailTime( ossAtomic64 *globTransAvailTime )
         {
            _globTransAvailTime = globTransAvailTime ;
         }

         OSS_INLINE BOOLEAN hasGlobTransAvailTime() const
         {
            return NULL != _globTransAvailTime ? TRUE : FALSE ;
         }

      protected :
         ossAtomic64 * _globTransAvailTime ;
         ossAtomic64 * _maxTransCommitTime ;
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

         void                 incLockCount( LOCKMGR_TYPE managerType,
                                            BOOLEAN isLeafLevel ) ;
         void                 decLockCount( LOCKMGR_TYPE managerType,
                                            BOOLEAN isLeafLevel ) ;
         void                 clearLockCount( LOCKMGR_TYPE managerType ) ;
         UINT32               getLockCount( LOCKMGR_TYPE managerType ) const ;
         UINT32               getLeafLockCount( LOCKMGR_TYPE managerType ) const ;

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

         // get the waiting LRB and lockId if this executor is waiting for a
         // trans lock and it has opened a transaction and has associated with
         // _tmsDataTransContext
         BOOLEAN getTransWaitingLRBInfo( dpsTxWaitLRB & waitInfo,
                                         LOCKMGR_TYPE
                                         lockMgrType = LOCKMGR_TRANS_LOCK ) ;
         DPS_TRANS_ID getNormalizedTransID() ;

         // for transaction meta-block statistics
         void commitMBStats ( UINT64 commitTime ) ;
         void rollbackMBStats ( UINT64 rollbackTime ) ;
         void clearMBStats () ;

         OSS_INLINE BOOLEAN isMBStatsEmpty () const
         {
            return _transMBStatMap.empty() ;
         }

         BOOLEAN incMBTotalRecords ( utilCLUniqueID clUniqueID,
                                     ossAtomic64 * globTransAvailTime,
                                     ossAtomic64 * maxTransCommitTime,
                                     ossAtomic64 * totalRecords,
                                     UINT64 delta ) ;
         BOOLEAN decMBTotalRecords ( utilCLUniqueID clUniqueID,
                                     ossAtomic64 * globTransAvailTime,
                                     ossAtomic64 * maxTransCommitTime,
                                     ossAtomic64 * totalRecords,
                                     UINT64 delta ) ;
         BOOLEAN updateMBStat( utilCLUniqueID clUniqueID,
                               ossAtomic64 * globTransAvailTime,
                               ossAtomic64 * maxTransCommitTime,
                               ossAtomic64 * totalRecords ) ;
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

         OSS_INLINE BOOLEAN hasRegReadTran() const
         {
            return _regReadTranTime ;
         }

         OSS_INLINE void setRegReadTran( BOOLEAN hasReg )
         {
            _regReadTranTime = hasReg ;
         }

         // reset transaction times ( begin time, pre-commit time and
         // pass arbitration time flag )
         void resetTransTime() ;

         UINT64   getReservedSpace() const ;
         UINT64   getUsedSpace() const ;
         UINT64   getLogSpace() const ;

         INT32                checkLockEscalation( LOCKMGR_TYPE managerType,
                                                   const dpsTransLockId &lockID,
                                                   BOOLEAN &needEscalation ) ;

         OSS_INLINE void      setLockEscalated( LOCKMGR_TYPE managerType,
                                                BOOLEAN isEscalated )
         {
            _isLockEscalated[ managerType ] = isEscalated ;
         }

         OSS_INLINE BOOLEAN   isLockEscalated( LOCKMGR_TYPE managerType ) const
         {
            return _isLockEscalated[ managerType ] ;
         }

         OSS_INLINE void      resetLockEscalated( LOCKMGR_TYPE managerType )
         {
            _isLockEscalated[ managerType ] = FALSE ;
         }

         // interface to get transaction ID
         OSS_INLINE DPS_TRANS_ID getTransID()
         {
            return getExecutor()->getTransID() ;
         }

         OSS_INLINE DPS_TRANS_ID getOrigTransID()
         {
            return getTransID().getOrigTransID() ;
         }

      protected:
         void                 initTransConf( INT32 isolation,
                                             UINT32 timeout,
                                             BOOLEAN waitLock,
                                             BOOLEAN autoCommit,
                                             BOOLEAN autoRollback,
                                             BOOLEAN useRBS,
                                             BOOLEAN rcCount,
                                             BOOLEAN allowLockEscalation,
                                             INT32 maxLockNum,
                                             INT32 maxLogSpaceRatio,
                                             UINT64 totalLogSpace ) ;

         BOOLEAN              updateTransConf( INT32 isolation,
                                               UINT32 timeout,
                                               BOOLEAN waitLock,
                                               BOOLEAN autoCommit,
                                               BOOLEAN autoRollback,
                                               BOOLEAN useRBS,
                                               BOOLEAN rcCount,
                                               BOOLEAN allowLockEscalation,
                                               INT32 maxLockNum,
                                               INT32 maxLogSpaceRatio,
                                               UINT64 totalLogSpace ) ;

         void                 copyTransConf( const dpsTransConfItem &conf,
                                             UINT64 totalLogSpace ) ;
         void                 updateTransConfByMask( const dpsTransConfItem &conf,
                                                     UINT64 totalLogSpace ) ;

         void     addReservedSpace( const UINT64 len ) ;
         void     decReservedSpace( const UINT64 len ) ;
         void     addUsedSpace( const UINT64 len ) ;

         void     resetLogSpace() ;

         INT32    checkLogSpace( UINT64 usedLen, UINT64 reservedLen ) const ;
         void     updateMaxLogSpace( UINT64 totalLogSpace ) ;

         void _initMBStat ( utilCLUniqueID clUniqueID,
                            ossAtomic64 * globTransAvailTime,
                            ossAtomic64 * maxTransCommitTime,
                            ossAtomic64 * totalRecords,
                            UINT64 incDelta,
                            UINT64 decDelta ) ;

      public:
         /*
            Interface
         */
         virtual EDUID        getEDUID() const = 0 ;
         virtual UINT32       getTID() const = 0 ;
         virtual void         wakeup( INT32 wakeupRC ) = 0 ;
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
         UINT32                  _leafLockCount[ LOCKMGR_TYPE_MAX ] ;
         BOOLEAN                 _isLockEscalated[ LOCKMGR_TYPE_MAX ] ;

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
         UINT64                  _usedLogSpace ;
         UINT64                  _maxLogSpace ;

         monMonitorManager      *_monMgr ;
         monClassLock           *_monLock ;
         BOOLEAN                 _lockWaitStarted ;
         ossTick                 _lockWaitStartTimer ;
         ossTickDelta            _lockWaitTime ;
   } ;
   typedef _dpsTransExecutor dpsTransExecutor ;

}

#endif // DPS_TRANS_EXECUTOR_HPP__

