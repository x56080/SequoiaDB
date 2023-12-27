/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = dmsScanner.cpp

   Descriptive Name = Data Management Service Scanner

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsScanner.hpp"
#include "dms.hpp"
#include "dmsOprHandler.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageDataCommon.hpp"
#include "rtnTBScanner.hpp"
#include "rtnIXScanner.hpp"
#include "rtnDiskIXScanner.hpp"
#include "rtnMergeIXScanner.hpp"
#include "rtnScannerFactory.hpp"
#include "bpsPrefetch.hpp"
#include "dmsCompress.hpp"
#include "dpsTransLockMgr.hpp"
#include "dpsTransExecutor.hpp"
#include "dmsTransLockCallback.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"


using namespace bson ;

namespace engine
{

   /*
      _dmsIndexRecordRW implement
    */
   class _dmsIndexRecordRW : public _dmsRecordRW
   {
      public:
         _dmsIndexRecordRW( const _dmsRecordRW &recordRW, const CHAR * ptr )
         :_dmsRecordRW( recordRW )
         {
            if ( ptr )
            {
               _isDirectMem = TRUE ;
               _ptr = ( const dmsRecord* )ptr ;
            }
         }
   } ;
   typedef _dmsIndexRecordRW dmsIndexRecordRW ;

   /*
      _dmsIndexCoverRecordBuilder define and implement
    */
   class _dmsIndexCoverRecordBuilder
   {
   public:
      _dmsIndexCoverRecordBuilder( CHAR *pBuf )
      {
         _pBuf    = pBuf ;
         _pCur    = _pBuf ;
         _ppPos   = NULL ;
         _hasDone = FALSE ;

         init() ;
      }

      _dmsIndexCoverRecordBuilder( CHAR **ppPos )
      {
         _pBuf    = *ppPos ;
         _pCur    = _pBuf ;
         _ppPos   = ppPos ;
         _hasDone = FALSE ;

         init() ;
      }

      ~_dmsIndexCoverRecordBuilder()
      {
         done() ;
      }

      BOOLEAN isEmpty() const
      {
         return len() <= 5 ? TRUE : FALSE ;
      }

      UINT32 len() const
      {
         return _pCur - _pBuf ;
      }

      const CHAR *done()
      {
         if ( !_hasDone )
         {
            _hasDone = TRUE ;
            *_pCur = (CHAR)EOO ;
            ++ _pCur ;
            /// set size
            *( (UINT32 *)_pBuf ) = _pCur - _pBuf ;
            /// set pos
            if ( _ppPos )
            {
               *_ppPos = _pCur ;
            }
         }
         return _pBuf ;
      }

      _dmsIndexCoverRecordBuilder *appendElement( BSONElement &ele )
      {
         ossMemcpy( _pCur, ele.rawdata(), ele.size() ) ;
         _pCur += ele.size() ;
         return this ;
      }

      _dmsIndexCoverRecordBuilder* appendAs( const BSONElement &e,
                                             const StringData &fieldName )
      {
         *_pCur = (CHAR)( e.type() ) ;
         _pCur += 1 ;

         INT32 len ;
         len = fieldName.size() ;
         ossMemcpy( _pCur, fieldName.data(), len ) ;
         _pCur += len ;
         *_pCur = 0 ;
         _pCur += 1 ;

         len = e.valuesize() ;
         ossMemcpy( _pCur, (void *)( e.value() ), len ) ;
         _pCur += len ;

         return this ;
      }

      CHAR **subobjStart( const StringData &fieldName )
      {
         *_pCur = (CHAR) Object ;
         _pCur += 1 ;

         const INT32 len = fieldName.size() ;
         ossMemcpy( _pCur, fieldName.data(), len ) ;
         _pCur += len ;
         *_pCur = 0 ;
         _pCur += 1 ;

         return &_pCur ;
      }

      void abortSubobj( const StringData &fieldName, _dmsIndexCoverRecordBuilder &sub )
      {
         _pCur -= 1 ;
         const INT32 len = fieldName.size() + 1 ;
         _pCur -= len ;
         _pCur -= sub.len() ;
      }

      static BOOLEAN buildObj( ixmIndexNode *node,
                               IXM_ELE_RAWDATA_ARRAY &value,
                               _dmsIndexCoverRecordBuilder &builder ) ;

   protected:
      void init()
      {
         *((UINT32*)_pBuf) = 0 ;
         _pCur = _pBuf + 4 ;
      }

   private:
      CHAR * _pBuf ;
      CHAR * _pCur ;
      CHAR ** _ppPos ;
      BOOLEAN _hasDone ;
   } ;

   typedef class _dmsIndexCoverRecordBuilder dmsIndexCoverRecordBuilder ;

   BOOLEAN _dmsIndexCoverRecordBuilder::buildObj( ixmIndexNode *node,
                                                  IXM_ELE_RAWDATA_ARRAY &value,
                                                  _dmsIndexCoverRecordBuilder &builder )
   {
      BOOLEAN finished = FALSE ;

      // node tree:      a
      //                 |
      //            b(1) c(2) d(EOO)
      // then builder obj is : a{b:1,c:2}

      IXM_INDEX_NODE_PTR_ARRAY &children = node->getChildren() ;

      try
      {
         for ( UINT32 i = 0; i < node->childrenSize(); i++ )
         {
            if( 0 == children[i]->childrenSize() )
            {
               UINT32 fieldIndex = children[i]->getFieldIndex() ;
               SDB_ASSERT( fieldIndex < value.size(), "Field index bigger than field size" ) ;

               BSONElement ele( value[ fieldIndex ] ) ;

               if( Undefined != ele.type() )
               {
                  builder.appendAs( ele, children[i]->getName() );
               }
               else if( children[i]->isEmbedded() )
               {
                  // if index fields is {"a.b":1,c:1},insert {a:10,c:10}
                  // key value is {"":{"Undefined":1},"c":10}
                  // dms value is {a:10,c:10}
                  // not the same so we should to read dms value again
                  goto done ;
               }
            }
            else
            {
               _dmsIndexCoverRecordBuilder sub(
                           builder.subobjStart( children[i]->getName() ) ) ;
               if( FALSE == buildObj( children[i], value, sub ) )
               {
                  goto done ;
               }
               sub.done() ;
               if( sub.isEmpty() )
               {
                  builder.abortSubobj( children[i]->getName(), sub ) ;
               }
            }
         }
         finished = TRUE ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Failed to build index value object, "
                 "occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return finished ;

   error:
      goto done ;
   }

   /*
      _dmsScannerContext implement
    */
   _dmsScannerContext::_dmsScannerContext( dmsSecScanner *scanner )
   : _hasPaused( FALSE ),
     _scanner( scanner )
   {
   }

   _dmsScannerContext::~_dmsScannerContext ()
   {
      _hasPaused = FALSE ;
      _scanner = NULL ;
   }

   INT32 _dmsScannerContext::pause()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isHolding = FALSE ;
      dpsTransRetInfo dpsTxResInfo ;
      dpsTransCB *transCB = sdbGetTransCB() ;

      isHolding = transCB->transIsHolding( _scanner->getEDUCB(),
                                           _scanner->getDataSU()->logicalID(),
                                           _scanner->getMBContext()->mbID(),
                                           &_scanner->getAdvancedRecordID() ) ;

      if ( isHolding )
      {
         _hasPaused = TRUE ;
         // don't need to release transaction locks here, since we need them holding
         return  _scanner->getScanner()->pauseScan() ;
      }

      return rc ;
   }

   INT32 _dmsScannerContext::resume()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isCursorSame = FALSE ;

      if ( !_hasPaused )
      {
         goto done ;
      }

