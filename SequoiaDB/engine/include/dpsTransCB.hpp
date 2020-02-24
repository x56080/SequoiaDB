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
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{
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

      stpLogicalTimeUS              _beginTime ;
      stpLogicalTimeUS              _preCommitTime ;

      _dpsTransBackInfo( DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET,
                         INT32 status = DPS_TRANS_DOING )
      {
         _lsn = lsn ;
         _curLSNWithRBPending = DPS_INVALID_LSN_OFFSET ;
         _status = status ;
      }
   } ;
   typedef _dpsTransBackInfo dpsTransBackInfo ;

   typedef ossPoolMap<DPS_TRANS_ID, dpsTransBackInfo> TRANS_MAP ;
   typedef ossPoolMap<DPS_TRANS_ID, _pmdEDUCB * >     TRANS_CB_MAP ;
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

      _dpsHisTransStatus()
      : _status( DPS_TRANS_COMMIT ),
        _lsn( DPS_INVALID_LSN_OFFSET ),
        _beginTime(),
        _preCommitTime()
      {
      }

      _dpsHisTransStatus( INT32 status,
                          DPS_LSN_OFFSET lsn,
                          const stpLogicalTimeUS &beginTime,
                          const stpLogicalTimeUS &commitTime )
      : _status( status ),
        _lsn( lsn ),
        _beginTime( beginTime ),
        _preCommitTime( commitTime )
      {
      }
   } ;
   typedef _dpsHisTransStatus dpsHisTransStatus ;

   typedef ossPoolMap<DPS_TRANS_ID, dpsHisTransStatus>   TRANS_ID_2_STATUS ;

   // delta between runtime log and undo log due to TransRelatedLSN
   #define DPS_TRANS_LOG_UNDO_DELTA  ( 12 )

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
                              BOOLEAN &visible ) ;

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
      // return:
      //    - SDB_OK: succeed to get time
      //    - STP_NOT_AVAILABLE: STP is not available for global transaction
      //    - SDB_TIMEOUT: failed to get time in given timeout
      INT32 getGlobTransTime( stpLogicalTimeUS &time,
                              INT32 timeout = OSS_ONE_SEC ) ;

      // get logical time from STP for pre-commit/commit of transaction
      // input:
      //    - eduCB: EDUCB of transaction
      // output:
      //    - preCommitTime: logical time to pre-commit/commit transaction
      // return:
      //    - SDB_OK: succeed to get time
      //    - STP_NOT_AVAILABLE: STP is not available for global transaction
      //    - SDB_TIMEOUT: failed to get time in given timeout
      // NOTE: for global transaction, it should be pre-commit or commit in
      //       a time error period later after transaction begin
      INT32 getGlobTransPreCommitTime( _pmdEDUCB *eduCB,
                                       stpLogicalTimeUS &preCommitTime ) ;

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

      // pre-arbitrate global write transaction
      // input:
      //    - writeTransID: transaction ID of current write transaction
      //    - currentTime: current time to get candidate transactions
      // return:
      //    - SDB_OK: succeed to do pre-arbitration
      //    - other errors: failed to do pre-arbitration
      // NOTE: transactions started before current time could be candidate
      //       pre-arbitrate transaction
      INT32 doPreArbitGlobTrans( const DPS_TRANS_ID &writeTransID,
                                 const stpLogicalTimeUS &currentTime ) ;

      // handle pre-arbitrate request for global write transaction
      // input:
      //    - writeTransID: transaction ID of current write transaction
      //    - preArbitList: list of pre-arbitrate read transactions
      // return:
      //    - SDB_OK: succeed to do pre-arbitration
      //    - other errors: failed to do pre-arbitration
      INT32 onPreArbitGlobTrans( const DPS_TRANS_ID &writeTransID,
                                 const TRANS_ID_LIST &preArbitList ) ;

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
      INT32 stopRollbackTask() ;
      BOOLEAN isDoRollback() const { return _doRollback ; }
      INT32   waitRollback( UINT64 millicSec = -1 ) ;

      void addTransInfo( const DPS_TRANS_ID &transID,
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
      // WARNING: this should be only called in callback of DPS logger
      //          these inputs should be only parsed from DPS record
      //          the DPS record could be replayed or rollbacked multiple
      //          times, so we only keep the latest status after any replay or
      //          rollback
      void updateTransInfo( const DPS_TRANS_ID &transID,
                            DPS_LSN_OFFSET lsnOffset,
                            INT32 status,
                            const stpLogicalTimeUS &transTime ) ;
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

      void updateTransStatus( DPS_TRANS_ID transID,
                              INT32 status ) ;

      BOOLEAN  addTransCB( const DPS_TRANS_ID &transID, _pmdEDUCB *eduCB ) ;
      void     delTransCB( const DPS_TRANS_ID &transID ) ;
      void     dumpTransEDUList( TRANS_EDU_LIST  &eduList ) ;
      UINT32   getTransCBSize() ;
      void     termAllTrans() ;
      TRANS_MAP *getTransMap() ;
      void     cloneTransMap( TRANS_MAP &result ) ;

      void     addHisTrans( const DPS_TRANS_ID &transID,
                            const dpsHisTransStatus &histInfo ) ;
      void     delHisTrans( const DPS_TRANS_ID &transID ) ;
      void     clearHisTrans() ;
      void     clearOutDateHisTrans( DPS_LSN_OFFSET lsn ) ;
      INT32    checkTransStatus( const DPS_TRANS_ID &transID,
                                 DPS_LSN_OFFSET & lsn ) ;

      void     clearTransInfo() ;

      void     saveTransInfoFromLog( const dpsLogRecord &record ) ;
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

      INT32 syncTransInfoFromLocal( DPS_LSN_OFFSET beginLsn ) ;

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
                           _dpsITransLockCallback *callback = NULL ) ;

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
                             _dpsITransLockCallback *callback = NULL ) ;

      void transLockReleaseAll( _pmdEDUCB *eduCB,
                                _dpsITransLockCallback * callback = NULL ) ;

      BOOLEAN isTransOn() const ;
      BOOLEAN isGlobTransOn() const ;
      BOOLEAN isGlobTransSyncCheck() const ;
      BOOLEAN isGlobTransArbitOn() const ;
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
                                   _dpsITransLockCallback *callback = NULL ) ;

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
                            _dpsITransLockCallback *callback = NULL ) ;

      INT32 transLockTestIX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                             UINT16 collectionID = DMS_INVALID_MBID,
                             const dmsRecordID *recordID = NULL,
                             dpsTransRetInfo * pdpsTxResInfo = NULL ) ;

      // test if the lock can be got.
      // test record-U-lock: also test the space-IS-lock and collection-IS-lock
      INT32 transLockTestU( _pmdEDUCB *eduCB, UINT32 logicCSID,
                            UINT16 collectionID ,
                            const dmsRecordID *recordID,
                            dpsTransRetInfo * pdpsTxResInfo = NULL,
                            _dpsITransLockCallback *callback = NULL ) ;


      // try to get record-X-lock: also try to get the space-IS-lock and
      // collection-IX-lock
      // try to get collection-X-lock: also try to get the space-IX-lock
      INT32 transLockTryX( _pmdEDUCB *eduCB, UINT32 logicCSID,
                           UINT16 collectionID = DMS_INVALID_MBID,
                           const dmsRecordID *recordID = NULL,
                           dpsTransRetInfo * pdpsTxResInfo = NULL,
                           _dpsITransLockCallback * callback = NULL ) ;


      // try to get record-U-lock: also try to get the space-IS-lock and
      // collection-IS-lock
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

      BOOLEAN hasWait( UINT32 logicCSID, UINT16 collectionID,
                       const dmsRecordID *recordID) ;

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

      // get GTS agent
      OSS_INLINE _dpsGTSAgent *getGTSAgent()
      {
         return _gtsAgent ;
      }

      // register GTS agent to transCB
      void registerGTSAgent( _dpsGTSAgent *gtsAgent ) ;
      // unregister GTS agent from transCB
      void unregisterGTSAgent() ;

   protected:
      // get global lowTran with a given time error as offset
      // return:
      //    - DPS_INVALID_TRANSID_SN: global lowTran is invalid
      //    - DPS_MAX_TRANSID_SN: global transaction feature is not enabled
      //                          among all nodes
      //    - other values: global lowTran with time error as offset
      DPS_TRANSID_SN _getGlobLowTran() ;

      // get global expireTran with a given time error as offset
      // return:
      //    - DPS_INVALID_TRANSID_SN: global expireTran is invalid
      //    - DPS_MAX_TRANSID_SN: global transaction feature is not enabled
      //                          among all nodes
      //    - other values: global expireTran with time error as offset
      DPS_TRANSID_SN _getGlobExpireTran() ;

      // get candidate global transactions to pre-arbitrate for
      // given write transactions
      // input:
      //    - writeTransID: global write transaction for pre-arbitration
      //    - currentTime: current global logical time
      // output:
      //    - preArbitList: candidate transactions to be pre-arbitrated
      // return:
      //    - SDB_OK: succeed to get list
      //    - other values: failed to get list
      INT32 _getPreArbitTrans( const DPS_TRANS_ID &writeTransID,
                               const stpLogicalTimeUS &currentTime,
                               TRANS_ID_LIST &preArbitList ) ;

      // filter candidate global transactions to pre-arbitrate for
      // given write transactions
      // input:
      //    - currentTime: current global logical time
      //    - preArbitList: candidate transactions to be pre-arbitrated
      //                    will remove non matched transactions from list
      // return:
      //    - SDB_OK: succeed to get list
      //    - other values: failed to get list
      // NOTE: we only want running transactions, so filter out non-doing
      //       transactions ( rollback or wait commit, etc )
      INT32 _filterPreArbitTrans( const stpLogicalTimeUS &currentTime,
                                  TRANS_ID_LIST &preArbitList ) ;

      // get running transaction info
      // input:
      //    - transID: transaction ID to search
      // output:
      //    - info: transaction info to be returned
      // return:
      //    - TRUE: transaction info is found
      //    - FALSE: transaction info is not found
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
                                 BOOLEAN &visible ) ;

      // checks version visibility in global for wait-commit write transaction
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - recTransID: transaction ID of record transaction
      //    - transID: transaction ID of current transaction
      //    - transBeginTime: begin time of current transaction
      // output:
      //    - visible: indicate if current transaction could see record
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
                                      BOOLEAN &visible ) ;

      // checks version visibility in global cluster with time error
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - recTransID: transaction ID of record transaction
      //    - transID: transaction ID of current transaction
      //    - transBeginTime: begin time of current transaction
      // output:
      //    - visible: indicate if current transaction could see record
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
                            BOOLEAN &visible ) ;

      // checks version visibility in local node without time error
      // input:
      //    - recTransID: transaction ID of record transaction
      //    - transID: transaction ID of current transaction
      //    - transBeginTime: begin time of current transaction
      // output:
      //    - visible: indicate if current transaction could see record
      // return:
      //    - SDB_OK: succeed to check visibility
      //    - other errors: failed to check visibility
      // NOTE: this functions checks visibility between transactions without
      //       arbitration enabled or from the same node
      INT32 _isLocalVisible( const DPS_TRANS_ID &recTransID,
                             const DPS_TRANS_ID &transID,
                             const stpLogicalTimeUS &transBeginTime,
                             BOOLEAN &visible ) ;

   private:
      // node ID of transaction
      DPS_TRANSID_NODEID _TransIDH16 ;
      // atomic to generate 56 bit SN for non global transactions
      ossAtomic64       _TransIDL56Cur ;

      monSpinSLatch     _MapMutex ;
      TRANS_MAP         _TransMap ;

      monSpinSLatch     _CBMapMutex ;
      TRANS_CB_MAP      _cbMap ;

      BOOLEAN           _isOn ;
      BOOLEAN           _isGlobTransOn ;

      // hidden option to indicate if we need to check logical time
      // synchronization between transaction nodes
      BOOLEAN           _isGlobTransSyncCheck ;

      // hidden option to indicate if we need to do arbitration between
      // global transactions
      BOOLEAN           _isGlobTransArbitOn ;

      BOOLEAN           _isMVCCOn ;
      BOOLEAN           _doRollback ;
      ossEvent          _rollbackEvent ;

      monSpinXLatch     _lsnMapMutex ;
      TRANS_LSN_ID_MAP  _beginLsnIdMap ;
      TRANS_ID_LSN_MAP  _idBeginLsnMap ;

      monSpinSLatch     _hisMutex ;
      TRANS_ID_2_STATUS _hisTransStatus ;
      TRANS_LSN_ID_MAP  _hisLsnTrans ;

      BOOLEAN           _isNeedSyncTrans ;
      monSpinXLatch     _maxFileSizeMutex ;
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

      // minimum logical time to accept global transactions
      // NOTE: set to logical time of this node to become primary
      ossAtomic64          _primaryActiveTime ;

      // global lowTran ( global minimum running transaction )
      // NOTE: only save timestamp SN and global tag )
      ossAtomic64          _globLowTran ;

      // global expireTran ( global maximum expired transaction )
      // NOTE: only save timestamp SN and global tag )
      ossAtomic64          _globExpireTran ;

      // archived lowTran from cb map
      // NOTE:
      // - cb map might be cleared, so keep an archived value for lowTran
      // - archived lowTran is the largest finished transaction ID
      // - if cb map is empty, archived lowTran will be the local lowTran
      ossAtomic64          _archivedLowTran ;

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
   } ;

   /*
      get global cb obj
   */
   dpsTransCB* sdbGetTransCB () ;

}

#endif // DPSTRANSCB_HPP_
