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

   Source File Name = dpsTransCB.hpp

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
#ifndef DPSTRANSCB_HPP_
#define DPSTRANSCB_HPP_

#include <queue>
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "dpsDef.hpp"
#include "monLatch.hpp"
#include "dms.hpp"
#include "dpsTransLockMgr.hpp"
#include "dpsTransArbit.hpp"
#include "dpsLogRecord.hpp"
#include "sdbInterface.hpp"
#include "ossEvent.hpp"
#include "ossMemPool.hpp"
#include "monLatch.hpp"
#include "stpAgent.hpp"
#include "dpsUtil.hpp"
#include "utilConcurrentMap.hpp"
#include "../bson/bson.hpp"
#include "dpsWriteContext.hpp"

using namespace bson ;

namespace engine
{
   #define DPS_TRANS_BUCKET_SIZE ( 64 )

   class _pmdEDUCB ;
   class _dmsExtScanner ;
   class _dmsIXSecScanner ;
   class oldVersionCB ;
   class dpsTransLockManager ;
   class _dpsITransLockCallback ;
   class _dpsLogWrapper ;
   class _dpsGTSAgent ;

   /*
      _dpsTransPendingKey define
   */
   struct _dpsTransPendingKey
   {
      ossPoolString  _collection ;
      BSONObj        _obj ;

      _dpsTransPendingKey()
      {
      }

      _dpsTransPendingKey( const CHAR *collection,
                           const BSONObj &obj,
                           BOOLEAN getOwned )
      {
         setKey( collection, obj, getOwned ) ;
      }

      bool operator <( const _dpsTransPendingKey &rhs ) const
      {
         /// compare id
         BSONElement l, r ;
         INT32 res = _collection.compare( rhs._collection ) ;
         if ( res < 0 )
         {
            return true ;
         }
         else if ( res > 0 )
         {
            return false ;
         }
         l = _obj.getField( DMS_ID_KEY_NAME ) ;
         r = rhs._obj.getField( DMS_ID_KEY_NAME ) ;
         return l.woCompare( r, FALSE ) < 0 ;
      }

      bool operator ==( const _dpsTransPendingKey &rhs ) const
      {
         if ( _collection == rhs._collection )
         {
            /// compare id
            BSONElement l, r ;
            l = _obj.getField( DMS_ID_KEY_NAME ) ;
            r = rhs._obj.getField( DMS_ID_KEY_NAME ) ;
            return l.woCompare( r, FALSE ) == 0 ;
         }
         return false ;
      }

      void setKey( const CHAR *collection,
                   const BSONObj &obj,
                   BOOLEAN getOwned )
      {
         _collection.assign( collection ) ;
         if ( getOwned )
         {
            _obj = obj.getOwned() ;
         }
         else
         {
            _obj = obj ;
         }
      }
   } ;
   typedef struct _dpsTransPendingKey dpsTransPendingKey ;

   /*
      _dpsTransPendingValue define
   */
   struct _dpsTransPendingValue
   {
      BSONObj     _obj ;
      INT32       _opType ;

      _dpsTransPendingValue()
      : _opType( LOG_TYPE_DUMMY )
      {
      }

      _dpsTransPendingValue( const BSONObj &obj, INT32 opType )
      {
         setValue( obj, opType ) ;
      }

      void setValue( const BSONObj &obj, INT32 opType )
      {
         _obj = obj.getOwned() ;
         _opType = opType ;
      }
   } ;
   typedef struct _dpsTransPendingValue dpsTransPendingValue ;

   // NOTE:
   // 1. pending key is expected DMS record during rollback
   //    it should contains the whole DMS record
   // 2. pending value is the actual DMS record on disk ( must have OID )
   //    during rollback
   typedef ossPoolMap<dpsTransPendingKey,
                      dpsTransPendingValue> MAP_TRANS_PENDING_OBJ ;