      _hasPaused = FALSE ;
      rc  = _scanner->getScanner()->resumeScan( isCursorSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resume scan, rc: %d", rc ) ;

      SDB_ASSERT( TRUE == isCursorSame, "Must be same" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _dmsScanner implement
   */
   _dmsScanner::_dmsScanner( dmsStorageDataCommon *su, dmsMBContext *context,
                             mthMatchRuntime *matchRuntime,
                             DMS_ACCESS_TYPE accessType,
                             INT64 maxRecords,
                             INT64 skipNum,
                             INT32 flags,
                             IDmsOprHandler *opHandler )
   {
      SDB_ASSERT( su, "storage data can't be NULL" ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;
      _pSu = su ;
      _context = context ;
      _matchRuntime = matchRuntime ;
      _accessType = accessType ;
      _mbLockType = SHARED ;

      _maxRecords = maxRecords ;
      _skipNum = skipNum ;
      _flags = flags ;

      _opHandler = opHandler ;
   }

   _dmsScanner::~_dmsScanner()
   {
      _context    = NULL ;
      _pSu        = NULL ;
   }

   void _dmsScanner::_saveAdvancedRecrodID( const dmsRecordID &recordID,
                                            INT32 rc )
   {
      if ( SDB_OK == rc )
      {
         _advancedRecordID = recordID ;
      }
      else
      {
         _advancedRecordID.reset() ;
      }
   }

   void _dmsScanner::_checkMaxRecordsNum( _mthRecordGenerator &generator )
   {
      if ( _maxRecords > 0 )
      {
         if ( _maxRecords >= generator.getRecordNum() )
         {
            _maxRecords -= generator.getRecordNum() ;
         }
         else
         {
            INT32 num = generator.getRecordNum() - _maxRecords ;
            generator.popTail( num ) ;
            _maxRecords = 0 ;
         }
      }
   }

   /*
      _dmsScannerLockHandler implement
    */
   _dmsScannerLockHandler::_dmsScannerLockHandler( IDmsOprHandler *opHandler,
                                                   INT32 flags )
   : _isInited( FALSE ),
     _pTransCB( pmdGetKRCB()->getTransCB() ),
     _transIsolation( TRANS_ISOLATION_RU ),
     _waitLock( FALSE ),
     _useRollbackSegment( TRUE ),
     _needEscalation( FALSE ),
     _hasLockedRecord( FALSE ),
     _recordLock( DPS_TRANSLOCK_MAX ),
     _selectLockMode( DPS_TRANSLOCK_MAX ),
     _lockOpMode( DPS_TRANSLOCK_OP_MODE_ACQUIRE ),
     _needUnLock( FALSE ),
     _CSCLLockHeld( FALSE ),
     _callback( opHandler )
   {
      // lock for update has higher priority
      if ( OSS_BIT_TEST( flags, FLG_QUERY_FOR_UPDATE ) )
      {
         _selectLockMode = DPS_TRANSLOCK_U ;
      }
      else if ( OSS_BIT_TEST( flags, FLG_QUERY_FOR_SHARE ) )
      {
         _selectLockMode = DPS_TRANSLOCK_S ;
      }
   }

   _dmsScannerLockHandler::~_dmsScannerLockHandler()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__ACQUIRECSCLLOCK, "_dmsScannerLockHandler::_acquireCSCLLock" )
   INT32 _dmsScannerLockHandler::_acquireCSCLLock( _dmsStorageDataCommon *su,
                                                   _dmsMBContext *mbContext,
                                                   pmdEDUCB *cb,
                                                   IContext *transContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__ACQUIRECSCLLOCK ) ;

      if ( !_CSCLLockHeld && DPS_TRANSLOCK_MAX != _recordLock )
      {
         dpsTransRetInfo   lockConflict ;
         if ( DPS_TRANSLOCK_IS == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIS( cb,
                                            su->logicalID(),
                                            mbContext->mbID(),
                                            transContext,
                                            &lockConflict ) ;
         }
         else if ( DPS_TRANSLOCK_IX == dpsIntentLockMode( _recordLock ) )
         {
            rc = _pTransCB->transLockGetIX( cb,
                                            su->logicalID(),
                                            mbContext->mbID(),
                                            transContext,
                                            &lockConflict ) ;
         }
         else
         {
            goto done ;
         }

         // this is performance improvement, failed to get lock should not
         // fail the operation
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDWARNING,
                      "Failed to get CS/CL lock, rc: %d" OSS_NEWLINE
                      "Conflict ( representative ):" OSS_NEWLINE
                      "   EDUID:  %llu" OSS_NEWLINE
                      "   TID:    %u" OSS_NEWLINE
                      "   LockId: %s" OSS_NEWLINE
                      "   Mode:   %s" OSS_NEWLINE,
                      rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;
            goto error ;
         }
         else
         {
            _CSCLLockHeld = TRUE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSCANLOCKHANDLER__ACQUIRECSCLLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__RELEASECSCLLOCK, "_dmsScannerLockHandler::_releaseCSCLLock" )
   void _dmsScannerLockHandler::_releaseCSCLLock( _dmsStorageDataCommon *su,
                                                  _dmsMBContext *mbContext,
                                                  pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__RELEASECSCLLOCK ) ;

      if ( _CSCLLockHeld )
      {
         _pTransCB->transLockRelease( cb,
                                      su->logicalID(),
                                      mbContext->mbID() ) ;
         _CSCLLockHeld = FALSE ;
      }

      PD_TRACE_EXIT( SDB__DMSSCANLOCKHANDLER__RELEASECSCLLOCK ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_CB, "_dmsScannerLockHandler::_initLockInfo" )
   void _dmsScannerLockHandler::_initLockInfo( _dmsStorageDataCommon *su,
                                               _dmsMBContext *mbContext,
                                               DMS_ACCESS_TYPE accessType,
                                               pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_CB ) ;

      if ( !_isInited )
      {
         dpsTransExecutor *pExe = cb->getTransExecutor() ;

         _transIsolation = pExe->getTransIsolation() ;
         _waitLock = pExe->isTransWaitLock() ;
         _useRollbackSegment = pExe->useRollbackSegment() ;

         _lockOpMode = DPS_TRANSLOCK_OP_MODE_ACQUIRE ;

         /// When not support trans
         if ( !su->isTransSupport( mbContext ) )
         {
            _recordLock = DPS_TRANSLOCK_MAX ;
         }
         /// When not in transaction
         else if ( DPS_INVALID_TRANS_ID == cb->getTransID() )
         {
            /// When not use trans lock
            if ( !pExe->useTransLock() )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            /// Write operation should release lock right now
            else if ( DMS_IS_WRITE_OPR( accessType ) )
            {
               _recordLock = DPS_TRANSLOCK_X ;
               _needUnLock = TRUE ;
               _useRollbackSegment = FALSE ;
            }
            /// Read is always no lock
            else
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
         }
         /// In transaction
         else
         {
            if ( cb->isInTransRollback() )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            else if ( !pExe->useTransLock() )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            else if ( DMS_IS_WRITE_OPR( accessType ) )
            {
               _recordLock = DPS_TRANSLOCK_X ;
               _needUnLock = FALSE ;
               _needEscalation = TRUE ;
            }
            else if ( TRANS_ISOLATION_RU == _transIsolation &&
                      DPS_TRANSLOCK_MAX == _selectLockMode )
            {
               _recordLock = DPS_TRANSLOCK_MAX ;
            }
            else
            {
               _recordLock =
                     DPS_TRANSLOCK_MAX != _selectLockMode ?
                                                _selectLockMode :
                                                DPS_TRANSLOCK_S ;
               if ( TRANS_ISOLATION_RS == _transIsolation ||
                    DPS_TRANSLOCK_MAX != _selectLockMode )
               {
                  _needUnLock = FALSE ;
                  _waitLock = TRUE ;
                  _needEscalation = TRUE ;
               }
               else
               {
                  _needUnLock = TRUE ;
               }
            }
         }

         _isInited = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_CB ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_INFO, "_dmsScannerLockHandler::_initLockInfo" )
   void _dmsScannerLockHandler::_initLockInfo( INT32 isolation,
                                               DPS_TRANSLOCK_TYPE lockType,
                                               DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode )
   {
      PD_TRACE_ENTRY( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_INFO ) ;

      if ( !_isInited )
      {
         _transIsolation = isolation ;
         _recordLock = lockType ;
         _selectLockMode = lockType ;
         _lockOpMode = lockOpMode ;

         _useRollbackSegment = FALSE ;
         _waitLock = TRUE ;
         _needUnLock = FALSE ;
         _isInited = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSSCANLOCKHANDLER__INITLOCKINFO_INFO ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSCANLOCKHANDLER__CHECKTRANSLOCK, "_dmsScannerLockHandler::_checkTransLock" )
   INT32 _dmsScannerLockHandler::_checkTransLock( _dmsStorageDataCommon *su,
                                                  _dmsMBContext *mbContext,
                                                  const dmsRecordID &curRID,
                                                  pmdEDUCB *cb,
                                                  dmsScanTransContext *transContext,
                                                  dmsRecordRW &recordRW,
                                                  dmsRecordID &waitUnlockRID,
                                                  BOOLEAN &skipRecord )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIXSECSCAN__CHECKTRANSLOCK ) ;

      BOOLEAN ignoredLock = FALSE ;
      dpsTransRetInfo lockConflict ;

      if ( DPS_TRANSLOCK_MAX == _recordLock )
      {
         goto done ;
      }

      /// already locked, but not the same, should release lock first
      if ( _hasLockedRecord &&
           waitUnlockRID.isValid() &&
           curRID != waitUnlockRID )
      {
         _pTransCB->transLockRelease( cb,
                                      su->logicalID(),
                                      mbContext->mbID(),
                                      &waitUnlockRID,
                                      &_callback ) ;
         waitUnlockRID.reset() ;
         _hasLockedRecord = FALSE ;
      }

      // attach the recordRW in callback
      _callback.attachRecordRW( &recordRW ) ;
      _callback.clearStatus() ;

      if ( DPS_TRANSLOCK_X == _recordLock )
      {
         // exclusive lock has to always wait on the lock
         rc = _pTransCB->transLockGetX( cb,
                                        su->logicalID(),
                                        mbContext->mbID(),
                                        &curRID,
                                        transContext,
                                        &lockConflict,
                                        &_callback ) ;
      }
      else if ( DPS_TRANSLOCK_U == _recordLock )
      {
         rc = _pTransCB->transLockGetU( cb, su->logicalID(),
                                        mbContext->mbID(),
                                        &curRID,
                                        transContext,
                                        &lockConflict,
                                        &_callback ) ;
      }
      // DPS_TRANSLOCK_S
      else
      {
         if ( !_waitLock )
         {
            // for new RC logic, we should first test on S lock instead
            // of directly wait on the record lock. Under the cover,
            // the lock call back function would try to use the old copy
            // (previous committed version) if exist
            rc = _pTransCB->transLockTestSPreempt( cb,
                                                   su->logicalID(),
                                                   mbContext->mbID(),
                                                   &curRID,
                                                   &lockConflict,
                                                   &_callback,
                                                   !_CSCLLockHeld ) ;
            ignoredLock = TRUE ;
            if ( _callback.isSkipRecord() )
            {
               _onRecordSkipped( curRID, transContext ) ;
               rc = SDB_OK ;
               skipRecord = TRUE ;
               goto done ;
            }
            if ( _callback.isUseOldVersion() )
            {
               rc = SDB_OK ;
            }
         }

         /// wait lock
         if ( _waitLock || rc )
         {
            // test S lock failed and the record is not in old version
            // container nor in RBS. most likely the one hold / wait X
            // hasn't finish updating the record.
            // NOTE: RS and lock for share requires lock escalation
            rc = _pTransCB->transLockGetS( cb,
                                           su->logicalID(),
                                           mbContext->mbID(),
                                           &curRID,
                                           transContext,
                                           &lockConflict,
                                           &_callback,
                                           _needEscalation ) ;
            if ( SDB_OK == rc )
            {
               ignoredLock = FALSE ;
            }
         }
      }

      if ( rc )
      {
         PD_LOG( PDERROR,
                  "Failed to get record lock, rc: %d" OSS_NEWLINE
                  "Request Mode:   %s" OSS_NEWLINE
                  "Conflict ( representative ):" OSS_NEWLINE
                  "   EDUID:  %llu" OSS_NEWLINE
                  "   TID:    %u" OSS_NEWLINE
                  "   LockId: %s" OSS_NEWLINE
                  "   Mode:   %s" OSS_NEWLINE,
                  rc,
                  lockModeToString( _recordLock ),
                  lockConflict._eduID,
                  lockConflict._tid,
                  lockConflict._lockID.toString().c_str(),
                  lockModeToString( lockConflict._lockType ) ) ;
         cb->printInfo( EDU_INFO_ERROR, "Failed to get record lock" ) ;
         goto error ;
      }

      if ( !ignoredLock )
      {
         _hasLockedRecord = TRUE ;
      }

      if ( _callback.hasError() )
      {
         rc = _callback.getResult() ;
         PD_LOG( PDERROR, "Occur error in callback, rc: %d", rc ) ;
         goto error ;
      }

      _onRecordLocked( curRID, transContext, skipRecord ) ;

   done:
      _callback.detachRecordRW() ;
      PD_TRACE_EXITRC( SDB__DMSIXSECSCAN__CHECKTRANSLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _dmsScannerLockHandler::_releaseTransLock( dmsStorageDataCommon *su,
                                                   dmsMBContext *mbContext,
                                                   const dmsRecordID &curRID,
                                                   pmdEDUCB *cb )
   {
      if ( _hasLockedRecord && curRID.isValid() )
      {
         if ( _needUnLock )
         {
            // last run have record lock held, but not trans, need to release
            // record lock
            _pTransCB->transLockRelease( cb, su->logicalID(),
                                         mbContext->mbID(), &curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
         else if ( NULL != cb &&
                   cb->getTransExecutor()->useTransLock() &&
                   _callback.getTransRecordInfo()->_transInsertDeleted )
         {
            SDB_ASSERT( !cb->isInTransRollback(), "should not be deleted by "
                        "table scan during trans rollback" ) ;
            // if the record is deleted in the same transaction, we can
            // release the lock
            // NOTE: we need to keep the IX locks on CS and CL
            _pTransCB->transLockRelease( cb, su->logicalID(),
                                         mbContext->mbID(), &curRID,
                                         &_callback, TRUE, FALSE ) ;

            _hasLockedRecord = FALSE ;
         }
      }
   }

   void _dmsScannerLockHandler::_releaseAllLocks( dmsStorageDataCommon *su,
                                                  dmsMBContext *mbContext,
                                                  const dmsRecordID &curRID,
                                                  pmdEDUCB *cb )
   {
      _releaseTransLock( su, mbContext, curRID, cb ) ;
      _releaseCSCLLock( su, mbContext, cb ) ;
   }

   /*
      _dmsSecScanner implement
    */
   _dmsSecScanner::_dmsSecScanner( dmsStorageDataCommon *su,
                                   dmsMBContext *context,
                                   rtnScanner *scanner,
                                   mthMatchRuntime *matchRuntime,
                                   DMS_ACCESS_TYPE accessType,
                                   INT64 maxRecords,
                                   INT64 skipNum,
                                   INT32 flags,
                                   IDmsOprHandler *handler )
   : _dmsScanner( su, context, matchRuntime, accessType, maxRecords, skipNum, flags, handler ),
     _dmsScannerLockHandler( handler, flags ),
     _scanner( scanner ),
     _transContext( context, scanner, accessType ),
     _scannerContext( this ),
     _isCountOnly( FALSE ),
     _firstRun( TRUE ),
     _onceRestNum( pmdGetOptionCB()->indexScanStep() ),
     _cb( NULL )
   {
   }

   _dmsSecScanner::~_dmsSecScanner()
   {
      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN_ADVANCE, "_dmsSecScanner::advance" )
   INT32 _dmsSecScanner::advance( dmsRecordID &recordID,
                                  _mthRecordGenerator &generator,
                                  pmdEDUCB *cb,
                                  _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSECSCAN_ADVANCE ) ;

      if ( _firstRun )
      {
         rc = _firstInit( cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to call first init, rc: %d", rc ) ;

         // unset first run
         _firstRun = FALSE ;
      }
      else if ( _curRID.isValid() )
      {
         _releaseTransLock( _pSu, _context, _curRID, cb ) ;
      }

      rc = _fetchNext( recordID, generator, cb, mthContext ) ;
      if ( rc )
      {
         // Do not write error log when EOC.
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Get next record failed, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      _saveAdvancedRecrodID( recordID, rc ) ;
      PD_TRACE_EXITRC( SDB__DMSSECSCAN_ADVANCE, rc ) ;
      return rc ;

   error:
      recordID.reset() ;
      goto done ;
   }

   void _dmsSecScanner::stop()
   {
      if ( _curRID.isValid() )
      {
         INT32 rc = _scanner->pauseScan() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to pause scanner, rc: %d", rc ) ;
         }
      }
      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;
      _curRID.reset() ;
   }

   void _dmsSecScanner::pause()
   {
      if ( _curRID.isValid() )
      {
         INT32 rc = _scanner->pauseScan() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to pause scanner, rc: %d", rc ) ;
         }
      }
      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;
      _context->pause() ;
      _firstRun = TRUE ;
   }

   dmsTransLockCallback* _dmsSecScanner::callbackHandler()
   {
      return &_callback ;
   }

   const dmsTransRecordInfo* _dmsSecScanner::recordInfo() const
   {
      return _callback.getTransRecordInfo() ;
   }

   BOOLEAN _dmsSecScanner::isHitEnd() const
   {
      return _scanner ? _scanner->isEOF() : TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN__FIRSTINIT, "_dmsSecScanner::_firstInit" )
   INT32 _dmsSecScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSECSCAN__FIRSTINIT ) ;

      _cb = cb ;

      _initLockInfo( _pSu, _context, _accessType, cb ) ;

      if ( cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      if ( !_context->isMBLock( _mbLockType ) )
      {
         rc = _context->mbLock( _mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }

      PD_CHECK( dmsAccessAndFlagCompatiblity( _context->mb()->_flag,
                                              _accessType ),
                SDB_DMS_INCOMPATIBLE_MODE, error, PDERROR,
                "Incompatible collection mode: %d", _context->mb()->_flag ) ;

      // set callback info
      _callback.setBaseInfo( _pTransCB, cb ) ;
      _callback.setIDInfo( _pSu->CSID(), _context->mbID(),
                           _pSu->logicalID(),
                           _context->clLID() ) ;

      // As a performance improvement, we are going to acquire the CS and
      // CL lock right in the beginning to avoid extra performance overhead
      // to acquire these locks when acquiring record lock in each step
      // We release and require the lock during pauseScan/resumeScan
      rc = _acquireCSCLLock( _pSu, _context, cb, &_transContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to acquired collection space and "
                   "collection locks, rc: %d", rc ) ;

      rc = _onFirstInit( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to call first init, rc: %d", rc ) ;

      _onceRestNum = _getOnceRestNum() ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSECSCAN__FIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSECSCAN__FETCHNEXT, "_dmsSecScanner::_fetchNext" )
   INT32 _dmsSecScanner::_fetchNext( dmsRecordID &recordID,
                                     _mthRecordGenerator &generator,
                                     _pmdEDUCB *cb,
                                     _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSECSCAN__FETCHNEXT ) ;

      BOOLEAN result = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      dmsRecordID lastRID ;

      while ( ( !isHitEnd() ) &&
              ( _onceRestNum -- > 0 ) &&
              ( 0 != _maxRecords ) )
      {
         dmsRecordID nextRID ;
         dmsRecordRW recordRW ;
         rc = _advanceScanner( cb, nextRID ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
            }
            goto error ;
         }

         if ( DPS_TRANSLOCK_MAX != _recordLock )
         {
            BOOLEAN skipRecord = FALSE ;
            _transContext.reset() ;
            rc = _checkTransLock( _pSu, _context, nextRID, cb, &_transContext,
                                  recordRW, lastRID, skipRecord ) ;
            lastRID.reset() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check transaction lock, rc: %d", rc ) ;

            if ( skipRecord )
            {
               lastRID = nextRID ;
               continue ;
            }
         }
         _curRID = nextRID ;
         lastRID = nextRID ;

         if ( recordRW.isEmpty() && !_hasLockedRecord )
         {
            BOOLEAN isSnapshotSame = FALSE ;
            rc = _checkSnapshotID( isSnapshotSame ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG( PDERROR, "Failed to check snapshot ID, rc: %d", rc ) ;
               }
               goto error ;
            }
            if ( !isSnapshotSame )
            {
               continue ;
            }
         }

         if ( !_matchRuntime && _skipNum > 0 )
         {
            if ( _skipNum > 0 )
            {
               --_skipNum ;
               continue ;
            }
            else if ( _isCountOnly )
            {
               if ( _maxRecords > 0 )
               {
                  --_maxRecords ;
               }
               recordID = _curRID ;
               recordDataPtr = 0 ;
               generator.setDataPtr( recordDataPtr ) ;
               goto done ;
            }
         }
         else
         {
            recordID = _curRID ;

            if ( recordRW.isEmpty() )
            {
               rc = _getCurrentRecord( recordData ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get record data, rc: %d", rc ) ;
            }
            else
            {
               const dmsRecord *record = recordRW.readPtr( 0 ) ;
               recordData.setData( record->getData(), record->getDataLength() ) ;
            }

            recordDataPtr = ( ossValuePtr )recordData.data() ;
            generator.setDataPtr( recordDataPtr ) ;

            // math
            if ( _matchRuntime && _matchRuntime->getMatchTree() )
            {
               result = TRUE ;
               try
               {
                  _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
                  rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
                  BSONObj obj( recordData.data() ) ;
                  //do not clear dollarlist flag
                  mthContextClearRecordInfoSafe( mthContext ) ;
                  rc = matcher->matches( obj, result, mthContext, parameters ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Failed to match record, rc: %d", rc ) ;
                     goto error ;
                  }
                  if ( result )
                  {
                     rc = generator.resetValue( obj, mthContext ) ;
                     PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;

                     if ( _skipNum > 0 )
                     {
                        if ( _skipNum >= generator.getRecordNum() )
                        {
                           _skipNum -= generator.getRecordNum() ;
                        }
                        else
                        {
                           generator.popFront( _skipNum ) ;
                           _skipNum = 0 ;
                           _checkMaxRecordsNum( generator ) ;

                           goto done ;
                        }
                     }
                     else
                     {
                        _checkMaxRecordsNum( generator ) ;
                        goto done ;
                     }
                  }
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to create BSON object, occur exception: %s",
                          e.what() ) ;
                  rc = ossException2RC( &e ) ;
                  goto error ;
               }
            }
            else
            {
               try
               {
                  BSONObj obj( recordData.data() ) ;
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
               }
               catch ( exception &e )
               {
                  PD_LOG( PDERROR, "Failed to create BSON object, occur exception: %s",
                          e.what() ) ;
                  rc = ossException2RC( &e ) ;
                  goto error ;
               }

               if ( _skipNum > 0 )
               {
                  --_skipNum ;
               }
               else
               {
                  if ( _maxRecords > 0 )
                  {
                     --_maxRecords ;
                  }
                  goto done ;
               }
            }
         }
      }

      if ( DPS_TRANSLOCK_MAX != _recordLock &&
           _hasLockedRecord &&
           lastRID.isValid() )
      {
         _pTransCB->transLockRelease( cb,
                                      _pSu->logicalID(),
                                      _context->mbID(),
                                      &lastRID,
                                      &_callback ) ;
         lastRID.reset() ;
         _hasLockedRecord = FALSE ;
      }

      // pause scanner on section EOC
      rc = _scanner->pauseScan() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause scan, rc: %d", rc ) ; ;
      rc = SDB_DMS_EOC ;
      goto error ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSECSCAN__FETCHNEXT, rc ) ;
      return rc ;

   error:
      if ( DPS_TRANSLOCK_MAX != _recordLock &&
           _hasLockedRecord &&
           lastRID.isValid() )
      {
         _pTransCB->transLockRelease( cb,
                                      _pSu->logicalID(),
                                      _context->mbID(),
                                      &lastRID,
                                      &_callback ) ;
         lastRID.reset() ;
         _hasLockedRecord = FALSE ;
      }

      _releaseAllLocks( _pSu, _context, _curRID, _cb ) ;

      recordID.reset() ;
      recordDataPtr = 0 ;
      generator.setDataPtr( recordDataPtr ) ;

      goto done ;
   }

   /*
      _dmsDataScanner implement
    */
   _dmsDataScanner::_dmsDataScanner( dmsStorageDataCommon *su,
                                     dmsMBContext *context,
                                     _rtnTBScanner *scanner,
                                     mthMatchRuntime *matchRuntime,
                                     DMS_ACCESS_TYPE accessType,
                                     INT64 maxRecords,
                                     INT64 skipNum,
                                     INT32 flags,
                                     IDmsOprHandler *opHandler )
   : _dmsSecScanner( su,
                     context,
                     scanner,
                     matchRuntime,
                     accessType,
                     maxRecords,
                     skipNum,
                     flags,
                     opHandler ),
     _scanner( scanner )
   {
      SDB_ASSERT( NULL != scanner, "scanner should not be NULL" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__ONFIRSTINIT, "_dmsDataScanner::_onFirstInit" )
   INT32 _dmsDataScanner::_onFirstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSDATASCAN__ONFIRSTINIT ) ;

      BOOLEAN isCursorSame = FALSE ;

      PD_CHECK( _scanner, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to init scanner, scanner is invalid" ) ;

      rc = _scanner->resumeScan( isCursorSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resum scanner, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__ONFIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__ONADVANCESCANNER, "_dmsDataScanner::_advanceScanner" )
   INT32 _dmsDataScanner::_advanceScanner( pmdEDUCB *cb, dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__ONADVANCESCANNER ) ;

      rc = _scanner->advance( rid ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to advance scanner, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__ONADVANCESCANNER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__CHECKSNAPSHOTID, "_dmsDataScanner::_checkSnapshotID" )
   INT32 _dmsDataScanner::_checkSnapshotID( BOOLEAN &isSnapshotSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__CHECKSNAPSHOTID ) ;

      rc = _scanner->checkSnapshotID( isSnapshotSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check snapshot ID, rc: %d", rc ) ;

      if ( !isSnapshotSame )
      {
         rc = _scanner->pauseScan() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pause scan, rc: %d", rc ) ; ;

         rc = _scanner->resumeScan( isSnapshotSame ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to resume scanner, rc: %d", rc ) ;
            }
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__CHECKSNAPSHOTID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__GETCURRID, "_dmsDataScanner::_getCurrentRID" )
   INT32 _dmsDataScanner::_getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__GETCURRID ) ;

      nextRID = _curRID ;

      PD_TRACE_EXITRC( SDB__DMSDATASCAN__GETCURRID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATASCAN__GETCURREC, "_dmsDataScanner::_getCurrentRecord" )
   INT32 _dmsDataScanner::_getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATASCAN__GETCURREC ) ;

      rc = _scanner->getCurrentRecord( recordData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSDATASCAN__GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   UINT64 _dmsDataScanner::_getOnceRestNum() const
   {
      return (UINT64)( pmdGetOptionCB()->indexScanStep() * 10 ) ;
   }

   /*
      _dmsIndexScanner implement
    */
   _dmsIndexScanner::_dmsIndexScanner( dmsStorageDataCommon *su,
                                       dmsMBContext *context,
                                       rtnIXScanner *scanner,
                                       mthMatchRuntime *matchRuntime,
                                       DMS_ACCESS_TYPE accessType,
                                       INT64 maxRecords,
                                       INT64 skipNum,
                                       INT32 flags,
                                       IDmsOprHandler *opHandler )
   : _dmsSecScanner( su,
                     context,
                     scanner,
                     matchRuntime,
                     accessType,
                     maxRecords,
                     skipNum,
                     flags,
                     opHandler ),
     _scanner( scanner )
   {
      SDB_ASSERT( NULL != scanner, "scanner should not be NULL" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__ONFIRSTINIT, "_dmsIndexScanner::_onFirstInit" )
   INT32 _dmsIndexScanner::_onFirstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSIDXSCAN__ONFIRSTINIT ) ;

      BOOLEAN isCursorSame = FALSE ;

      PD_CHECK( _scanner, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to init scanner, scanner is invalid" ) ;

      _scanner->setReadonly( isReadOnly() ) ;
      if ( DPS_TRANSLOCK_MAX == _recordLock ||
           cb->getTransExecutor()->isLockEscalated( LOCKMGR_TRANS_LOCK ) )
      {
         _scanner->disableByType( SCANNER_TYPE_MEM_TREE ) ;
      }

      rc = _scanner->resumeScan( isCursorSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resum index scanner, rc: %d", rc ) ;

      _callback.setIXScanner( _scanner ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__ONFIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__ONADVANCESCANNER, "_dmsIndexScanner::_advanceScanner" )
   INT32 _dmsIndexScanner::_advanceScanner( pmdEDUCB *cb, dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__ONADVANCESCANNER ) ;

      rc = _scanner->advance( rid ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_IXM_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to advance scanner, rc: %d", rc ) ;
         }
         else
         {
            rc = SDB_DMS_EOC ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__ONADVANCESCANNER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__CHECKSNAPSHOTID, "_dmsIndexScanner::_checkSnapshotID" )
   INT32 _dmsIndexScanner::_checkSnapshotID( BOOLEAN &isSnapshotSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__CHECKSNAPSHOTID ) ;

      rc = _scanner->checkSnapshotID( isSnapshotSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check snapshot ID, rc: %d", rc ) ;

      if ( !isSnapshotSame )
      {
         rc = _scanner->pauseScan() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pause scan, rc: %d", rc ) ; ;

         rc = _scanner->resumeScan( isSnapshotSame ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_IXM_EOC != rc )
            {
               PD_LOG( PDERROR, "Failed to resume scanner, rc: %d", rc ) ;
            }
            else
            {
               rc = SDB_DMS_EOC ;
            }
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__CHECKSNAPSHOTID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__GETCURRID, "_dmsIndexScanner::_getCurrentRID" )
   INT32 _dmsIndexScanner::_getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__GETCURRID ) ;

      nextRID = _curRID ;

      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__GETCURRID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIDXSCAN__GETCURREC, "_dmsIndexScanner::_getCurrentRecord" )
   INT32 _dmsIndexScanner::_getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIDXSCAN__GETCURREC ) ;

      if( ( _scanner->isIndexCover() ) &&
          ( DMS_IS_READ_OPR( _accessType ) ) )
      {
         const CHAR *record = _buildIndexRecord() ;
         if ( NULL != record )
         {
            dmsRecordRW recordRW ;

            recordRW = dmsIndexRecordRW( recordRW, record ) ;
            const dmsRecord *record = recordRW.readPtr( 0 ) ;
            recordData.setData( record->getData(), record->getDataLength() ) ;
            goto done ;
         }
      }

      rc = _pSu->extractData( _context, _curRID, _cb, recordData, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSIDXSCAN__GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _dmsIndexScanner::_onRecordSkipped( const dmsRecordID &curRID,
                                            dmsScanTransContext *transContext )
   {
      _scanner->removeDuplicatRID( curRID ) ;
   }

   void _dmsIndexScanner::_onRecordLocked( const dmsRecordID &curRID,
                                           dmsScanTransContext *transContext,
                                           BOOLEAN &skipRecord )
   {
      if ( !transContext->isCursorSame() || _callback.isSkipRecord() )
      {
         /// remove the duplicate key
         _scanner->removeDuplicatRID( curRID ) ;

#ifdef _DEBUG
         PD_LOG( PDDEBUG, "Cursor changed while waiting for lock, "
                 "rid(%d, %d), isCursorSame(%d), _onceRestNum(%d), "
                 "isSkipRecord(%d)",
                 curRID._extent, curRID._offset,
                 transContext->isCursorSame(), _onceRestNum,
                 _callback.isSkipRecord()) ;
#endif
         // When cursor changed, we may need to go back to previous
         // key to retry, don't count as a step. Also avoid potential
         // pause here if step becomes 0, in which case we may unexpectly
         // lose previously savedObj and savedRID and cause skip record.
         if ( !transContext->isCursorSame() )
         {
            ++ _onceRestNum ;
         }

         skipRecord = TRUE ;
      }
   }

   UINT64 _dmsIndexScanner::_getOnceRestNum() const
   {
      return (UINT64)( pmdGetOptionCB()->indexScanStep() ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXSCAN__BUILDIDINDEXRECORD, "_dmsIndexScanner::_buildIndexRecord" )
   const CHAR *_dmsIndexScanner::_buildIndexRecord()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIXSCAN__BUILDIDINDEXRECORD ) ;

      dmsRecord *pNewRecord = NULL ;
      ixmIndexCover &index = _scanner->getIndex() ;
      const BSONObj *keyValue = _scanner->getCurKeyObj() ;
      CHAR *recordPtr = NULL ;
      BSONObjIterator iter( *keyValue ) ;
      IXM_ELE_RAWDATA_ARRAY& container = index.getContainer() ;
      UINT32 extraSize = 0 ;
      UINT32 evalBufSize = 0 ;

      //1. pre caculte buf size
      rc = index.getExtraSize( extraSize ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index value extra size, rc: %d", rc ) ;

      evalBufSize = DMS_RECORD_METADATA_SZ + extraSize + keyValue->objsize() ;

      rc = index.ensureBuff( evalBufSize, recordPtr ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get index buffer [%u], rc: %d",
                   evalBufSize, rc ) ;

      try
      {
         //2. parse keyValue element to vector
         //   index node tree will find element by vector index
         rc = index.reInitContainer() ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to reserve container space, rc: %d", rc ) ;
         while( iter.more() )
         {
            rc = container.append( iter.next().rawdata() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to append index field value, rc: %d", rc ) ;
         }

         //3. reset header
         ossMemset( recordPtr, 0, DMS_RECORD_METADATA_SZ ) ;
         //4. build body(BSONObj)
         _dmsIndexCoverRecordBuilder builder( recordPtr + DMS_RECORD_METADATA_SZ ) ;
         ixmIndexNode *pTree =  NULL ;
         rc = index.getTree( pTree ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get index tree, rc: %d", rc ) ;
         if( FALSE == _dmsIndexCoverRecordBuilder::buildObj( pTree, container, builder ) )
         {
            goto done ;
         }
         builder.done() ;

         pNewRecord = (dmsRecord *)recordPtr ;
         pNewRecord->setNormal() ;
         pNewRecord->resetAttr() ;
         pNewRecord->setSize( DMS_RECORD_METADATA_SZ + builder.len() ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDWARNING, "Failed to build record, occur exception: %s",
                 e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSIXSCAN__BUILDIDINDEXRECORD, rc ) ;
      return (const CHAR *)( pNewRecord ) ;

   error:
      pNewRecord = nullptr ;
      goto done ;
   }

   /*
      _dmsExtScannerBase implement
   */
   _dmsExtScannerBase::_dmsExtScannerBase( dmsStorageDataCommon *su,
                                           dmsMBContext *context,
                                           mthMatchRuntime *matchRuntime,
                                           dmsExtentID curExtentID,
                                           dmsExtentID lastExtentID,
                                           DMS_ACCESS_TYPE accessType,
                                           INT64 maxRecords,
                                           INT64 skipNum,
                                           INT32 flags,
                                           IDmsOprHandler *handler )
   :_dmsScanner( su, context, matchRuntime, accessType, maxRecords, skipNum, flags, handler ),
    _dmsScannerLockHandler( handler, flags ),
    _curRecordPtr( NULL )
   {
      _maxRecords          = maxRecords ;
      _skipNum             = skipNum ;
      _next                = DMS_INVALID_OFFSET ;
      _firstRun            = TRUE ;
      _extent              = NULL ;
      _curRID._extent      = curExtentID ;
      _lastExtentID        = lastExtentID ;
      _cb                  = NULL ;
   }

   _dmsExtScannerBase::~_dmsExtScannerBase ()
   {
      _extent     = NULL ;

      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX &&
           _hasLockedRecord &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }

      _releaseCSCLLock( _pSu, _context, _cb ) ;
   }

   dmsTransLockCallback* _dmsExtScannerBase::callbackHandler()
   {
      return &_callback ;
   }

   const dmsTransRecordInfo* _dmsExtScannerBase::recordInfo() const
   {
      return _callback.getTransRecordInfo() ;
   }

   dmsExtentID _dmsExtScannerBase::curExtentID() const
   {
      return _curRID._extent ;
   }

   dmsExtentID _dmsExtScannerBase::nextExtentID() const
   {
      if ( _extent )
      {
         return _extent->_nextExtent ;
      }
      return DMS_INVALID_EXTENT ;
   }

   INT32 _dmsExtScannerBase::stepToNextExtent()
   {
      if ( 0 != _maxRecords &&
           DMS_INVALID_EXTENT != nextExtentID() )
      {
         _lastExtentID = _curRID._extent ;
         _curRID._extent = nextExtentID() ;
         _releaseCSCLLock( _pSu, _context, _cb ) ;
         _firstRun = TRUE ;
         return SDB_OK ;
      }
      return SDB_DMS_EOC ;
   }

   void _dmsExtScannerBase::_checkMaxRecordsNum( _mthRecordGenerator &generator )
   {
      if ( _maxRecords > 0 )
      {
         if ( _maxRecords >= generator.getRecordNum() )
         {
            _maxRecords -= generator.getRecordNum() ;
         }
         else
         {
            INT32 num = generator.getRecordNum() - _maxRecords ;
            generator.popTail( num ) ;
            _maxRecords = 0 ;
         }
      }
   }

   INT32 _dmsExtScannerBase::advance( dmsRecordID &recordID,
                                      _mthRecordGenerator &generator,
                                      pmdEDUCB *cb,
                                      _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      if ( _firstRun )
      {
         rc = _firstInit( cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "first init failed, rc: %d", rc ) ;
      }
      else if ( DMS_INVALID_OFFSET != _curRID._offset )
      {
         if ( _hasLockedRecord && _needUnLock )
         {
            // last run have record lock held, but not trans, need to release
            // record lock
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
         else if ( NULL != cb &&
                   cb->getTransExecutor()->useTransLock() &&
                   _callback.getTransRecordInfo()->_transInsertDeleted )
         {
            SDB_ASSERT( !cb->isInTransRollback(), "should not be deleted by "
                        "table scan during trans rollback" ) ;
            // if the record is deleted in the same transaction, we can
            // release the lock
            // NOTE: we need to keep the IX locks on CS and CL
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback, TRUE, FALSE ) ;

            _hasLockedRecord = FALSE ;
         }
      }

      rc = _fetchNext( recordID, generator, cb, mthContext ) ;
      if ( rc )
      {
         // Do not write error log when EOC.
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Get next record failed, rc: %d", rc ) ;
         }
         goto error ;
      }

   done:
      _saveAdvancedRecrodID( recordID, rc ) ;
      return rc ;
   error:
      recordID.reset() ;
      _curRID._offset = DMS_INVALID_OFFSET ;
      goto done ;
   }

   void _dmsExtScannerBase::stop()
   {
      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX
           && _hasLockedRecord &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      // release CSCL lock if held
      _releaseCSCLLock( _pSu, _context, _cb ) ;

      _next = DMS_INVALID_OFFSET ;
      _curRID._offset = DMS_INVALID_OFFSET ;
   }

   _dmsExtScanner::_dmsExtScanner( dmsStorageDataCommon *su,
                                   _dmsMBContext *context,
                                   mthMatchRuntime *matchRuntime,
                                   dmsExtentID curExtentID,
                                   dmsExtentID lastExtentID,
                                   DMS_ACCESS_TYPE accessType,
                                   INT64 maxRecords,
                                   INT64 skipNum,
                                   INT32 flag,
                                   IDmsOprHandler *handler )
   : _dmsExtScannerBase( su, context, matchRuntime, curExtentID, lastExtentID,
                         accessType, maxRecords, skipNum, flag, handler )
   {
   }

   _dmsExtScanner::~_dmsExtScanner()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEXTSCAN__FIRSTINIT, "_dmsExtScanner::_firstInit" )
   INT32 _dmsExtScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc          = SDB_OK ;
      SDB_BPSCB *pBPSCB = pmdGetKRCB()->getBPSCB () ;
      BOOLEAN   bPreLoadEnabled = pBPSCB->isPreLoadEnabled() ;

      PD_TRACE_ENTRY ( SDB__DMSEXTSCAN__FIRSTINIT );

      dmsTBTransContext tbTxContext( _context, NULL, _accessType ) ;

      _initLockInfo( _pSu, _context, _accessType, cb ) ;

      _extRW = _pSu->extent2RW( _curRID._extent, _context->mbID() ) ;
      _extRW.setNothrow( TRUE ) ;
      _extent = _extRW.readPtr<dmsExtent>() ;
      if ( NULL == _extent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      if ( !_context->isMBLock( _mbLockType ) )
      {
         rc = _context->mbLock( _mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }
      if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                           _accessType ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  _context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      if ( !_extent->validate( _context->mbID() ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // set callback info
      _callback.setBaseInfo( _pTransCB, cb ) ;
      _callback.setIDInfo( _pSu->CSID(), _context->mbID(),
                           _pSu->logicalID(),
                           _context->clLID() ) ;

      // send pre-load request
      if ( bPreLoadEnabled && DMS_INVALID_EXTENT != _extent->_nextExtent )
      {
         pBPSCB->sendPreLoadRequest ( bpsPreLoadReq( _pSu->CSID(),
                                                     _pSu->logicalID(),
                                                     _extent->_nextExtent )) ;
      }

      _cb   = cb ;

      // As a performance improvement, we are going to acquire the CS and
      // CL lock right in the beginning to avoid extra performance overhead
      // to acquire these locks when acquiring record lock in each step
      // We release and require the lock during pauseScan/resumeScan
      rc = _acquireCSCLLock( _pSu, _context, cb, &tbTxContext ) ;
      if ( rc )
      {
         goto error ;
      }

      // if we span different segment, we should get next extent again
      if ( _lastExtentID != DMS_INVALID_EXTENT &&
           _curRID._extent != DMS_INVALID_EXTENT &&
           _pSu->extent2Segment( _lastExtentID ) !=
           _pSu->extent2Segment( _curRID._extent ) )
      {
         dmsExtRW extRW = _pSu->extent2RW( _lastExtentID, _context->mbID() ) ;
         extRW.setNothrow( TRUE ) ;
         const dmsExtent* extent = extRW.readPtr<dmsExtent>() ;
         if ( NULL == extent )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR,
                    "Failed to read collection[%s.%s]'s extent[%d], rc: %d",
                    _pSu->getSuName(), _context->mb()->_collectionName,
                    _lastExtentID, rc ) ;
            goto error ;
         }
         if ( DMS_INVALID_EXTENT == extent->_nextExtent )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
         if ( extent->_nextExtent != _curRID._extent )
         {
            _curRID._extent = extent->_nextExtent ;
            _extRW = _pSu->extent2RW( _curRID._extent, _context->mbID() ) ;
            _extRW.setNothrow( TRUE ) ;
            _extent = _extRW.readPtr<dmsExtent>() ;
            if ( NULL == _extent )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR,
                       "Failed to read collection[%s.%s]'s extent[%d], rc: %d",
                       _pSu->getSuName(), _context->mb()->_collectionName,
                       _curRID._extent, rc ) ;
               goto error ;
            }
         }
      }

      // WARNING: once the collection has been locked eXclusively by
      //          other transaction, the first record offset may be changed
      //          by that transaction, so we should not get the first record
      //          offset before we acquired CS and CL locks
      _next = _extent->_firstRecordOffset ;

      // unset first run
      _firstRun = FALSE ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSEXTSCAN__FIRSTINIT, rc );
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSEXTSCAN__FETCHNEXT, "_dmsExtScanner::_fetchNext" )
   INT32 _dmsExtScanner::_fetchNext( dmsRecordID &recordID,
                                     _mthRecordGenerator &generator,
                                     pmdEDUCB *cb,
                                     _mthMatchTreeContext *mthContext )
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN result          = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      BOOLEAN ignoredLock     = FALSE ;

      _hasLockedRecord        = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSEXTSCAN__FETCHNEXT );
      PD_TRACE5 ( SDB__DMSEXTSCAN__FETCHNEXT,
                 PD_PACK_UINT(_needUnLock),
                 PD_PACK_UINT(_mbLockType),
                 PD_PACK_UINT(_accessType),
                 PD_PACK_UINT(_waitLock),
                 PD_PACK_UINT(_recordLock) );

      // skip extent all
      if ( !_matchRuntime && _skipNum > 0 && _skipNum >= _extent->_recCount )
      {
         _skipNum -= _extent->_recCount ;
         _next = DMS_INVALID_OFFSET ;
      }

      while ( DMS_INVALID_OFFSET != _next && 0 != _maxRecords )
      {
         _curRID._offset = _next ;
         _recordRW = _pSu->record2RW( _curRID, _context->mbID() ) ;
         _curRecordPtr = _recordRW.readPtr( 0 ) ;
         _next = _curRecordPtr->getNextOffset() ;
         ignoredLock = FALSE ;

         if ( _recordLock != DPS_TRANSLOCK_MAX )
         {
            dmsTBTransContext tbTxContext( _context, NULL, _accessType ) ;
            dpsTransRetInfo   lockConflict ;

            // attach the recordRW in callback
            _callback.attachRecordRW( &_recordRW ) ;
            _callback.clearStatus() ;

            if ( DPS_TRANSLOCK_X == _recordLock )
            {
               rc = _pTransCB->transLockGetX( cb, _pSu->logicalID(),
                                              _context->mbID(), &_curRID,
                                              & tbTxContext, &lockConflict,
                                              &_callback ) ;
            }
            else if ( DPS_TRANSLOCK_U == _recordLock )
            {
               rc = _pTransCB->transLockGetU( cb, _pSu->logicalID(),
                                              _context->mbID(), &_curRID,
                                              &tbTxContext,
                                              &lockConflict ) ;
            }
            /// DPS_TRANSLOCK_S
            else
            {
               if ( !_waitLock )
               {
                  // for new RC logic, we should first test on S lock instead
                  // of directly wait on the record lock. Under the cover,
                  // the lock call back function would try to use the old copy
                  // (previous committed version) if exist
                  rc = _pTransCB->transLockTestSPreempt( cb, _pSu->logicalID(),
                                                         _context->mbID(),
                                                         &_curRID,
                                                         &lockConflict,
                                                         &_callback,
                                                         !_CSCLLockHeld ) ;
                  ignoredLock = TRUE ;
                  if ( _callback.isSkipRecord() )
                  {
                     // For newly created records by another transaction,
                     // we could still find it through diskIXScan, we will
                     // skip those records without waiting for lock.
                     rc = SDB_OK ;
                     continue ;
                  }
                  if ( _callback.isUseOldVersion() )
                  {
                     rc = SDB_OK ;
                  }
               }

               /// wait lock
               if ( _waitLock || rc )
               {
                  // NOTE: RS and lock for share requires lock escalation
                  rc = _pTransCB->transLockGetS( cb, _pSu->logicalID(),
                                                 _context->mbID(), &_curRID,
                                                 & tbTxContext,
                                                 &lockConflict,
                                                 &_callback,
                                                 _needEscalation ) ;
                  if ( SDB_OK == rc )
                  {
                     ignoredLock = FALSE ;
                  }
               }
            }

            if ( rc )
            {
               PD_LOG( PDERROR,
                       "Failed to get record lock, rc: %d" OSS_NEWLINE
                       "Request Mode:   %s" OSS_NEWLINE
                       "Conflict ( representative ):" OSS_NEWLINE
                       "   EDUID:  %llu" OSS_NEWLINE
                       "   TID:    %u" OSS_NEWLINE
                       "   LockId: %s" OSS_NEWLINE
                       "   Mode:   %s" OSS_NEWLINE,
                       rc,
                       lockModeToString( _recordLock ),
                       lockConflict._eduID,
                       lockConflict._tid,
                       lockConflict._lockID.toString().c_str(),
                       lockModeToString( lockConflict._lockType ) ) ;
               cb->printInfo( EDU_INFO_ERROR, "Failed to get record lock" ) ;
               goto error ;
            }

            if ( !ignoredLock )
            {
               _hasLockedRecord = TRUE ;
            }

            if ( _callback.hasError() )
            {
               rc = _callback.getResult() ;
               PD_LOG( PDERROR, "Occur error in callback, rc: %d", rc ) ;
               goto error ;
            }

            // while we wait on the record lock, we could give up the mblatch,
            // another guy can come in and delete the next record (note that
            // the current record we are working on should be fine). So we
            // need to refresh the next record(_next) based on the record
            // on disk. We might or might not have record lock, but we have
            // mbLatch, so it's a valid version..
            _next = _curRecordPtr->getNextOffset() ;

            // Since we might got an old version of the record from a buffer,
            // let's the _curRecordPtr. Otherwise we might mistakenly skip
            // the record if it's being deleted.
            _curRecordPtr = _recordRW.readPtr( 0 ) ;
         }

         // if this delete is from the same transaction as marking it
         // deleting, we would end up do the delete, but how to tell?
         // If the X lock was newly acquired/granted, we consider it as a
         // new transaction, because the original one has committed and
         // original lock was released. Pass down newXAcquire.
         if ( _curRecordPtr->isDeleting() )
         {
            if ( _recordLock == DPS_TRANSLOCK_X )
            {
               INT32 rc1 = _pSu->deleteRecord( _context, _curRID,
                                               0, cb, NULL, NULL,
                                               _callback.getTransRecordInfo() ) ;
               if ( rc1 )
               {
                  PD_LOG( PDWARNING, "Failed to delete the deleting record, "
                          "rc: %d", rc ) ;
               }
            }

            if ( _hasLockedRecord )
            {
               _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                            _context->mbID(), &_curRID,
                                            &_callback ) ;
               _hasLockedRecord = FALSE ;
            }
            continue ;
         }
         SDB_ASSERT( !_curRecordPtr->isDeleted(), "record can't be deleted" ) ;