   // helper functions for transaction rollback pending objects
   // add pending key and value into pending map
   INT32 dpsAddTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                             dpsTransPendingKey &pendingKey,
                             dpsTransPendingValue &pendingValue,
                             BOOLEAN &added ) ;
   // remove pending key and value from pending map
   // and get back the removed key and value if needed
   INT32 dpsRemoveTransPending( MAP_TRANS_PENDING_OBJ &pendingMap,
                                const dpsTransPendingKey &pendingKey,
                                BSONObj *oldKey,
                                dpsTransPendingValue *oldValue,
                                BOOLEAN &removed ) ;

   /*
      _dpsTransBackInfo define
   */
   struct _dpsTransBackInfo
   {
      // the last LSN of transaction
      // NOTE: in rollback status, it is the last rollbacked transaction LSN
      DPS_LSN_OFFSET                _lsn ;
      INT32                         _status ;
      // current pending LSN: the last LSN still rollback pending
      DPS_LSN_OFFSET                _curLSNWithRBPending ;
      // current non pending LSN: the LSN which did not create pending object
      // during rollback pending
      ossPoolSet< DPS_LSN_OFFSET >  _curNonPendingLSN ;

      // logical time of transaction begin
      stpLogicalTimeUS              _beginTime ;
      // logical time of transaction pre-commit
      stpLogicalTimeUS              _preCommitTime ;
      // logical time of transaction commit
      stpLogicalTimeUS              _commitTime ;

      _dpsTransBackInfo( DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET,
                         INT32 status = DPS_TRANS_DOING )
      {
         _lsn = lsn ;
         _curLSNWithRBPending = DPS_INVALID_LSN_OFFSET ;
         _status = status ;
      }
   } ;
   typedef _dpsTransBackInfo dpsTransBackInfo ;

   typedef ossPoolMap<DPS_TRANS_ID, dpsTransBackInfo> TRANS_DUMP_MAP ;
   typedef utilConcurrentMap< DPS_TRANS_ID,
                              dpsTransBackInfo,
                              DPS_TRANS_BUCKET_SIZE,
                              dpsTransIDHash,
                              monSpinSLatch >         TRANS_MAP ;
   typedef utilConcurrentMap< DPS_TRANS_ID,
                              _pmdEDUCB *,
                              DPS_TRANS_BUCKET_SIZE,
                              dpsTransIDHash,
                              monSpinSLatch >         TRANS_CB_MAP ;
   typedef ossPoolMap<DPS_LSN_OFFSET, DPS_TRANS_ID>   TRANS_LSN_ID_MAP ;
   typedef ossPoolMap<DPS_TRANS_ID, DPS_LSN_OFFSET>   TRANS_ID_LSN_MAP ;
   typedef std::queue< EDUID >                        TRANS_EDU_LIST ;
   typedef ossPoolList< DPS_TRANS_ID >                TRANS_ID_LIST ;

   /*
      _dpsHisTransStatus define
   */
   struct _dpsHisTransStatus
   {
      // status of transaction
      INT32             _status ;
      // the last LSN of transaction
      // NOTE: in rollback status, it is the first LSN of transaction
      DPS_LSN_OFFSET    _lsn ;
      // logical time of transaction begin
      stpLogicalTimeUS  _beginTime ;
      // logical time of transaction pre-commit
      stpLogicalTimeUS  _preCommitTime ;
      // logical time of transaction commit
      stpLogicalTimeUS  _commitTime ;

      _dpsHisTransStatus()
      : _status( DPS_TRANS_COMMIT ),
        _lsn( DPS_INVALID_LSN_OFFSET ),
        _beginTime(),
        _preCommitTime(),
        _commitTime()
      {
      }

      _dpsHisTransStatus( INT32 status,
                          DPS_LSN_OFFSET lsn,
                          const stpLogicalTimeUS &beginTime,
                          const stpLogicalTimeUS &preCommitTime )
      : _status( status ),
        _lsn( lsn ),
        _beginTime( beginTime ),
        _preCommitTime( preCommitTime )
      {
      }
   } ;
   typedef _dpsHisTransStatus dpsHisTransStatus ;

   typedef utilConcurrentMap< DPS_TRANS_ID,
                              dpsHisTransStatus,
                              DPS_TRANS_BUCKET_SIZE,
                              dpsTransIDHash,
                              monSpinSLatch >      TRANS_HIST_MAP ;

   // delta between runtime log and undo log due to TransRelatedLSN
   // sizeof( _dpsRecordEle ) + sizeof( UINT64 ) = 13
   // and align to 4 bytes, finally, we need at least 16 bytes
   #define DPS_TRANS_LOG_UNDO_DELTA  ( 16 )

   /*
      dpsTransCB define
   */
   class dpsTransCB : public _IControlBlock, public _IEventHander
   {
      friend class _dmsExtScannerBase ;
      friend class _dmsExtScanner ;
      friend class _dmsIXSecScanner ;
   public:
      dpsTransCB() ;
      virtual ~dpsTransCB() ;

      virtual SDB_CB_TYPE cbType() const ;
      virtual const CHAR* cbName() const ;

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

      virtual void   onRegistered( const MsgRouteID &nodeID ) ;
      virtual void   onPrimaryChange( BOOLEAN primary,
                                      SDB_EVENT_OCCUR_TYPE occurType ) ;

      void           setEventHandler( dpsTransEvent *pEventHandler ) ;
      dpsTransEvent* getEventHandler() ;

      // allocate new transaction ID
      // input:
      //    - isAutoCommit: indicate a auto-commit transaction
      //    - isGlobTrans: indicate a global transaction
      //    - timeout: timeout ( in millisecond ) to get a global
      //               transaction ID
      // output:
      //    - transID: transaction ID allocated
      //    - beginTime: logical time to begin transaction
      // return:
      //    - SDB_OK: succeed
      //    - SDB_GLOB_TRANS_NOT_AVAILABLE: global transaction is not enabled
      //    - STP_NOT_AVAILABLE: STP is not available for global transaction
      // NOTE: TransactionID:
      //       +-----------+-----------+---------------+
      //       | TAG(8bit) | SN(56bit) | nodeID(16bit) |
      //       +-----------+-----------+---------------+
      INT32 allocTransID( BOOLEAN isAutoCommit,
                          BOOLEAN isGlobTrans,
                          UINT32 timeout,
                          DPS_TRANS_ID &transID,
                          stpLogicalTimeUS &beginTime ) ;

      // check visibility of given record against given transaction ID
      // input:
      //    - recTransID: transaction ID for given record
      //                  indicates which transaction created or modified the
      //                  record
      //    - transID: current transaction ID of the transaction to visit
      //               the record
      //    - transBeginTime: logical time to begin current transaction
      //    - transIsolation: isolation level to check visibility
      //                      currently only RR is supported
      //    - strictIsolation: whether to check isolations between
      //                       non-transaction, non-global transaction and
      //                       global transaction
      // output:
      //    - visible: indicate if current transaction could visit given record
      //    - pVisibleTime: indicate the visible time of this record
      // return:
      //    - SDB_OK: succeed to check version visible
      //    - other errors: failed to check version visible
      // NOTE:
      //    - strictIsolation is used to check isolations between
      //      non-transaction, non-global transaction and global transaction
      //    - if strictIsolation is FALSE, it means lower level of
      //      transaction could change results of higher level of transaction
      //      e.g. transaction queries could see changes from non-transaction
      //      operators
      //    - if strictIsolation is TRUE, it means lower level of
      //      transaction could not change results of higher level of
      //      transaction, if a higher level transaction sees a record with
      //      lower transaction level, it should report error
      INT32 isVersionVisible( _pmdEDUCB *eduCB,
                              const DPS_TRANS_ID &recTransID,
                              const DPS_TRANS_ID &transID,
                              const stpLogicalTimeUS &transBeginTime,
                              INT32 isolation,
                              BOOLEAN strictIsolation,
                              BOOLEAN &visible,
                              stpLogicalTimeUS *pVisibleTime = NULL ) ;

      // check expired of given transaction ID against lowTran
      // input:
      //    - transID: transaction ID to check
      // return:
      //    - TRUE: transaction is expired ( could be cleared )
      //    - FALSE: transaction is not expired ( could not be cleared )
      BOOLEAN isVersionExpired( const DPS_TRANS_ID &transID ) ;

      // get transaction ID of the minimum running transaction ID ( lowTran )
      // of whole cluster
      // return:
      //    - transaction ID of lowTran of whole cluster
      // NOTE: this call gets global lowTran in cache ( won't update with
      //       CATALOG
      DPS_TRANS_ID getGlobLowTran() ;

      // get transaction ID of the maximum expired transaction ID
      // ( expireTran ) of whole cluster
      // return:
      //    - transaction ID of expireTran of whole cluster
      // NOTE: this call gets global expireTran in cache ( won't update with
      //       CATALOG
      DPS_TRANS_ID getGlobExpireTran() ;

      // update global lowTran to CATALOG, and get back latest global lowTran
      // input:
      //    - timeout: timeout to get response from CATALOG
      // output:
      //    - globLowTran: global lowTran from CATALOG
      // return:
      //    - SDB_OK: succeed to get global lowTran
      //    - SDB_TIMEOUT: failed to get global lowTran in timeout
      // NOTE: if not update cache, it will be a slightly older lowTran
      //       if update cache, it will query from remote to get the earliest
      //       lowTran among all nodes in the cluster
      INT32 syncUpdateGlobLowTran( DPS_TRANS_ID &globLowTran,
                                   INT64 timeout = -1 ) ;

      // set transaction ID of the global minimum running transaction ID
      // ( global lowTran )
      // input :
      //    - globLowTran: global low transaction ID
      // NOTE:
      // - the lowTran will be updated in monotonic
      // - it is atomic, so no need to acquire locks
      void setGlobLowTran( const DPS_TRANSID_SN &globLowTran ) ;

      // set transaction ID of the global maximum expired transaction ID
      // ( global expireTran )
      // input :
      //    - globExpireTran: global expired transaction ID
      // NOTE:
      //    - the expireTran will be updated in monotonic
      //    - it is atomic, so no need to acquire locks
      void setGlobExpireTran( const DPS_TRANSID_SN &globExpireTran ) ;

      // get transaction ID of the minimum running global transaction ID
      // ( lowTran ) in this node
      // return:
      //    - transaction ID of lowTran of this node
      DPS_TRANS_ID getLocalLowTran() ;

      // get transaction ID of the maximum expired global transaction ID
      // ( lowTran ) in this node
      // return:
      //    - transaction ID of expireTran of this node
      DPS_TRANS_ID getLocalExpireTran() ;

      // get maximum expired transaction
      // return:
      //    - expireTran which is safely expired
      //    - DPS_INVALID_TRANSID_SN for invalid value
      // NOTE: expired transaction SN is calculated from global expireTran
      //       minus a max time error ( which is a safe value for clear
      //       expired transaction objects, old version, etc )
      DPS_TRANSID_SN getExpiredVersion() ;

      // get logical time from STP
      // input:
      //    - timeout: timeout to get logical time
      //               0 means try once, -1 means never timeout
      // output:
      //    - time: time from STP
      //    - pWaitedTime: return total wait time
      // return:
      //    - SDB_OK: succeed to get time
      //    - STP_NOT_AVAILABLE: STP is not available for global transaction
      //    - SDB_TIMEOUT: failed to get time in given timeout
      INT32 getGlobTransTime( stpLogicalTimeUS &time,
                              INT32 timeout = OSS_ONE_SEC,
                              INT32 *pWaitedTime = NULL ) ;

      // get logical time from STP which should after expecting time
      // input:
      //    - eduCB: EDU CB of current transaction
      //    - expectTimeUS: expecting time in microseconds
      //    - timeout: timeout to get logical time
      //               0 means try once, -1 means never timeout
      // output:
      //    - time: time from STP
      // return:
      //    - SDB_OK: succeed to get time
      //    - STP_NOT_AVAILABLE: STP is not available for global transaction
      //    - SDB_TIMEOUT: failed to get time in given timeout
      INT32 getGlobTransTime( _pmdEDUCB *eduCB,
                              UINT64 expectTimeUS,
                              stpLogicalTimeUS &time,
                              INT32 timeout = OSS_ONE_SEC ) ;

      // get logical time from STP for pre-commit of transaction
      // input:
      //    - eduCB: EDUCB of transaction
      // output:
      //    - preCommitTime: logical time to pre-commit transaction
      // return:
      //    - SDB_OK: succeed to get time
      //    - STP_NOT_AVAILABLE: STP is not available for global transaction
      //    - SDB_TIMEOUT: failed to get time in given timeout
      // NOTE: for global transaction, it should be pre-commit in
      //       a time error period later after transaction begin
      INT32 getGlobPreCommitTime( _pmdEDUCB *eduCB,
                                  stpLogicalTimeUS &preCommitTime ) ;

      // get logical time to pre-commit transaction by searching the maximum
      // running global transaction ID in this node
      // input:
      //    - localTime: local time to test with the maximum running
      //                 global transaction ID
      // output:
      //    - preCommitTime: global logical time to pre-commit transaction
      // return:
      //    - SDB_OK: get pre-commit time succeed
      //    - other errors: failed to get pre-commit time
      // NOTE:
      //    - consider with time error
      //    - pre-commit time on current node should be larger than
      //      maximum running global transaction ID ( to resolve conflicts )
      //    - pre-commit time might be delayed by maximum running global
      //      transaction in this node
      INT32 getLocalPreCommitTime( const stpLogicalTimeUS &localTime,
                                   stpLogicalTimeUS &preCommitTime ) ;

      // get logical time from STP for commit transaction
      // input:
      //    - eduCB: EDUCB of transaction
      // output:
      //    - commitTime: logical time to commit transaction
      // NOTE: for global transaction, it should be commit after
      //       pre-commit time
      void getGlobCommitTime( _pmdEDUCB *eduCB,
                              stpLogicalTimeUS &commitTime ) ;

      // get transaction info
      // input:
      //    - transID: transaction ID to search
      // output:
      //    - info: transaction info of given transaction
      // return:
      //    - TRUE: found given transaction
      //    - FALSE: failed to find given transaction
      // NOTE: from either running transaction info or history transaction info
      BOOLEAN getTransInfo( const DPS_TRANS_ID &transID,
                            dpsTransBackInfo &info ) ;

      // check if global transaction is valid to start
      // NOTE: transaction should be started after this node becomes primary
      // input:
      //    - transID: transaction ID to be checked
      //    - beginTime: begin logical time of transaction
      // return :
      //    - SDB_OK: global transaction is OK to start
      //    - SDB_GLOB_TRANS_NOT_AVAILABLE: global transaction is not available
      INT32 checkGlobTrans( const DPS_TRANS_ID &transID,
                            const stpLogicalTimeUS &beginTime ) ;

      // do arbitrate for global transactions
      // input:
      //    - eduCB: EDUCB of current reading transaction
      //    - readTransID: transaction ID of current reading transaction
      //    - writeTransID: transaction ID of write transaction
      //    - writeTransStatus: transaction status of write transaction
      //    - forceLocal: force do arbitration on local
      // output:
      //    - visible: indicate whether read transaction could see changes
      //               from write transaction
      // return:
      //    - SDB_OK: succeed to do arbitration for global transactions
      //    - other errors: failed to do arbitration for global transactions
      // NOTE:
      //    - may send arbitrate request to remote node
      //    - only when write transaction is involved in a single DATA
      //      group, we could use the force local mode
      INT32 doArbitGlobTrans( _pmdEDUCB *eduCB,
                              const DPS_TRANS_ID &readTransID,
                              const DPS_TRANS_ID &writeTransID,
                              DPS_TRANS_STATUS writeTransStatus,
                              BOOLEAN forceLocal,
                              BOOLEAN &visible ) ;

      // handle arbitrate request, and do the arbitration
      // input:
      //    - readTransID: transaction ID of current reading transaction
      //    - writeTransID: transaction ID of write transaction
      //    - writeTransStatus: transaction status of write transaction
      // output:
      //    - visible: indicate whether read transaction could see changes
      //               from write transaction
      // return:
      //    - SDB_OK: succeed to do arbitration for global transactions
      //    - other errors: failed to do arbitration for global transactions
      // NOTE: on when write transaction is committed, the visible could be
      //       TRUE
      INT32 onArbitGlobTrans( const DPS_TRANS_ID &readTransID,
                              const DPS_TRANS_ID &writeTransID,
                              DPS_TRANS_STATUS writeTransStatus,
                              BOOLEAN &visible ) ;

      DPS_TRANS_ID getRollbackID( const DPS_TRANS_ID &transID ) ;
      DPS_TRANS_ID getTransID( const DPS_TRANS_ID &rollbackID ) ;

      // get primary active time
      OSS_INLINE UINT64 getPrimaryActiveTime()
      {
         return _primaryActiveTime.fetch() ;
      }

      // reset primary active time
      OSS_INLINE void resetPrimaryActiveTime()
      {
         _primaryActiveTime.swap( DPS_INVALID_TRANS_TIME ) ;
      }

      // check if primary active time is valid
      OSS_INLINE BOOLEAN isPrimaryActived()
      {
         return !( _primaryActiveTime.compare( DPS_INVALID_TRANS_TIME ) ) ;
      }

      // set primary active time
      void setPrimaryActiveTime() ;

      // check primary active time
      // it not set, try to set primary active time
      void checkPrimaryActiveTime() ;

      // register read transaction
      OSS_INLINE void regReadTranTime( UINT64 readTime )
      {
         // maxReadTran is the transaction ID of the latest started read
         // transaction with upper bound of time error
         // maxReadTran is used to check pre-commit time of write transactions
         // which should be delayed by running read transactions ( which is
         // indicates by maxReadTran )
         _maxReadTran.swapGreaterThan( readTime ) ;
      }

      // register read transaction
      void regReadTran( _pmdEDUCB *eduCB ) ;

      // get restore window for point-in-time restore
      // output :
      // - minTime: minimum global logical time ( in microseconds ) to restore
      // - maxTransCommitTime: maximum global logical time of any commit records
      // - restorePointTime: logical time of restore point (either backup time
      //   of image used in the most recent sdbrestore or the time of the latest
      //   restorePrepare)
      // return :
      // - SDB_OK: succeed
      // NOTE:
      // - currently, the restore window is
      //   [ minRecoverableTime, min(maxTransCommitTime, restorePointTime) ]
      INT32 getRestoreWindow( UINT64 &minTime, UINT64 &maxTransCommitTime,
                              UINT64 &restorePointTime ) ;

      // get max transaction commit time before given LSN
      // input:
      // - lsn: given LSN to get max transaction commit time before
      // output:
      // - maxTime: max transaction commit time before given LSN
      // return :
      // - SDB_OK: succeed
      // - SDB_DPS_LSN_OUTOFRANGE: given LSN could not been found in log files
      // NOTE: this commit time is calculate from log summary
      INT32 getMaxCommitTimeBefore( DPS_LSN_OFFSET lsn, UINT64 &maxTime ) ;

      // set minimum recoverable time
      OSS_INLINE void setMinRecoverableTime( UINT64 minRecoverableTime )
      {
         ossAtomicExchangePtr( &_minRecoverableTime, minRecoverableTime ) ;
      }

      // set maximum transaction commit time
      OSS_INLINE void setMaxTransCommitTime( UINT64 maxTransCommitTime )
      {
         ossAtomicExchangePtr( &_maxTransCommitTime, maxTransCommitTime ) ;
      }

      // get maximum transaction commit time
      OSS_INLINE UINT64 getMaxTransCommitTime()
      {
         return ossAtomicFetch64( &_maxTransCommitTime ) ;
      }

      // set restore point time (either backup time or restorePrepare time)
      OSS_INLINE void setRestorePointTime( UINT64 restorePointTime )
      {
         ossAtomicExchangePtr( &_restorePointTime, restorePointTime ) ;
      }

      // dump transaction information into log summary
      // - dump minimum recoverable time
      // - dump maximum transaction time
      // - dump running time
      // input:
      // - inLock: whether call this function under protection of log lock
      //           ( write mutex of DPS log )
      // output:
      // - summary: log summary to dump transaction information
      OSS_INLINE void dumpLogSummary( BOOLEAN inLock,
                                      dpsLogSummary &summary )
      {
         if ( inLock )
         {
            // in lock, no need to use atomic fetch
            summary._minRecoverableTime = _minRecoverableTime ;
            summary._maxTransCommitTime = _maxTransCommitTime ;
            summary._restorePointTime = _restorePointTime ;
         }
         else
         {
            // not in lock, use atomic fetch
            summary._minRecoverableTime =
                  ossAtomicFetch64( &_minRecoverableTime ) ;
            summary._maxTransCommitTime =
                  ossAtomicFetch64( &_maxTransCommitTime ) ;
            summary._restorePointTime =
                  ossAtomicFetch64( &_restorePointTime ) ;
         }
      }

      // update restore window
      // - update maximum transaction commit time
      OSS_INLINE void updateRestoreWindow( UINT64 transTime )
      {
         // if maximum transaction commit time is smaller than given time,
         // set to given time
         if ( _maxTransCommitTime < transTime ||
              DPS_INVALID_TRANSID_SN == _maxTransCommitTime )
         {
            _maxTransCommitTime = transTime ;
         }
      }

      // reset restore window to invalid values
      OSS_INLINE void resetRestoreWindow()
      {
         // reset to invalid values
         _minRecoverableTime = DPS_INVALID_TRANS_TIME ;
         _maxTransCommitTime = DPS_INVALID_TRANS_TIME ;
         _restorePointTime = DPS_INVALID_TRANS_TIME ;
      }

      // push restore window forward
      OSS_INLINE void pushRestoreWindow()
      {
         // push the restore window by irreversible operators like DDL
         // or non-transaction operators
         // e.g. the current window is ( min: 10, max: 20 ), after a
         // DDL, it becames ( min: 21, max: 20 )
         // NOTE: we don't move the max time to avoid a large volumn
         // of irreversible operators to push the window with large
         // values which might exceeds the current logical time
         if ( DPS_INVALID_TRANS_TIME != _maxTransCommitTime &&
              _minRecoverableTime < _maxTransCommitTime )
         {
            _minRecoverableTime = _maxTransCommitTime + 1 ;
         }
      }

      // rollback restore window to given transaction time
      OSS_INLINE void rollbackRestoreWindow( UINT64 transTime )
      {
         _minRecoverableTime = transTime ;
         _maxTransCommitTime = transTime ;
      }

      OSS_INLINE ossEvent *getUpdateLowTranEvent()
      {
         return &( _updateLowTranEvent ) ;
      }

      OSS_INLINE ossEvent *getWaitLowTranEvent()
      {
         return &( _waitLowTranEvent ) ;
      }

      // increase count of transaction ID allocation conflict
      OSS_INLINE void incTransIDConflict()
      {
         ++ _numTransIDConflict ;
      }

      // Check if EDU hold certain lock and return the holding mode
      BOOLEAN isHolding( _pmdEDUCB *eduCB,
                         INT8   & owningLockMode, 
                         UINT32 logicCSID,
                         UINT16 collectionID = DMS_INVALID_MBID,
                         const dmsRecordID *recordID = NULL ) ;

      oldVersionCB * getOldVCB () { return _oldVCB ; }

      BOOLEAN isRollback( const DPS_TRANS_ID &transID ) ;
      BOOLEAN isFirstOp( const DPS_TRANS_ID &transID ) ;
      void    clearFirstOpTag( DPS_TRANS_ID &transID ) ;

      // check transaction if rollback pending
      BOOLEAN isRBPending( const DPS_TRANS_ID &transID ) ;
      // check if has rollback pending transactions
      BOOLEAN hasRBPendingTrans() ;

      INT32 startRollbackTask() ;
      INT32 stopRollbackTask( UINT64 doRollbackID ) ;
      BOOLEAN isDoRollback() const { return _doRollback ; }
      INT32   waitRollback( UINT64 millicSec = -1 ) ;

      INT32 addTransInfo( DPS_TRANS_ID transID,
                          DPS_LSN_OFFSET lsnOffset,
                          INT32 status ) ;
      // NOTE: log rollback means rollbacked by consulting or replay failure
      // find transaction info from transMap and update with given transaction
      // ID
      // input:
      //    - transID: transaction ID
      //    - lsnOffset: last LSN offset of transaction
      //    - status: status of transaction
      //    - transTime: begin time or commit time of transaction
      //    - checkRstPITWindow: whether to check restore window
      // WARNING: this should be only called in callback of DPS logger
      //          these inputs should be only parsed from DPS record
      //          the DPS record could be replayed or rollbacked multiple
      //          times, so we only keep the latest status after any replay or
      //          rollback
      void updateTransInfo( const DPS_TRANS_ID &transID,
                            DPS_LSN_OFFSET lsnOffset,
                            INT32 status,
                            const stpLogicalTimeUS &transTime,
                            BOOLEAN checkRstPITWindow ) ;
      // update transaction info with given transaction ID
      // output:
      //    - transInfo: transaction info to be updated
      // input:
      //    - status: status of transaction
      //    - lsn: last LSN offset of transaction
      //    - rbPending: indicates if transaction is rollback pending
      void updateTransInfo( dpsTransBackInfo &transInfo,
                            INT32 status,
                            DPS_LSN_OFFSET lsn,
                            BOOLEAN rbPending ) ;

      // update transaction status with given transaction ID
      // input:
      //    - transID: transaction ID
      //    - status: transaction status to update
      // return:
      //    - SDB_OK: update succeed
      //    - SDB_DPS_TRANS_NO_TRANS: transaction info is not found
      INT32 updateTransStatus( const DPS_TRANS_ID &transID,
                               INT32 status ) ;

      BOOLEAN  addTransCB( const DPS_TRANS_ID &transID, _pmdEDUCB *eduCB ) ;
      void     delTransCB( const DPS_TRANS_ID &transID ) ;
      void     dumpTransEDUList( TRANS_EDU_LIST  &eduList ) ;
      void     snapTransLockWaiterLRB( DPS_TX_WAIT_LRB_SET & txWaiterLRBSet ) ;
      UINT32   getTransCBSize() ;
      void     termAllTrans() ;
      UINT32   getTransMapSize() ;
      void     removeTrans( const DPS_TRANS_ID &transID ) ;
      void     cloneTransMap( TRANS_DUMP_MAP &result ) ;

      void     addHisTrans( const DPS_TRANS_ID &transID,
                            const dpsHisTransStatus &histInfo,
                            BOOLEAN checkRstPITWindow ) ;
      void     delHisTrans( const DPS_TRANS_ID &transID ) ;
      void     clearHisTrans() ;
      void     clearOutDateHisTrans( DPS_LSN_OFFSET lsn ) ;

      void     clearTransInfo() ;

      void     saveTransInfoFromLog( const dpsLogRecord &record,
                                     BOOLEAN checkRstPITWindow ) ;

      void     saveTransInfoFromCtx( const dpsWriteContext &ctx,
                                     BOOLEAN checkRstPITWindow ) ;
      // rollback transaction info to expect LSN ( generally it is older than
      // replayer's completed LSN )
      INT32    rollbackTransInfoFromLog( _dpsLogWrapper *dpsCB,
                                         const DPS_LSN &expectLSN ) ;
      // rollback transaction info for a single DPS record
      BOOLEAN  rollbackTransInfoFromLog( const dpsLogRecord &record ) ;

      void           addBeginLsn( DPS_LSN_OFFSET beginLsn,
                                  const DPS_TRANS_ID &transID ) ;
      void           delBeginLsn( const DPS_TRANS_ID &transID ) ;
      DPS_LSN_OFFSET getBeginLsn( const DPS_TRANS_ID &transID ) ;
      DPS_LSN_OFFSET getOldestBeginLsn() ;

      BOOLEAN  isNeedSyncTrans() ;
      void     setIsNeedSyncTrans( BOOLEAN isNeed ) ;

      INT32 syncTransInfoFromLocal( DPS_LSN_OFFSET beginLsn,
                                    BOOLEAN checkRestoreWindow ) ;

      // get record-X-lock: also get the space-IS-lock and collection-IX-lock
      // get collection-X-lock: also get the space-IX-lock
      INT32 transLockGetX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           _IContext *pContext = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL ) ;

      // get record-U-lock: also get the space-IS-lock and collection-IS-lock
      INT32 transLockGetU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID,
                           const dmsRecordID *recordID,
                           _IContext *pContext = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL ) ;

      // get record-S-lock: also get the space-IS-lock and collection-IS-lock
      // get collection-S-lock: also get the space-IS-lock
      INT32 transLockGetS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           _IContext *pContext = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL,
                           BOOLEAN useEscalation = TRUE ) ;

      // also get the space-IS-lock
      INT32 transLockGetIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            _IContext *pContext = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // also get the space-IS-lock
      INT32 transLockGetIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            _IContext *pContext = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // release record-lock: also release the space-lock and collection-lock
      // release collection-lock: also release the space-lock
      void transLockRelease( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL,
                             _dpsITransLockCallback *callback = NULL,
                             BOOLEAN forceRelease = FALSE,
                             BOOLEAN releaseUpperLock = TRUE ) ;

      void transLockReleaseAll( _pmdEDUCB *eduCB,
                                _dpsITransLockCallback * callback = NULL ) ;

      BOOLEAN isTransOn() const ;
      BOOLEAN isGlobTransOn() const ;
      BOOLEAN isGlobTransSyncCheck() const ;
      BOOLEAN isMVCCOn() const ;
      // RR requires --transactionon, --globtranson and --mvccon
      BOOLEAN isRRSupported() const ;

      // test if the lock can be got.
      // test record-S-lock: also test the space-IS-lock and collection-IS-lock
      // test collection-IS-lock: also test the space-IS-lock
      INT32 transLockTestS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            const dmsRecordID *recordID = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL ) ;

      INT32 transLockTestSPreempt( _pmdEDUCB *eduCB, UINT32 logicCSID,
                                   UINT16 collectionID = DMS_INVALID_MBID,
                                   const dmsRecordID *recordID = NULL,
                                   dpsTransRetInfo * pdpsTxResInfo = NULL,
                                   _dpsITransLockCallback *callback = NULL,
                                   BOOLEAN needUpperLock = TRUE ) ;

      INT32 transLockTestIS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL,
                             dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // test if the lock can be got.
      // test record-X-lock: also test the space-IS-lock and collection-IX-lock
      INT32 transLockTestX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID = DMS_INVALID_MBID,
                            const dmsRecordID *recordID = NULL,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL,
                            BOOLEAN needUpperLock = TRUE ) ;

      INT32 transLockTestIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL,
                             dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // test if the lock can be got.
      // test record-U-lock: also test the space-IX-lock and collection-IX-lock
      INT32 transLockTestU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID ,
                            const dmsRecordID *recordID,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL ) ;

      // test if the Z lock can be got.
      INT32 transLockTestZ( _pmdEDUCB *eduCB,
                            UINT32 logicCSID,
                            UINT16 collectionID ,
                            const dmsRecordID *recordID,
                            dpsTransRetInfo *pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL ) ;

      // try to get record-X-lock: also try to get the space-IX-lock and
      // collection-IX-lock
      // try to get collection-X-lock: also try to get the space-IX-lock
      INT32 transLockTryX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback * callback = NULL ) ;

      // try to get record-Z-lock: also try to get the space-IX-lock and
      // collection-IX-lock
      // try to get collection-Z-lock: also try to get the space-IX-lock
      INT32 transLockTryZ( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback * callback = NULL ) ;

      // try to get record-U-lock: also try to get the space-IX-lock and
      // collection-IX-lock
      INT32 transLockTryU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID ,
                           const dmsRecordID *recordID,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback *callback = NULL ) ;

      // try to get record-S-lock: also try to get the space-IS-lock and
      // collection-IS-lock
      // try to get collection-S-lock: also try to get the space-IS-lock
      INT32 transLockTryS( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback * callback = NULL ) ;

      // check if any writing transactions on the object, and then try acquire
      // S lock
      INT32 transLockTrySAgainstWrite( _pmdEDUCB *eduCB,
                                       UINT32 logicCSID,
                                       UINT16 collectionID = DMS_INVALID_MBID,
                                       const dmsRecordID *recordID = NULL,
                                       dpsTransRetInfo *pdpsTxResInfo = NULL,
                                       _dpsITransLockCallback *callback = NULL ) ;

      // kill waiters for a specified lock ID
      BOOLEAN transLockKillWaiters( UINT32 logicCSID,
                                    UINT16 collectionID,
                                    const dmsRecordID *recordID,
                                    INT32 errorCode ) ;

      BOOLEAN transIsHolding( _pmdEDUCB *eduCB, UINT32 logicCSID,
                              UINT16 collectionID,
                              const dmsRecordID *recordID ) ;

      BOOLEAN hasWait( UINT32 logicCSID, UINT16 collectionID,
                       const dmsRecordID *recordID) ;

      INT32 getIncompTrans( _pmdEDUCB *               cb,
                            const dpsTransLockId &    lockID,
                            const DPS_TRANSLOCK_TYPE  lockMode,
                            BOOLEAN                   canSelfIncomp,
                            DPS_TRANS_ID_SET &        incompTrans ) ;

      INT32 reservedLogSpace( UINT32 length, _pmdEDUCB *cb ) ;

      void releaseLogSpace( UINT32 length, _pmdEDUCB *cb ) ;

      void releaseRBLogSpace( _pmdEDUCB *cb ) ;

      UINT64 remainLogSpace() ;

      UINT64 usedLogSpace() ;

      dpsTransLockManager * getLockMgrHandle() ;
      ixmIndexLockManager * getIndexLockMgrHandle() ;

      UINT32 getMaxLRSize() ;
      void   updateMaxLRSize( UINT32 recordSize, DPS_LSN_OFFSET curLSN ) ;
      void   printCounters() ;

      // succeed and error counts
      UINT64 getSucCount() const
      {
         return _sucCount ;
      }

      UINT64 getErrCount() const
      {
         return _errCount ;
      }

      void incSucCount()
      {
         ++ _sucCount ;
      }

      void incErrCount()
      {
         ++ _errCount ;
      }

      // report global transaction errors
      void incGlobErrCount( INT32 rc )
      {
         // only count error when STP or global transaction is not
         // available
         if ( STP_NOT_AVAILABLE == rc ||
              SDB_GLOB_TRANS_NOT_AVAILABLE == rc )
         {
            ++ _errCount ;
         }
      }

      // get GTS agent
      OSS_INLINE _dpsGTSAgent *getGTSAgent()
      {
         return _gtsAgent ;
      }

      // register GTS agent to transCB
      void registerGTSAgent( _dpsGTSAgent *gtsAgent ) ;
      // unregister GTS agent from transCB
      void unregisterGTSAgent() ;

      // get record transID from oldVersion container
      BOOLEAN getOldVerRecordTransID( UINT32 logicCSID,
                                      UINT16 collectionID,
                                      const dmsRecordID *recordID,
                                      DPS_TRANS_ID &transID ) ;

      // Rollback log limit cache. Used by _rtnRestoreCheck. The rollback
      // manager is given a target rollback time and simulates rolling back
      // everything newer. As it scans the log it sums up the space needed for
      // rollback log records and checks against the available log space. If
      // the log space is filled before the target time is reached, the limit
      // is recorded. This allows for skipping the check on subsequent runs.

      // Set the rollback log limit cache
      void setLogLimitTime( UINT64 tim, UINT64 lim ) ;

      // Reset the values for the rollback log limit cache
      void clearLogLimitTime() ;

      // Get the rollback log limit for the target time
      UINT64 getLogLimitTime( UINT64 tim ) ;

   protected:
      // initialize transaction maps
      void _initTransMaps() ;

      // initialize transaction info from DPS
      INT32 _initFromDPS() ;

      // get global lowTran with a given time error as offset
      // return:
      //    - DPS_INVALID_TRANSID_SN: global lowTran is invalid
      //    - DPS_MAX_TRANSID_SN: global transaction feature is not enabled
      //                          among all nodes
      //    - other values: global lowTran with time error as offset
      DPS_TRANSID_SN _getGlobLowTran() ;

      // get running transaction info
      // input:
      //    - transID: transaction ID to search
      // output:
      //    - info: transaction info to be returned
      // return:
      //    - TRUE: transaction info is found
      //    - FALSE: transaction info is not found
      // NOTE: must be origin transaction ID
      BOOLEAN _getTransInfo( const DPS_TRANS_ID &transID,
                             dpsTransBackInfo &info ) ;

      // get history transaction info
      // input:
      //    - transID: transaction ID to search
      // output:
      //    - histInfo: history transaction info to be returned
      // return:
      //    - TRUE: history transaction info is found
      //    - FALSE: history transaction info is not found
      // NOTE: must be origin transaction ID
      BOOLEAN _getTransHistInfo( const DPS_TRANS_ID &transID,
                                 dpsHisTransStatus &histInfo ) ;

      // checks version visibility in global for doing write transaction
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - recTransID: transaction ID of record transaction
      //    - transID: transaction ID of current transaction
      //    - transBeginTime: begin time of current transaction
      // output:
      //    - visible: indicate if current transaction could see record
      //    - visibleTime: indicate the visible time of this record
      // return:
      //    - SDB_OK: succeed to check visibility
      //    - other errors: failed to check visibility
      // NOTE: this functions checks visibility between transactions with
      //       arbitration to remote node ( node to launch the current
      //       transaction ) if needed
      INT32 _isGlobDoingVisible( _pmdEDUCB *eduCB,
                                 const DPS_TRANS_ID &recTransID,
                                 const dpsTransBackInfo &recTransInfo,
                                 const DPS_TRANS_ID &transID,
                                 const stpLogicalTimeUS &transBeginTime,
                                 BOOLEAN &visible,
                                 stpLogicalTimeUS &visibleTime ) ;

      // checks version visibility in global for wait-commit write transaction
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - recTransID: transaction ID of record transaction
      //    - transID: transaction ID of current transaction
      //    - transBeginTime: begin time of current transaction
      // output:
      //    - visible: indicate if current transaction could see record
      //    - visibleTime: indicate the visible time of this record
      // return:
      //    - SDB_OK: succeed to check visibility
      //    - other errors: failed to check visibility
      // NOTE: this functions checks visibility between transactions with
      //       arbitration to remote node ( node to launch the current
      //       transaction ) if needed
      INT32 _isGlobWaitCommitVisible( _pmdEDUCB *eduCB,
                                      const DPS_TRANS_ID &recTransID,
                                      const dpsTransBackInfo &recTransInfo,
                                      const DPS_TRANS_ID &transID,
                                      const stpLogicalTimeUS &transBeginTime,
                                      BOOLEAN &visible,
                                      stpLogicalTimeUS &visibleTime ) ;

      // checks version visibility in global cluster with time error
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - recTransID: transaction ID of record transaction
      //    - transID: transaction ID of current transaction
      //    - transBeginTime: begin time of current transaction
      // output:
      //    - visible: indicate if current transaction could see record
      //    - visibleTime: indicate the visible time of this record
      // return:
      //    - SDB_OK: succeed to check visibility
      //    - other errors: failed to check visibility
      // NOTE: this functions checks visibility between transactions with
      //       arbitration to remote node ( node to launch the current
      //       transaction ) if needed
      INT32 _isGlobVisible( _pmdEDUCB *eduCB,
                            const DPS_TRANS_ID &recTransID,
                            const DPS_TRANS_ID &transID,
                            const stpLogicalTimeUS &transBeginTime,
                            BOOLEAN &visible,
                            stpLogicalTimeUS &visibleTime ) ;

      // checks version visibility in local node without time error
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - recTransID: transaction ID of record transaction
      //    - transID: transaction ID of current transaction
      //    - transBeginTime: begin time of current transaction
      // output:
      //    - visible: indicate if current transaction could see record
      //    - visibleTime: indicate the visible time of this record
      // return:
      //    - SDB_OK: succeed to check visibility
      //    - other errors: failed to check visibility
      // NOTE: this functions checks visibility between transactions without
      //       arbitration enabled or from the same node
      INT32 _isLocalVisible( _pmdEDUCB *eduCB,
                             const DPS_TRANS_ID &recTransID,
                             const DPS_TRANS_ID &transID,
                             const stpLogicalTimeUS &transBeginTime,
                             BOOLEAN &visible,
                             stpLogicalTimeUS &visibleTime ) ;

   private:
      // node ID of transaction
      DPS_TRANSID_NODEID _TransIDH16 ;
      // atomic to generate 56 bit SN for non global transactions
      ossAtomic64       _TransIDL56Cur ;

      // transaction map ( running write transactions )
      TRANS_MAP         _transMap ;

      // transaction to edu CB map ( all running transactions )
      TRANS_CB_MAP      _cbMap ;

      BOOLEAN           _isOn ;
      BOOLEAN           _isGlobTransOn ;

      // hidden option to indicate if we need to check logical time
      // synchronization between transaction nodes
      BOOLEAN           _isGlobTransSyncCheck ;

      BOOLEAN           _isMVCCOn ;
      BOOLEAN           _doRollback ;
      UINT64            _doRollbackID ;
      ossEvent          _rollbackEvent ;

      monSpinSLatch     _lsnMapMutex ;
      TRANS_LSN_ID_MAP  _beginLsnIdMap ;
      TRANS_ID_LSN_MAP  _idBeginLsnMap ;

      // history map for global transactions
      // NOTE: global transactions are cleared by expireTran, so no need to
      //       save LSN map as secondary index
      TRANS_HIST_MAP    _histGlobMap ;

      // history map for rolled back ( non-global ) transactions
      TRANS_HIST_MAP    _histRBMap ;
      // LSN map for rolled back ( non-global ) transactions
      // NOTE: non-global transactions are cleared by begin LSN of DPS logger
      //       so we need a LSN map as secondary index
      // WARNING: should be protected by _histRBMap's bucket lock
      TRANS_LSN_ID_MAP  _histRBLSNMap[ DPS_TRANS_BUCKET_SIZE ] ;

      BOOLEAN           _isNeedSyncTrans ;
      UINT64            _logFileTotalSize ;

      // Largest two record size within the system, and the most recent LR LSN
      // used these two record. We need the largest record size to caculate if
      // we have sufficient log size for LR during reserveLogSpace
      UINT32            _maxLRSize1 ;
      UINT64            _maxLRLSN1 ;
      UINT32            _maxLRSize2 ;
      UINT64            _maxLRLSN2 ;
      // The _reservedRBspace and _reservedSpace are incremented by the size of
      // LR at runtime before writting LR. Once LR is written, _reservedSpace
      // is released.
      // each transaction also track total log space it reserves
      // for rollback. The space is released during commit or rollback.
      // _reservedRBSpace holds the sum of such space from all transactions
      ossAtomic64       _reservedRBSpace ;
      ossAtomic64       _reservedSpace ;

      dpsTransLockManager  *_transLockMgr ;
      ixmIndexLockManager  *_indexLockMgr ;
      oldVersionCB         *_oldVCB ;  // control block holding old(last committed)
                                       // version of record and index key value

      dpsTransEvent        *_pEventHandler ;

      // succeed and error counts
      // no need to be atomic
      volatile UINT64      _sucCount ;
      volatile UINT64      _errCount ;

      // minimum logical time to accept global transactions
      // NOTE: set to logical time of this node to become primary
      DPS_TRANSID_SN_ATOMIC _primaryActiveTime ;

      // global lowTran ( global minimum running transaction )
      // NOTE: only save timestamp SN and global tag )
      DPS_TRANSID_SN_ATOMIC _globLowTran ;

      // global expireTran ( global maximum expired transaction )
      // NOTE: only save timestamp SN and global tag )
      DPS_TRANSID_SN_ATOMIC _globExpireTran ;

      // archived lowTran from cb map
      // NOTE:
      // - cb map might be cleared, so keep an archived value for lowTran
      // - archived lowTran is the largest finished transaction ID
      // - if cb map is empty, archived lowTran will be the local lowTran
      DPS_TRANSID_SN_ATOMIC _archivedLowTran ;

      // upper bound of max read transaction ID ( with time error )
      // NOTE:
      // - maxReadTran is the transaction ID of the latest started read
      //   transaction with upper bound of time error
      // - maxReadTran is used to check pre-commit time of write transactions
      //   which should be delayed by running read transactions ( which is
      //   indicates by maxReadTran )
      DPS_TRANSID_SN_ATOMIC _maxReadTran ;

      // maximum commit time of global transactions in this node
      // NOTE:
      // - only for write transactions
      // - no need to be atomic
      //   - for primary node, it is protected by mutex of log writer
      //   - for secondary node or restore mode, it is updated by main thread
      //     of log replayer
      UINT64               _maxTransCommitTime ;

      // minimum recoverable time in this node
      // NOTE:
      // - affected by irreversible operators ( DDL operators, e.g. drop,
      //   truncate, etc )
      // - no need to be atomic
      //   - for primary node, it is protected by mutex of log writer
      //   - for secondary node or restore mode, it is updated by main thread
      //     of log replayer
      UINT64               _minRecoverableTime ;

      // restore point time of this node
      // NOTE:
      // - set by restorePrepare (to time of command) or by the sdbrestore tool
      //   (for a global backup)
      UINT64               _restorePointTime ;

      // update event to notify lowTran job to update global lowTran
      ossEvent             _updateLowTranEvent ;
      // wait event to wait lowTran to finish global lowTran update
      ossEvent             _waitLowTranEvent ;

      // record conflict counts of allocate transaction ID
      // NOTE:
      // - increase when generated the same transaction ID by STP
      // - no need to be atomic, just a value for statistics
      UINT64               _numTransIDConflict ;

      // STP agent to provide global logical time service
      stpAgent             _stpAgent ;

      // global transaction service agent
      _dpsGTSAgent *       _gtsAgent ;

      // Rollback log limit cache values - time checked, time limit
      UINT64               _rollbackLogTime ;
      UINT64               _rollbackLogLimit ;
   } ;

   /*
      get global cb obj
   */
   dpsTransCB* sdbGetTransCB () ;

}

#endif // DPSTRANSCB_HPP_