         if ( !_matchRuntime && _skipNum > 0 )
         {
            --_skipNum ;
         }
         else
         {

            recordID = _curRID ;
            rc = _pSu->extractData( _context, _recordRW, cb, recordData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Extract record data failed, rc: %d", rc ) ;
               goto error ;
            }
            recordDataPtr = ( ossValuePtr )recordData.data() ;
            generator.setDataPtr( recordDataPtr ) ;

            // math
            if ( _matchRuntime && _matchRuntime->getMatchTree() )
            {
               result = TRUE ;
               try
               {
                  _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
                  rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
                  BSONObj obj( recordData.data() ) ;
                  //do not clear dollarlist flag
                  mthContextClearRecordInfoSafe( mthContext ) ;
                  rc = matcher->matches( obj, result, mthContext, parameters ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Failed to match record, rc: %d", rc ) ;
                     goto error ;
                  }
                  if ( result )
                  {
                     rc = generator.resetValue( obj, mthContext ) ;
                     PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;

                     if ( _skipNum > 0 )
                     {
                        if ( _skipNum >= generator.getRecordNum() )
                        {
                           _skipNum -= generator.getRecordNum() ;
                        }
                        else
                        {
                           generator.popFront( _skipNum ) ;
                           _skipNum = 0 ;
                           _checkMaxRecordsNum( generator ) ;

                           goto done ;
                        }
                     }
                     else
                     {
                        _checkMaxRecordsNum( generator ) ;
                        goto done ; // find ok
                     }
                  }
               }
               catch( std::exception &e )
               {
                  PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                           e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } // if ( _match )
            else
            {
               try
               {
                  BSONObj obj( recordData.data() ) ;
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_SYS ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to create BSON object: %s",
                               e.what() ) ;
                  goto error ;
               }

               if ( _skipNum > 0 )
               {
                  --_skipNum ;
               }
               else
               {
                  if ( _maxRecords > 0 )
                  {
                     --_maxRecords ;
                  }
                  goto done ; // find ok
               }
            }
         }

         if ( _hasLockedRecord )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
      } // while

      rc = SDB_DMS_EOC ;
      goto error ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSEXTSCAN__FETCHNEXT, rc );
      return rc ;
   error:
      if ( _hasLockedRecord )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      _releaseCSCLLock( _pSu, _context, cb ) ;

      recordID.reset() ;
      recordDataPtr = 0 ;
      generator.setDataPtr( recordDataPtr ) ;
      _curRID._offset = DMS_INVALID_OFFSET ;
      goto done ;
   }

   _dmsCappedExtScanner::_dmsCappedExtScanner( dmsStorageDataCommon *su,
                                               dmsMBContext *context,
                                               mthMatchRuntime *matchRuntime,
                                               dmsExtentID curExtentID,
                                               dmsExtentID lastExtentID,
                                               DMS_ACCESS_TYPE accessType,
                                               INT64 maxRecords,
                                               INT64 skipNum,
                                               INT32 flag,
                                               IDmsOprHandler *handler )
   : _dmsExtScannerBase( su, context, matchRuntime, curExtentID, lastExtentID,
                         accessType, maxRecords, skipNum, flag, handler )
   {
      _maxRecords = maxRecords ;
      _skipNum = skipNum ;
      _extent = NULL ;
      _curRID._extent = curExtentID ;
      _next = DMS_INVALID_OFFSET ;
      _lastOffset = DMS_INVALID_OFFSET ;
      _firstRun = TRUE ;
      _cb = NULL ;
      _workExtInfo = NULL ;
      _rangeInit = FALSE ;
      _fastScanByID = FALSE ;

      /// not support transaction
      _recordLock = DPS_TRANSLOCK_MAX ;
   }

   _dmsCappedExtScanner::~_dmsCappedExtScanner()
   {
   }

   INT32 _dmsCappedExtScanner::_firstInit( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN inRange = FALSE ;

      _extRW = _pSu->extent2RW( _curRID._extent, _context->mbID() ) ;
      _extRW.setNothrow( TRUE ) ;
      _extent = _extRW.readPtr<dmsExtent>() ;
      if ( NULL == _extent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _validateRange( inRange ) ;
      PD_RC_CHECK( rc , PDERROR, "Failed to validate extant range, rc: %d",
                   rc ) ;

      if ( !inRange )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_context->isMBLock( _mbLockType ) )
      {
         rc = _context->mbLock( _mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }

      _workExtInfo =
         (( _dmsStorageDataCapped * )_pSu)->getWorkExtInfo( _context->mbID() ) ;

      // Check if we are about to scan the working extent. As the extent is very
      // big, in order to improve performance, try to avoid getting the last
      // offset again and again from the extent head when in advance.
      if ( _curRID._extent == _workExtInfo->getID() )
      {
         _next = _workExtInfo->_firstRecordOffset ;
         _lastOffset = _workExtInfo->_lastRecordOffset ;
      }
      else
      {
         _next = _extent->_firstRecordOffset ;
         _lastOffset = _extent->_lastRecordOffset ;
      }

      if ( !_extent->validate( _context->mbID() ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      _cb = cb ;
      _firstRun = FALSE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsCappedExtScanner::_fetchNext( dmsRecordID &recordID,
                                           _mthRecordGenerator &generator,
                                           pmdEDUCB *cb,
                                           _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      const dmsCappedRecord *record = NULL ;
      UINT32 currExtRecNum = 0 ;

      currExtRecNum = ( _curRID._extent == _workExtInfo->getID() ) ?
                       _workExtInfo->_recCount : _extent->_recCount ;

      if ( !_matchRuntime && _skipNum > 0 && _skipNum >= currExtRecNum &&
           ( _curRID._extent != _context->mb()->_lastExtentID ) )
      {
         _skipNum -= currExtRecNum ;
         _next = DMS_INVALID_OFFSET ;
      }

      while ( _next <= _lastOffset && DMS_INVALID_OFFSET != _next &&
              0 != _maxRecords )
      {
         _curRID._offset = _next ;
         _recordRW = _pSu->record2RW( _curRID, _context->mbID() ) ;
         _recordRW.setNothrow( TRUE ) ;
         record= _recordRW.readPtr<dmsCappedRecord>( 0 ) ;
         if ( !record )
         {
            PD_LOG( PDERROR, "Get record failed" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         _next += record->getSize() ;

         if ( !_matchRuntime && _skipNum > 0 )
         {
            --_skipNum ;
         }
         else
         {
            recordID = _curRID ;
            rc = _pSu->extractData( _context, _recordRW, cb, recordData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Extract record data failed, rc: %d", rc ) ;
               goto error ;
            }
            recordDataPtr = ( ossValuePtr )recordData.data() ;
            generator.setDataPtr( recordDataPtr ) ;

            if ( _matchRuntime && _matchRuntime->getMatchTree() )
            {
               result = TRUE ;
               try
               {
                  _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
                  rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
                  BSONObj obj( recordData.data() ) ;
                  //do not clear dollarlist flag
                  mthContextClearRecordInfoSafe( mthContext ) ;
                  rc = matcher->matches( obj, result, mthContext, parameters ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Failed to match record, rc: %d", rc ) ;
                     goto error ;
                  }
                  if ( result )
                  {
                     rc = generator.resetValue( obj, mthContext ) ;
                     PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;

                     if ( _skipNum > 0 )
                     {
                        if ( _skipNum >= generator.getRecordNum() )
                        {
                           _skipNum -= generator.getRecordNum() ;
                        }
                        else
                        {
                           generator.popFront( _skipNum ) ;
                           _skipNum = 0 ;
                           _checkMaxRecordsNum( generator ) ;

                           goto done ;
                        }
                     }
                     else
                     {
                        _checkMaxRecordsNum( generator ) ;
                        goto done ; // find ok
                     }
                  }
               }
               catch( std::exception &e )
               {
                  PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                           e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
            } // if ( _match )
            else
            {
               try
               {
                  BSONObj obj( recordData.data() ) ;
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed, rc: %d", rc ) ;
               }
               catch( std::exception &e )
               {
                  rc = SDB_SYS ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to create BSON object: %s",
                               e.what() ) ;
                  goto error ;
               }

               if ( _maxRecords > 0 )
               {
                  --_maxRecords ;
               }

               goto done ;
            }
         }
      }

      rc =  SDB_DMS_EOC ;
      goto error ;

   done:
      return rc ;
   error:
      goto done ;
   }

   OSS_INLINE dmsExtentID _dmsCappedExtScanner::_idToExtLID( INT64 id )
   {
      SDB_ASSERT( id >= 0, "id can't be negative" ) ;
      return ( id / DMS_CAP_EXTENT_BODY_SZ ) ;
   }

   // Filter out the extents that we really need to scan(in the range of the
   // condition).
   INT32 _dmsCappedExtScanner::_initFastScanRange()
   {
      INT32 rc = SDB_OK ;
      rtnPredicateSet predicateSet ;
      _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
      rtnParamList *parameters = _matchRuntime->getParametersPointer() ;

      rc = matcher->calcPredicate( predicateSet, parameters ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to calculate predicate, rc: %d",
                   rc ) ;

      {
         rtnPredicate predicate = predicateSet.predicate( DMS_ID_KEY_NAME ) ;
         if ( predicate.isGeneric() )
         {
            _fastScanByID = FALSE ;
         }
         else
         {
            _fastScanByID = TRUE ;
            // Add valid logical id range.
            BSONObj boStartKey = BSON( DMS_ID_KEY_NAME << 0 ) ;
            BSONObj boStopKey = BSON( DMS_ID_KEY_NAME << OSS_SINT64_MAX ) ;

            rc = predicateSet.addPredicate( DMS_ID_KEY_NAME,
                                            boStartKey.firstElement(),
                                            BSONObj::GTE, FALSE, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add predicate, rc: %d", rc ) ;
            rc = predicateSet.addPredicate( DMS_ID_KEY_NAME,
                                            boStopKey.firstElement(),
                                            BSONObj::LTE, FALSE, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add predicate, rc: %d", rc ) ;

            predicate = predicateSet.predicate( DMS_ID_KEY_NAME ) ;

            for ( RTN_SSKEY_LIST::iterator itr = predicate._startStopKeys.begin();
                  itr != predicate._startStopKeys.end(); ++itr )
            {
               dmsExtentID startExtLID = DMS_INVALID_EXTENT ;
               dmsExtentID endExtLID = DMS_INVALID_EXTENT ;
               const rtnKeyBoundary &startKey = itr->_startKey ;
               const rtnKeyBoundary &stopKey = itr->_stopKey ;

               startExtLID = _idToExtLID( startKey._bound.numberLong() ) ;
               endExtLID = _idToExtLID( stopKey._bound.numberLong() ) ;

               _rangeSet.insert( std::pair<dmsExtentID, dmsExtentID>( startExtLID,
                                                                      endExtLID ) ) ;
            }
         }
      }
      _rangeInit = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // As there is no index on capped table, query with condition will be slow as
   // we always need to do full table scan. But if _id is specified in the
   // condition, we can first filter out the extents which are in the range of
   // the condition. This will lead to much better performance.
   INT32 _dmsCappedExtScanner::_validateRange( BOOLEAN &inRange )
   {
      INT32 rc = SDB_OK ;

      // If there is no condition, or no _id in the condition, the validation
      // result should always be TRUE.
      inRange = TRUE ;
      if ( _matchRuntime )
      {
         if ( !_rangeInit )
         {
            rc = _initFastScanRange() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initial fast scan range, "
                         "rc: %d", rc ) ;
         }

         if ( _fastScanByID )
         {
            // Traverse the range set to check if the current extent is in any
            // valid range.
            for ( EXT_RANGE_SET_ITR itr = _rangeSet.begin();
                  itr != _rangeSet.end(); ++itr )
            {
               if ( _extent->_logicID >= itr->first &&
                    _extent->_logicID <= itr->second )
               {
                  inRange = TRUE ;
                  break ;
               }
            }
         }
         else
         {
            inRange = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   dmsExtentID _dmsCappedExtScanner::nextExtentID () const
   {
      if ( _extent )
      {
         return _extent->_nextExtent ;
      }
      return DMS_INVALID_EXTENT ;
   }

   /*
      _dmsEntireScanner implement
    */
   _dmsEntireScanner::_dmsEntireScanner( dmsStorageDataCommon *su,
                                         dmsMBContext *context,
                                         mthMatchRuntime *matchRuntime,
                                         dmsSecScanner &secScanner,
                                         rtnScanner *scanner,
                                         BOOLEAN ownedScanner,
                                         dmsScannerContext &scannerContext,
                                         DMS_ACCESS_TYPE accessType,
                                         INT64 maxRecords,
                                         INT64 skipNum,
                                         INT32 flag,
                                         IDmsOprHandler *opHandler )
   : _dmsScanner( su, context, matchRuntime, accessType, maxRecords, skipNum, flag, opHandler ),
     _secScanner( secScanner ),
     _scanner( scanner ),
     _scannerContext( scannerContext )
   {
      _firstRun      = TRUE ;
      _scanner       = scanner ;
      _ownedScanner  = ownedScanner ;

      _lockInited    = FALSE ;
      _isolation     = TRANS_ISOLATION_RU ;
      _lockType      = DPS_TRANSLOCK_MAX ;
      _lockOpMode    = DPS_TRANSLOCK_OP_MODE_ACQUIRE ;
   }

   _dmsEntireScanner::~_dmsEntireScanner()
   {
      if ( _ownedScanner )
      {
         SDB_OSS_DEL _scanner ;
         _scanner = NULL ;
      }
   }

   INT32 _dmsEntireScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;

      if ( _lockInited )
      {
         _secScanner.initLockInfo( _isolation, _lockType, _lockOpMode ) ;
      }

      if ( !_context->isMBLock( _secScanner.getMBLockType() ) )
      {
         rc = _context->mbLock( _secScanner.getMBLockType() ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }

      rc = _onInit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init scanner, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsEntireScanner::_pauseInnerScanner()
   {
      _secScanner.pause() ;
   }

   INT32 _dmsEntireScanner::advance( dmsRecordID &recordID,
                                     _mthRecordGenerator &generator,
                                     pmdEDUCB *cb,
                                     _mthMatchTreeContext *mthContext )
   {
      INT32 rc = SDB_OK ;

      if ( _firstRun )
      {
         rc = _firstInit() ;
         PD_RC_CHECK( rc, PDERROR, "First init failed, rc: %d", rc ) ;
         _firstRun = FALSE ;
      }

      while ( TRUE )
      {
         rc = _secScanner.advance( recordID, generator, cb, mthContext ) ;
         if ( SDB_DMS_EOC == rc )
         {
            if ( _secScanner.isHitEnd() )
            {
               goto error ;
            }

            // just pause
            _pauseInnerScanner() ;

            rc = SDB_OK ;
            continue ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to advance scanner, rc: %d", rc ) ;
            goto error ;
         }

         break ;
      }

   done:
      _saveAdvancedRecrodID( recordID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _dmsEntireScanner::stop()
   {
      _secScanner.stop() ;
   }

   /*
      _dmsTBScanner implement
   */
   _dmsTBScanner::_dmsTBScanner( dmsStorageDataCommon *su,
                                 dmsMBContext *context,
                                 mthMatchRuntime *matchRuntime,
                                 rtnTBScanner *scanner,
                                 BOOLEAN ownedScanner,
                                 DMS_ACCESS_TYPE accessType,
                                 INT64 maxRecords,
                                 INT64 skipNum,
                                 INT32 flag,
                                 IDmsOprHandler *opHandler )
   : _dmsEntireScanner( su, context, matchRuntime, _secScanner, scanner,
                        ownedScanner, _scannerContext, accessType, maxRecords,
                        skipNum, flag, opHandler ),
     _secScanner( su, context, scanner, matchRuntime, accessType, maxRecords,
                  skipNum, flag, opHandler ),
     _scannerContext( &_secScanner )
   {
   }

   INT32 _dmsTBScanner::_onInit()
   {
      INT32 rc = SDB_OK ;

      _context->mbStat()->_crudCB.increaseTbScan( 1 ) ;

      return rc ;
   }

   class _dmsIXSecScanner::_SimpleBSONBuilder
   {
      public:
         _SimpleBSONBuilder( CHAR *pBuf )
         {
            _pBuf    = pBuf ;
            _pCur    = _pBuf ;
            _ppPos   = NULL ;
            _hasDone = FALSE ;

            init() ;
         }

         _SimpleBSONBuilder( CHAR **ppPos )
         {
            _pBuf    = *ppPos ;
            _pCur    = _pBuf ;
            _ppPos   = ppPos ;
            _hasDone = FALSE ;

            init() ;
         }
         ~_SimpleBSONBuilder()
         {
            done() ;
         }

         BOOLEAN isEmpty() const
         {
            return len() <= 5 ? TRUE : FALSE ;
         }

         UINT32 len() const { return _pCur - _pBuf ; }

         const CHAR* done()
         {
            if ( !_hasDone )
            {
               _hasDone = TRUE ;
               *_pCur = (CHAR) EOO ;
               ++_pCur ;
               /// set size
               *((UINT32*)_pBuf) = _pCur - _pBuf ;
               /// set pos
               if ( _ppPos )
               {
                  *_ppPos = _pCur ;
               }
            }
            return _pBuf ;
         }

         _SimpleBSONBuilder* appendElement( BSONElement &ele )
         {
            ossMemcpy( _pCur, ele.rawdata(), ele.size() ) ;
            _pCur += ele.size() ;
            return this ;
         }

         _SimpleBSONBuilder* appendAs( const BSONElement& e, const StringData &fieldName )
         {
            *_pCur = (CHAR) e.type() ;
            _pCur += 1 ;

            INT32 len ;
            len = fieldName.size() ;
            ossMemcpy( _pCur, fieldName.data(), len ) ;
            _pCur += len ;
            *_pCur = 0 ;
            _pCur += 1 ;

            len = e.valuesize() ;
            ossMemcpy( _pCur, (void *) e.value(), len );
            _pCur += len ;

            return this ;
         }

         CHAR** subobjStart( const StringData &fieldName )
         {
            *_pCur = (CHAR) Object ;
            _pCur += 1 ;

            const INT32 len = fieldName.size() ;
            ossMemcpy( _pCur, fieldName.data(), len ) ;
            _pCur += len ;
            *_pCur = 0 ;
            _pCur += 1 ;

            return &_pCur ;
         }

         void abortSubobj( const StringData &fieldName, _SimpleBSONBuilder& sub )
         {
            _pCur -= 1 ;
            const INT32 len = fieldName.size() + 1 ;
            _pCur -= len ;
            _pCur -= sub.len() ;
         }

      protected:
         void init()
         {
            *((UINT32*)_pBuf) = 0 ;
            _pCur = _pBuf + 4 ;
         }

      private:
         CHAR*       _pBuf ;
         CHAR*       _pCur ;
         CHAR**      _ppPos ;
         BOOLEAN     _hasDone ;
   };

   /*
      _dmsIXSecScanner implement
   */
   _dmsIXSecScanner::_dmsIXSecScanner( dmsStorageDataCommon *su,
                                       dmsMBContext *context,
                                       mthMatchRuntime *matchRuntime,
                                       rtnIXScanner *scanner,
                                       DMS_ACCESS_TYPE accessType,
                                       INT64 maxRecords,
                                       INT64 skipNum,
                                       INT32 flag,
                                       IDmsOprHandler *opHandler )
   :_dmsScanner( su, context, matchRuntime, accessType, maxRecords, skipNum, flag, opHandler ),
    _dmsScannerLockHandler( opHandler, flag ),
    _curRecordPtr( NULL )
   {
      _maxRecords          = maxRecords ;
      _skipNum             = skipNum ;
      _firstRun            = TRUE ;
      _cb                  = NULL ;
      _scanner             = scanner ;
      _onceRestNum         = 0 ;
      _eof                 = FALSE ;
      _indexBlockScan      = FALSE ;
      _judgeStartKey       = FALSE ;
      _includeStartKey     = FALSE ;
      _includeEndKey       = FALSE ;
      _blockScanDir        = 1 ;
      _countOnly           = FALSE ;

      // lock for update has higher priority
      if ( OSS_BIT_TEST( flag, FLG_QUERY_FOR_UPDATE ) )
      {
         _selectLockMode = DPS_TRANSLOCK_U ;
      }
      else if ( OSS_BIT_TEST( flag, FLG_QUERY_FOR_SHARE ) )
      {
         _selectLockMode = DPS_TRANSLOCK_S ;
      }
   }

   _dmsIXSecScanner::~_dmsIXSecScanner ()
   {
      release() ;
   }

   void _dmsIXSecScanner::release()
   {
      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX
           && _hasLockedRecord
           && DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }

      _releaseCSCLLock( _pSu, _context, _cb ) ;

      _scanner = NULL ;
   }

   dmsTransLockCallback* _dmsIXSecScanner::callbackHandler()
   {
      return &_callback ;
   }

   const dmsTransRecordInfo* _dmsIXSecScanner::recordInfo() const
   {
      return _callback.getTransRecordInfo() ;
   }

   void  _dmsIXSecScanner::enableIndexBlockScan( const BSONObj &startKey,
                                                 const BSONObj &endKey,
                                                 const dmsRecordID &startRID,
                                                 const dmsRecordID &endRID,
                                                 INT32 direction )
   {
      SDB_ASSERT( _scanner, "Scanner can't be NULL" ) ;

      _blockScanDir     = direction ;
      _indexBlockScan   = TRUE ;
      _judgeStartKey    = FALSE ;
      _includeStartKey  = FALSE ;
      _includeEndKey    = FALSE ;

      _startKey = startKey.getOwned() ;
      _endKey = endKey.getOwned() ;
      _startRID = startRID ;
      _endRID = endRID ;

      BSONObj startObj = _scanner->getPredicateList()->startKey() ;
      BSONObj endObj = _scanner->getPredicateList()->endKey() ;

      if ( 0 == startObj.woCompare( *_getStartKey(), BSONObj(), false ) &&
           _getStartRID()->isNull() )
      {
         _includeStartKey = TRUE ;
      }
      if ( 0 == endObj.woCompare( *_getEndKey(), BSONObj(), false ) &&
           _getEndRID()->isNull() )
      {
         _includeEndKey = TRUE ;
      }

      // reset start/end RID
      if ( _getStartRID()->isNull() )
      {
         if ( 1 == _scanner->getDirection() )
         {
            _getStartRID()->resetMin () ;
         }
         else
         {
            _getStartRID()->resetMax () ;
         }
      }
      if ( _getEndRID()->isNull() )
      {
         if ( !_includeEndKey )
         {
            if ( 1 == _scanner->getDirection() )
            {
               _getEndRID()->resetMin () ;
            }
            else
            {
               _getEndRID()->resetMax () ;
            }
         }
         else
         {
            if ( 1 == _scanner->getDirection() )
            {
               _getEndRID()->resetMax () ;
            }
            else
            {
               _getEndRID()->resetMin () ;
            }
         }
      }
   }

   INT32 _dmsIXSecScanner::_firstInit( pmdEDUCB * cb )
   {
      INT32 rc          = SDB_OK ;

      BOOLEAN isCursorSame = FALSE ;
      dmsIXTransContext ixTxContext( _context, _scanner, _accessType ) ;

      _initLockInfo( _pSu, _context, _accessType, cb ) ;

      if ( NULL == _scanner )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _scanner->setReadonly( isReadOnly() ) ;
      if ( DPS_TRANSLOCK_MAX == _recordLock ||
           cb->getTransExecutor()->isLockEscalated( LOCKMGR_TRANS_LOCK ) )
      {
         _scanner->disableByType( SCANNER_TYPE_MEM_TREE ) ;
      }
      if ( cb && cb->isInterrupted() )
      {
         rc = SDB_APP_INTERRUPT ;
         goto error ;
      }
      if ( !_context->isMBLock( _mbLockType ) )
      {
         rc = _context->mbLock( _mbLockType ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;
      }
      if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                           _accessType ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  _context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      rc = _scanner->resumeScan( isCursorSame ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resum ixscan, rc: %d", rc ) ;
      _cb = cb ;

      // As a performance improvement, we are going to acquire the CS and
      // CL lock right in the beginning to avoid extra performance overhead
      // to acquire these locks when acquiring record lock in each step
      // We release and require the lock during pauseScan/resumeScan
      rc = _acquireCSCLLock( _pSu, _context, cb, &ixTxContext ) ;
      if ( rc )
      {
         goto error ;
      }

      /// set callback info
      _callback.setBaseInfo( _pTransCB, cb ) ;
      _callback.setIDInfo( _pSu->CSID(), _context->mbID(),
                           _pSu->logicalID(),
                           _context->clLID() ) ;
      _callback.setIXScanner( _scanner ) ;

      // unset first run
      _firstRun = FALSE ;
      _onceRestNum = (INT64)pmdGetKRCB()->getOptionCB()->indexScanStep() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BSONObj* _dmsIXSecScanner::_getStartKey ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_startKey : &_endKey ;
   }

   BSONObj* _dmsIXSecScanner::_getEndKey ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_endKey : &_startKey ;
   }

   dmsRecordID* _dmsIXSecScanner::_getStartRID ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_startRID : &_endRID ;
   }

   dmsRecordID* _dmsIXSecScanner::_getEndRID ()
   {
      return _scanner->getDirection() == _blockScanDir ? &_endRID : &_startRID ;
   }

   void _dmsIXSecScanner::_updateMaxRecordsNum( _mthRecordGenerator &generator )
   {
      if ( _maxRecords > 0 )
      {
         if ( _maxRecords >= generator.getRecordNum() )
         {
            _maxRecords -= generator.getRecordNum() ;
         }
         else
         {
            INT32 num = generator.getRecordNum() - _maxRecords ;
            generator.popTail( num ) ;
            _maxRecords = 0 ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXSECSCAN__CHECKTRANSLOCK, "_dmsIXSecScanner::_checkTransLock" )
   INT32 _dmsIXSecScanner::_checkTransLock( pmdEDUCB *cb,
                                            dmsRecordID &waitUnlockRID,
                                            BOOLEAN &skipRecord )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSIXSECSCAN__CHECKTRANSLOCK ) ;

      BOOLEAN ignoredLock = FALSE ;

      // If need lock and the RID was not setup from the in memory tree.
      // As an optimization, if the RID was originally found through in
      // memory tree scan, don't try lock at all.
      if ( _recordLock != DPS_TRANSLOCK_MAX )
      {
         dpsTransRetInfo   lockConflict ;
         dmsIXTransContext ixTxContext( _context, _scanner, _accessType ) ;

         /// already locked, but not the same, should release lock first
         if ( waitUnlockRID.isValid() && _curRID != waitUnlockRID )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &waitUnlockRID,
                                         &_callback ) ;
            waitUnlockRID.reset() ;
         }

         // attach the recordRW in callback
         _callback.attachRecordRW( &_recordRW ) ;
         _callback.clearStatus() ;

         if ( DPS_TRANSLOCK_X == _recordLock )
         {
            // exclusive lock has to always wait on the lock
            rc = _pTransCB->transLockGetX( cb, _pSu->logicalID(),
                                           _context->mbID(), &_curRID,
                                           &ixTxContext,
                                           &lockConflict, &_callback ) ;
         }
         else if ( DPS_TRANSLOCK_U == _recordLock )
         {
            rc = _pTransCB->transLockGetU( cb, _pSu->logicalID(),
                                           _context->mbID(), &_curRID,
                                           &ixTxContext,
                                           &lockConflict, &_callback ) ;
         }
         // DPS_TRANSLOCK_S
         else
         {
            if ( !_waitLock )
            {
               // for new RC logic, we should first test on S lock instead
               // of directly wait on the record lock. Under the cover,
               // the lock call back function would try to use the old copy
               // (previous committed version) if exist
               rc = _pTransCB->transLockTestSPreempt( cb, _pSu->logicalID(),
                                                      _context->mbID(),
                                                      &_curRID,
                                                      &lockConflict,
                                                      &_callback,
                                                      !_CSCLLockHeld ) ;
               ignoredLock = TRUE ;
               if ( _callback.isSkipRecord() )
               {
                  // For newly created records by another transaction,
                  // we could still find it through diskIXScan, we will
                  // skip those records without waiting for lock.
                  _scanner->removeDuplicatRID( _curRID ) ;
                  rc = SDB_OK ;
                  skipRecord = TRUE ;
                  goto done ;
               }
               if ( _callback.isUseOldVersion() )
               {
                  rc = SDB_OK ;
               }
            }

            /// wait lock
            if ( _waitLock || rc )
            {
               // test S lock failed and the record is not in old version
               // container nor in RBS. most likely the one hold / wait X
               // hasn't finish updating the record.
               // NOTE: RS and lock for share requires lock escalation
               rc = _pTransCB->transLockGetS( cb, _pSu->logicalID(),
                                              _context->mbID(), &_curRID,
                                              &ixTxContext,
                                              &lockConflict,
                                              &_callback,
                                              _needEscalation ) ;
               if ( SDB_OK == rc )
               {
                  ignoredLock = FALSE ;
               }
            }
         }

         if ( waitUnlockRID.isValid() )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &waitUnlockRID,
                                         &_callback ) ;
            waitUnlockRID.reset() ;
         }

         if ( rc )
         {
            PD_LOG( PDERROR,
                    "Failed to get record lock, rc: %d" OSS_NEWLINE
                    "Request Mode:   %s" OSS_NEWLINE
                    "Conflict ( representative ):" OSS_NEWLINE
                    "   EDUID:  %llu" OSS_NEWLINE
                    "   TID:    %u" OSS_NEWLINE
                    "   LockId: %s" OSS_NEWLINE
                    "   Mode:   %s" OSS_NEWLINE,
                    rc,
                    lockModeToString( _recordLock ),
                    lockConflict._eduID,
                    lockConflict._tid,
                    lockConflict._lockID.toString().c_str(),
                    lockModeToString( lockConflict._lockType ) ) ;
            cb->printInfo( EDU_INFO_ERROR, "Failed to get record lock" ) ;
            goto error ;
         }

         if ( !ignoredLock )
         {
            _hasLockedRecord = TRUE ;
         }

         if ( _callback.hasError() )
         {
            rc = _callback.getResult() ;
            PD_LOG( PDERROR, "Occur error in callback, rc: %d", rc ) ;
            goto error ;
         }

         // index has changed under us between wait on lock and got lock
         if ( !ixTxContext.isCursorSame() || _callback.isSkipRecord() )
         {
            if ( _hasLockedRecord )
            {
               waitUnlockRID = _curRID ;
               _hasLockedRecord = FALSE ;
            }

            /// remove the duplicate key
            _scanner->removeDuplicatRID( _curRID ) ;

#ifdef _DEBUG
            PD_LOG( PDDEBUG, "Cursor changed while waiting for lock, "
                    "rid(%d, %d), isCursorSame(%d), _onceRestNum(%d), "
                    "isSkipRecord(%d)",
                    _curRID._extent, _curRID._offset,
                    ixTxContext.isCursorSame(), _onceRestNum,
                    _callback.isSkipRecord()) ;
#endif
            // When cursor changed, we may need to go back to previous
            // key to retry, don't count as a step. Also avoid potential
            // pause here if step becomes 0, in which case we may unexpectly
            // lose previously savedObj and savedRID and cause skip record.
            if ( !ixTxContext.isCursorSame() )
            {
               _onceRestNum++ ;
            }

            skipRecord = TRUE ;
            goto done ;
         }
      } // end of (_recordLock != -1)

   done:
      PD_TRACE_EXITRC( SDB__DMSIXSECSCAN__CHECKTRANSLOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // node tree:      a
   //                 |
   //            b(1) c(2) d(EOO)
   // then builder obj is : a{b:1,c:2}
   BOOLEAN _dmsIXSecScanner::_buildObj( ixmIndexNode *node,
                                        IXM_ELE_RAWDATA_ARRAY& value,
                                        SimpleBSONBuilder& builder )
   {
      IXM_INDEX_NODE_PTR_ARRAY &children = node->getChildren() ;
      BOOLEAN finished =  FALSE ;

      try
      {
         for ( UINT32 i = 0; i < node->childrenSize(); i++ )
         {
            if( 0 == children[i]->childrenSize() )
            {
               UINT32 fieldIndex = children[i]->getFieldIndex() ;
               SDB_ASSERT( fieldIndex < value.size(), "Field index bigger than field size" ) ;

               BSONElement ele( value[ fieldIndex ] ) ;

               if( Undefined != ele.type() )
               {
                  builder.appendAs( ele, children[i]->getName() );
               }
               else if( children[i]->isEmbedded() )
               {
                  // if index fields is {"a.b":1,c:1},insert {a:10,c:10}
                  // key value is {"":{"Undefined":1},"c":10}
                  // dms value is {a:10,c:10}
                  // not the same so we should to read dms value again
                  goto done ;
               }
            }
            else
            {
               SimpleBSONBuilder sub( builder.subobjStart( children[i]->getName() ) ) ;
               if( FALSE == _buildObj( children[i], value, sub ) )
               {
                  goto done ;
               }
               sub.done() ;
               if( sub.isEmpty() )
               {
                  builder.abortSubobj( children[i]->getName(), sub ) ;
               }
            }
         }
         finished = TRUE ;
      }
      catch ( std::exception &e )
      {
         INT32 rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDWARNING, "Build index value object, Occur exception: %s", e.what() ) ;
      }
   done:
      return finished ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXSECSCAN_BUILDIDINDEXRECORD, "_dmsIXSecScanner::_buildIndexRecord" )
   const CHAR* _dmsIXSecScanner::_buildIndexRecord()
   {
      INT32 rc = SDB_OK ;
      dmsRecord *pNewRecord = NULL ;
      ixmIndexCover &index = _scanner->getIndex() ;
      const BSONObj* keyValue = _scanner->getCurKeyObj() ;
      CHAR* recordPtr = NULL ;
      BSONObjIterator iter( *keyValue ) ;
      IXM_ELE_RAWDATA_ARRAY& container = index.getContainer() ;
      UINT32 extraSize = 0 ;
      UINT32 evalBufSize = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSIXSECSCAN_BUILDIDINDEXRECORD );

      //1. pre caculte buf size
      rc = index.getExtraSize( extraSize ) ;
      PD_RC_CHECK( rc, PDWARNING, "Get index value extra size faield, rc: %d", rc ) ;

      evalBufSize = DMS_RECORD_METADATA_SZ + extraSize + keyValue->objsize() ;

      rc = index.ensureBuff( evalBufSize, recordPtr ) ;
      PD_RC_CHECK( rc, PDWARNING, "Get index buffer failed, rc: %d", evalBufSize, rc ) ;

      try
      {
         //2. parse keyValue element to vector
         //   index node tree will find element by vector index
         rc = index.reInitContainer() ;
         PD_RC_CHECK( rc, PDWARNING, "Reserve container space failed, rc: %d", rc ) ;
         while( iter.more() )
         {
            rc = container.append( iter.next().rawdata() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Append index field value failed, rc: %d", rc ) ;
         }

         //3. reset header
         ossMemset( recordPtr, 0, DMS_RECORD_METADATA_SZ ) ;
         //4. build body(BSONObj)
         SimpleBSONBuilder builder( recordPtr + DMS_RECORD_METADATA_SZ ) ;
         ixmIndexNode *pTree =  NULL ;
         rc = index.getTree( pTree ) ;
         PD_RC_CHECK( rc, PDWARNING, "Get index tree failed, rc: %d", rc ) ;
         if( FALSE == _buildObj( pTree, container, builder ) )
         {
            goto done ;
         }
         builder.done() ;

         pNewRecord = ( dmsRecord* )recordPtr ;
         pNewRecord->setNormal() ;
         pNewRecord->resetAttr() ;
         pNewRecord->setSize( DMS_RECORD_METADATA_SZ + builder.len() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDWARNING, "Build index record, Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSIXSECSCAN_BUILDIDINDEXRECORD, rc );
      if( SDB_OK == rc )
      {
         return ( const CHAR* )pNewRecord ;
      }
      else
      {
         return NULL ;
      }
   error:
      goto done ;
   }

   // Description
   //    Index section scan. Advance to next index entry based on the on-disk
   // index tree and searching criteria, return the found recordID pointed
   // by the index
   // Input
   //
   // Ouput
   //    recordID:
   //    On normal return, record lock is held on the matching record.
   // Return
   //
   //
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSIXSECSCAN_ADVANCE, "_dmsIXSecScanner::advance" )
   INT32 _dmsIXSecScanner::advance( dmsRecordID &recordID,
                                    _mthRecordGenerator &generator,
                                    pmdEDUCB * cb,
                                    _mthMatchTreeContext *mthContext )
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN result          = TRUE ;
      ossValuePtr recordDataPtr ;
      dmsRecordData recordData ;
      dmsRecordID waitUnlockRID ;
      BOOLEAN skipRecord      = FALSE ;
      const CHAR* pRecord = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSIXSECSCAN_ADVANCE );

      PD_TRACE5( SDB__DMSIXSECSCAN_ADVANCE,
                 PD_PACK_UINT(_needUnLock),
                 PD_PACK_UINT(_mbLockType),
                 PD_PACK_UINT(_accessType),
                 PD_PACK_UINT(_waitLock),
                 PD_PACK_UINT(_recordLock) );

      if ( _firstRun )
      {
         rc = _firstInit( cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "first init failed, rc: %d", rc ) ;
      }
      else if ( DMS_INVALID_OFFSET != _curRID._offset )
      {
         if ( _hasLockedRecord && _needUnLock )
         {
            // last run have record lock held, but not trans, need to release
            // record lock
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
         else if ( NULL != cb &&
                   cb->getTransExecutor()->useTransLock() &&
                   _callback.getTransRecordInfo()->_transInsertDeleted )
         {
            // if the record is deleted in the same transaction, we can
            // release the lock
            // NOTE: we need to keep the IX locks on CS and CL
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback, TRUE, FALSE ) ;

            _hasLockedRecord = FALSE ;
         }
      }

      _hasLockedRecord = FALSE ;
      while ( _onceRestNum-- > 0 && 0 != _maxRecords )
      {
         skipRecord = FALSE ;

         // advance index tree
         rc = _scanner->advance( _curRID ) ;
         if ( SDB_IXM_EOC == rc )
         {
            _eof = TRUE ;
            rc = SDB_DMS_EOC ;
            break ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "IXScanner advance failed, rc: %d", rc ) ;
            goto error ;
         }
         SDB_ASSERT( _curRID.isValid(), "rid must be valid" ) ;

         // index block scan
         if ( _indexBlockScan )
         {
            INT32 result = 0 ;
            if ( !_judgeStartKey )
            {
               _judgeStartKey = TRUE ;

               result = _scanner->compareWithCurKeyObj( *_getStartKey() ) ;
               if ( 0 == result )
               {
                  result = _curRID.compare( *_getStartRID() ) *
                           _scanner->getDirection() ;
               }
               if ( result < 0 )
               {
                  // need to relocate
                  rc = _scanner->relocateRID( *_getStartKey(),
                                              *_getStartRID() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to relocateRID, rc: %d",
                               rc ) ;
                  continue ;
               }
            }

            result = _scanner->compareWithCurKeyObj( *_getEndKey() ) ;
            if ( 0 == result )
            {
               result = _curRID.compare( *_getEndRID() ) *
                        _scanner->getDirection() ;
            }
            if ( result > 0 || ( !_includeEndKey && result == 0 ) )
            {
               _eof = TRUE ;
               rc = SDB_DMS_EOC ;
               break ;
            }
         }

         // don't read data
         if ( !_matchRuntime )
         {
            if ( _skipNum > 0 )
            {
               --_skipNum ;
               continue ;
            }
            else if ( _countOnly )
            {
               if ( cb->isTransaction() && !cb->isTransRU() )
               {
                  // no need to read record
                  // look for transaction lock
                  rc = _checkTransLock( cb, waitUnlockRID, skipRecord ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to check transaction lock, "
                               "rc: %d", rc ) ;
                  if ( skipRecord )
                  {
                     continue ;
                  }
               }
               if ( _maxRecords > 0 )
               {
                  --_maxRecords ;
               }
               recordID = _curRID ;
               recordDataPtr = 0 ;
               generator.setDataPtr( recordDataPtr ) ;
               goto done ;
            }
         }

         // read record for further process
         // record2RW already take care of in memory version vs on disk
         // version under the cover
         // _recordRW = _pSu->record2RW( _curRID, _context->mbID() ) ;

         // look for transaction lock
         rc = _checkTransLock( cb, waitUnlockRID, skipRecord ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check transaction lock, "
                      "rc: %d", rc ) ;
         if ( skipRecord )
         {
            continue ;
         }

         pRecord = NULL ;

         if( _scanner->isIndexCover() &&
             !_recordRW.isDirectMem() &&
             DMS_IS_READ_OPR( _accessType ) )
         {
            pRecord = _buildIndexRecord() ;
            if( NULL != pRecord )
            {
               _recordRW = dmsIndexRecordRW( _recordRW, pRecord ) ;
            }
         }

         recordID = _curRID ;
         rc = _pSu->extractData( _context, recordID, cb, recordData, !pRecord ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Extract record data failed, rc: %d", rc ) ;
            goto error ;
         }
         recordDataPtr = ( ossValuePtr )recordData.data() ;
         generator.setDataPtr( recordDataPtr ) ;

         // match
         if ( _matchRuntime && _matchRuntime->getMatchTree() )
         {
            result = TRUE ;
            try
            {
               _mthMatchTree *matcher = _matchRuntime->getMatchTree() ;
               rtnParamList *parameters = _matchRuntime->getParametersPointer() ;
               BSONObj obj ( recordData.data() ) ;
               //do not clear dollarlist flag
               mthContextClearRecordInfoSafe( mthContext ) ;
               rc = matcher->matches( obj, result, mthContext, parameters ) ;
               if ( rc )
               {
                  if ( SDB_IXM_ADVANCE_EOC == rc )
                  {
                     goto done ;
                  }
                  PD_LOG( PDERROR, "Failed to match record, rc: %d", rc ) ;
                  goto error ;
               }
               if ( result )
               {
                  rc = generator.resetValue( obj, mthContext ) ;
                  PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
                  if ( _skipNum > 0 )
                  {
                     if ( _skipNum >= generator.getRecordNum() )
                     {
                        _skipNum -= generator.getRecordNum() ;
                     }
                     else
                     {
                        generator.popFront( _skipNum ) ;
                        _skipNum = 0 ;
                        _updateMaxRecordsNum( generator ) ;
                        goto done ;
                     }
                  }
                  else
                  {
                     _updateMaxRecordsNum( generator ) ;
                     goto done ; // find ok
                  }
               }
            }
            catch( std::exception &e )
            {
               PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                        e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         } // if ( _match )
         else
         {
            try
            {
               BSONObj obj( recordData.data() ) ;
               rc = generator.resetValue( obj, mthContext ) ;
               PD_RC_CHECK( rc, PDERROR, "resetValue failed:rc=%d", rc ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_RC_CHECK( rc, PDERROR, "Failed to create BSON object: %s",
                            e.what() ) ;
               goto error ;
            }

            if ( _skipNum > 0 )
            {
               --_skipNum ;
            }
            else
            {
               if ( _maxRecords > 0 )
               {
                  --_maxRecords ;
               }
               goto done ; // find ok
            }
         }

         // Proper found case will jump to done, if we got here, there was
         // either unmatch, or we need to skip. Either way, we should
         // release the record lock before advance to next index
         if ( _hasLockedRecord )
         {
            _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                         _context->mbID(), &_curRID,
                                         &_callback ) ;
            _hasLockedRecord = FALSE ;
         }
      } // while

      rc = SDB_DMS_EOC ;
      {
         INT32 rcTmp = _scanner->pauseScan() ;
         if ( rcTmp )
         {
            PD_LOG( PDERROR, "Pause scan failed, rc: %d", rcTmp ) ;
            rc = rcTmp ;
         }
         // release CS/CL lock when we are done
         _releaseCSCLLock( _pSu, _context, cb ) ;
      }
      goto error ;

   done:
      if ( waitUnlockRID.isValid() )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(),
                                      _context->mbID(), &waitUnlockRID,
                                      &_callback ) ;
      }

      _saveAdvancedRecrodID( recordID, rc ) ;
      PD_TRACE_EXITRC ( SDB__DMSIXSECSCAN_ADVANCE, rc ) ;
      return rc ;
   error:
      if ( _hasLockedRecord && _recordLock != DPS_TRANSLOCK_MAX )
      {
         _pTransCB->transLockRelease( cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      _releaseCSCLLock( _pSu, _context, cb ) ;
      recordID.reset() ;
      recordDataPtr = 0 ;
      generator.setDataPtr( recordDataPtr ) ;
      _curRID._offset = DMS_INVALID_OFFSET ;
      goto done ;
   }

   void _dmsIXSecScanner::stop ()
   {
      if ( FALSE == _firstRun && _recordLock != DPS_TRANSLOCK_MAX
           && _hasLockedRecord &&
           DMS_INVALID_OFFSET != _curRID._offset )
      {
         _pTransCB->transLockRelease( _cb, _pSu->logicalID(), _context->mbID(),
                                      &_curRID, &_callback ) ;
         _hasLockedRecord = FALSE ;
      }
      if ( DMS_INVALID_OFFSET != _curRID._offset )
      {
         INT32 rc = _scanner->pauseScan() ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Pause scan failed, rc: %d", rc ) ;
         }
      }
      _releaseCSCLLock( _pSu, _context, _cb ) ;
      _curRID._offset = DMS_INVALID_OFFSET ;
   }

   /*
      _dmsIXScanner implement
    */
   _dmsIXScanner::_dmsIXScanner( dmsStorageDataCommon *su,
                                 dmsMBContext *context,
                                 mthMatchRuntime *matchRuntime,
                                 rtnIXScanner *scanner,
                                 BOOLEAN ownedScanner,
                                 DMS_ACCESS_TYPE accessType,
                                 INT64 maxRecords,
                                 INT64 skipNum,
                                 INT32 flag,
                                 IDmsOprHandler *opHandler )
   : _dmsEntireScanner( su, context, matchRuntime, _secScanner, scanner,
                        ownedScanner, _scannerContext, accessType, maxRecords, skipNum, flag,
                        opHandler ),
     _secScanner( su, context, scanner, matchRuntime, accessType, maxRecords,
                  skipNum, flag, opHandler ),
     _scannerContext( &_secScanner )
   {
   }

   INT32 _dmsIXScanner::_onInit()
   {
      INT32 rc = SDB_OK ;

      _context->mbStat()->_crudCB.increaseIxScan( 1 ) ;

      return rc ;
   }

   /*
      _dmsExtentItr implement
   */
   _dmsExtentItr::_dmsExtentItr( dmsStorageData *su, dmsMBContext * context,
                                 DMS_ACCESS_TYPE accessType,
                                 INT32 direction )
   :_curExtent( NULL )
   {
      SDB_ASSERT( su, "storage data unit can't be NULL" ) ;
      SDB_ASSERT( context, "context can't be NULL" ) ;

      _pSu = su ;
      _context = context ;
      _extentCount = 0 ;
      _accessType = accessType ;
      _direction = direction ;
   }

   void _dmsExtentItr::reset( INT32 direction )
   {
      _extentCount = 0 ;
      _direction = direction ;
      _curExtent = NULL ;
   }

   _dmsExtentItr::~_dmsExtentItr ()
   {
      _pSu = NULL ;
      _context = NULL ;
      _curExtent = NULL ;
   }

   #define DMS_EXTENT_ITR_EXT_PERCOUNT    20

   INT32 _dmsExtentItr::next( dmsExtentID &extentID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( _extentCount >= DMS_EXTENT_ITR_EXT_PERCOUNT )
      {
         _context->pause() ;
         _extentCount = 0 ;

         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
      }

      if ( !_context->isMBLock() )
      {
         if ( _context->canResume() )
         {
            rc = _context->resume() ;
         }
         else
         {
            rc = _context->mbLock( DMS_IS_WRITE_OPR( _accessType ) ?
                                   EXCLUSIVE : SHARED ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         if ( !dmsAccessAndFlagCompatiblity ( _context->mb()->_flag,
                                              _accessType ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     _context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }
      }

      if ( NULL == _curExtent )
      {
         if ( _direction > 0 )
         {
            if ( DMS_INVALID_EXTENT == _context->mb()->_firstExtentID )
            {
               rc = SDB_DMS_EOC ;
               goto error ;
            }
            else
            {
               _extRW = _pSu->extent2RW( _context->mb()->_firstExtentID,
                                         _context->mbID() ) ;
               _extRW.setNothrow( FALSE ) ;
               _curExtent = _extRW.readPtr<dmsExtent>() ;
            }
         }
         else
         {
            if ( DMS_INVALID_EXTENT == _context->mb()->_lastExtentID )
            {
               rc = SDB_DMS_EOC ;
               goto error ;
            }
            else
            {
               _extRW = _pSu->extent2RW( _context->mb()->_lastExtentID,
                                         _context->mbID() ) ;
               _extRW.setNothrow( FALSE ) ;
               _curExtent = _extRW.readPtr<dmsExtent>() ;
            }
         }
      }
      else if ( ( _direction >= 0 &&
                  DMS_INVALID_EXTENT == _curExtent->_nextExtent ) ||
                ( _direction < 0 &&
                  DMS_INVALID_EXTENT == _curExtent->_prevExtent ) )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      else
      {
         dmsExtentID next = _direction >= 0 ?
                            _curExtent->_nextExtent :
                            _curExtent->_prevExtent ;
         _extRW = _pSu->extent2RW( next, _context->mbID() ) ;
         _extRW.setNothrow( FALSE ) ;
         _curExtent = _extRW.readPtr<dmsExtent>() ;
      }

      if ( 0 == _curExtent )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !_curExtent->validate( _context->mbID() ) )
      {
         PD_LOG( PDERROR, "Invalid extent[%d]", _extRW.getExtentID() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      extentID = _extRW.getExtentID() ;
      ++_extentCount ;

   done:
      return rc ;
   error:
      goto done ;
   }

   _dmsExtScannerFactory::_dmsExtScannerFactory()
   {
   }

   _dmsExtScannerFactory::~_dmsExtScannerFactory()
   {
   }

   dmsExtScannerBase* _dmsExtScannerFactory::create( dmsStorageDataCommon *su,
                                                     dmsMBContext *context,
                                                     mthMatchRuntime *matchRuntime,
                                                     dmsExtentID curExtentID,
                                                     dmsExtentID lastExtentID,
                                                     DMS_ACCESS_TYPE accessType,
                                                     INT64 maxRecords,
                                                     INT64 skipNum,
                                                     INT32 flag,
                                                     IDmsOprHandler *opHandler )
   {
      dmsExtScannerBase* scanner = NULL ;

      if ( OSS_BIT_TEST( DMS_MB_ATTR_CAPPED, context->mb()->_attributes ) )
      {
         scanner = SDB_OSS_NEW dmsCappedExtScanner( su, context,
                                                    matchRuntime,
                                                    curExtentID,
                                                    lastExtentID,
                                                    accessType,
                                                    maxRecords,
                                                    skipNum,
                                                    flag,
                                                    opHandler ) ;
      }
      else
      {
         scanner = SDB_OSS_NEW dmsExtScanner( su, context,
                                              matchRuntime,
                                              curExtentID,
                                              lastExtentID,
                                              accessType,
                                              maxRecords,
                                              skipNum,
                                              flag,
                                              opHandler ) ;
      }

      if ( !scanner )
      {
         PD_LOG( PDERROR, "Allocate memory for extent scanner failed" ) ;
         goto error ;
      }

   done:
      return scanner ;
   error:
      goto done ;
   }

   dmsExtScannerFactory* dmsGetScannerFactory()
   {
      static dmsExtScannerFactory s_factory ;
      return &s_factory ;
   }


}


