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

   Source File Name = dmsTransLockCallback.hpp

   Descriptive Name = DMS Transaction Lock Callback Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/02/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_TRANS_LOCK_CALLBACK_HPP__
#define DMS_TRANS_LOCK_CALLBACK_HPP__

#include "dpsTransLockCallback.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "dmsOprHandler.hpp"
#include "dpsTransCB.hpp"
#include "pmdEDU.hpp"
#include "utilBitmap.hpp"

using namespace bson ;

namespace engine
{

   class dpsTransCB ;
   class _dmsRecordRW ;
   class oldVersionContainer ;
   class oldVersionCB ;
   class _rtnIXScanner ;
   struct _dmsMBStatInfo ;

   typedef _utilStackBitmap< DMS_COLLECTION_MAX_INDEX > DMS_TRANS_INDEX_BITMAP ;

   // Class to implment lock call back funtions for DMS scanner
   class dmsTransLockCallback : public _dpsITransLockCallback,
                                public _IDmsOprHandler
   {
   public:
      dmsTransLockCallback() ;

      dmsTransLockCallback( dpsTransCB *transCB,
                            _pmdEDUCB  *eduCB ) ;

      void     clearStatus() ;

      virtual ~dmsTransLockCallback() ;

      void     setBaseInfo( dpsTransCB *transCB, _pmdEDUCB *eduCB ) ;

      void     setIDInfo( INT32 csID, UINT16 clID,
                          UINT32 csLID, UINT32 clLID ) ;

      void     setIXScanner( _rtnIXScanner *pScanner ) ;

      void     attachRecordRW( _dmsRecordRW  * recordRW, 
                               dmsRecordData * recordData ) ;
      void     detachRecordRW() ;

      /*
         Status
      */
      BOOLEAN  isSkipRecord() const { return _skipRecord ; }
      BOOLEAN  isNonTransNeedCleanup() const { return _nonTransNeedCleanup ; }
      void     setNonTransNeedCleanup() { _nonTransNeedCleanup = TRUE ; }
      BOOLEAN  isUseOldVersion() const { return _useOldVersion ; }
      void     setUseLatestVersion() { _useLatestVersion = TRUE ; }
      BOOLEAN  isUseLatestVersion() const { return _useLatestVersion ; }

      BOOLEAN  idxTreeLatched ( SINT32 lid )
      {
         return ( lid == _latchedIdxLid ) ;
      }
      INT32 idxTreeLatchMode () const ;

      const dmsTransRecordInfo*  getTransRecordInfo() const ;

      DPS_TRANS_ID getRecordTransID() ;
      DPS_TRANS_ID getOwnerTransID() ;

      BOOLEAN isIndexProtectionRequired() ;
      BOOLEAN isPostActionRequired() { return _needPostAction ; }

      BOOLEAN isIndexProtected( INT32 idxTreeId, INT32 latchMode = -1 ) ;

      const dmsRBSOffset & getRBSRecordOffset() ;

      void  setRBSRecordOffset( dmsRBSOffset & loc )
      {
         _rbsRecordOffset._clID = loc._clID ;
         _rbsRecordOffset._logicalID = loc._logicalID ;
      }

      // mark index updated
      void setIndexUpdated( INT32 indexID )
      {
         if ( indexID >= 0 )
         {
            _indexBitmap.setBit( (UINT32)indexID ) ;
         }
      }

      // check if index is updated
      BOOLEAN isIndexUpdated( INT32 indexID )
      {
         return ( indexID >= 0 ) &&
                ( _indexBitmap.testBit( (UINT32)indexID ) ) ;
      }

   public:
      INT32 checkRecordVisible( dmsMBContext *context ) ;

      /// Interface
      virtual void afterLockAcquire( const dpsTransLockId &lockId,
                                     INT32 irc,
                                     DPS_TRANSLOCK_TYPE requestLockMode,
                                     UINT32 refCounter,
                                     DPS_TRANSLOCK_OP_MODE_TYPE opMode,
                                     dpsLRBExtData *pExtData ) ;

      virtual void afterLockAcquirePostAction( const dpsTransLockId &lockId );

      virtual void beforeLockRelease( const dpsTransLockId &lockId,
                                      DPS_TRANSLOCK_TYPE lockMode,
                                      UINT32 refCounter,
                                      dpsLRBExtData *pExtData ) ;

      virtual void afterLockEscalated( const dpsTransLockId &lockId,
                                       DPS_TRANSLOCK_OP_MODE_TYPE opMode ) ;

      virtual INT32 getResult() { return _result ; }
      virtual BOOLEAN hasError()
      {
         return SDB_OK != _result ? TRUE : FALSE ;
      }

   public:
      virtual void  onCSClosed( INT32 csID ) ;
      virtual void  onCLTruncated( INT32 csID, UINT16 clID ) ;

      virtual INT32 onCreateIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   _pmdEDUCB *cb ) ;

      virtual INT32 onDropIndex( _dmsMBContext *context,
                                 const ixmIndexCB *indexCB,
                                 _pmdEDUCB *cb ) ;

      virtual INT32 onRebuildIndex( _dmsMBContext *context,
                                    const ixmIndexCB *indexCB,
                                    _pmdEDUCB *cb,
                                    utilWriteResult *pResult ) ;

      virtual INT32 onInsertRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB* cb ) ;

      virtual INT32 onDeleteRecord( _dmsMBContext *context,
                                    const BSONObj &object,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    BOOLEAN markDeleting,
                                    _pmdEDUCB* cb ) ;

      virtual INT32 onUpdateRecord( _dmsMBContext *context,
                                    const BSONObj &orignalObj,
                                    const BSONObj &newObj,
                                    const dmsRecordID &rid,
                                    const _dmsRecordRW *pRecordRW,
                                    _pmdEDUCB* cb ) ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) ;

      virtual INT32 onInsertIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObj &keyObj,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) ;

      virtual INT32 onUpdateIndex( _dmsMBContext *context,
                                   INT32 indexID,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforce,
                                   const BSONObjSet &oldKeySet,
                                   const BSONObjSet &newKeySet,
                                   const dmsRecordID &rid,
                                   BOOLEAN isRollback,
                                   _pmdEDUCB* cb,
                                   utilWriteResult *pResult ) ;

      virtual INT32 onDeleteIndex( _dmsMBContext *context,
                                   const ixmIndexCB *indexCB,
                                   BOOLEAN isUnique,
                                   const BSONObjSet &keySet,
                                   const dmsRecordID &rid,
                                   _pmdEDUCB* cb ) ;

   protected:
      enum _INSERT_CURSOR
      {
         _INSERT_NONE,
         _INSERT_CHECK
      } ;
      INT32         _checkInsertIndex( preIdxTreePtr &treePtr,
                                       _INSERT_CURSOR &insertCursor,
                                       const ixmIndexCB *indexCB,
                                       BOOLEAN isUnique,
                                       BOOLEAN isEnforce,
                                       const BSONObj &keyObj,
                                       const dmsRecordID &rid,
                                       _pmdEDUCB* cb,
                                       BOOLEAN allowSelfDup,
                                       utilWriteResult *pResult ) ;

      enum _DELETE_CURSOR
      {
         _DELETE_NONE,
         _DELETE_IGNORE,
         _DELETE_SAVE
      } ;
      INT32         _checkDeleteIndex( preIdxTreePtr &treePtr,
                                       _DELETE_CURSOR &deleteCursor,
                                       INT32 indexID,
                                       const ixmIndexCB *indexCB,
                                       BOOLEAN isUnique,
                                       const BSONObj &keyObj,
                                       const dmsRecordID &rid,
                                       _pmdEDUCB* cb ) ;

      INT32 _checkIDIndexUpdate( const dmsRecordID &rid,
                                 const BSONElement &idEle,
                                 _pmdEDUCB *cb ) ;

      // check if we need to rollback on given index
      INT32 _checkRollbackIndex( INT32 indexID,
                                 const ixmIndexCB *indexCB,
                                 pmdEDUCB *cb ) ;

   private:

      INT32    saveOldVersionRecord( const _dmsRecordRW *pRecordRW,
                                     const dmsRecordID  &rid,
                                     const UINT32        clLID,
                                     const BSONObj      &obj,
                                     const UINT32        ownerTID ) ;

      INT32    saveOldVersionRecordToRBS( const _dmsRecordRW *pRecordRW,
                                     const dmsRecordID &rid,
                                     const BSONObj &obj,
                                     UINT32 ownerTID ) ;

      void     _afterAcquireUXLockOrNonRRread(
                                     const dpsTransLockId      &lockId,
                                     INT32                      irc,
                                     DPS_TRANSLOCK_TYPE         requestLockMode,
                                     UINT32                     refCounter,
                                     DPS_TRANSLOCK_OP_MODE_TYPE opMode,
                                     dpsLRBExtData             *pExtData ) ;

      void    _afterAcquireSLockRRread(
                                     const dpsTransLockId      &lockId,
                                     INT32                      irc,
                                     DPS_TRANSLOCK_TYPE         requestLockMode,
                                     UINT32                     refCounter,
                                     DPS_TRANSLOCK_OP_MODE_TYPE opMode,
                                     dpsLRBExtData             *pExtData ) ;

      INT32   _validateRecordFromOldVer( pmdEDUCB             *eduCB,
                                         const DPS_TRANS_ID   &transID,
                                         BOOLEAN              &visible ) ;

   private:
      dpsTransCB           *_transCB ;    // use it to access global old copy tree
      pmdEDUCB             *_eduCB ;
      oldVersionCB         *_oldVerCB ;
      _dmsRBSMgr         *_rbsMgr ;

      // DMS related information
      _dmsRecordRW         *_recordRW ;
      // record data read from RBS
      dmsRecordData        *_rbsRecordData ;
      // working area to be setup by callback function so the update can
      // put proper old copy into the area right before the update
      oldVersionContainer  *_oldVer ;

      /// control var
      BOOLEAN              _skipRecord ;
      INT32                _result ;
      BOOLEAN              _needPostAction ;
      BOOLEAN              _useOldVersion ;
      BOOLEAN              _recordOnDiskVisible ;
      BOOLEAN              _useLatestVersion ;
      // save transaction ID of record from disk, which will be used
      // in RBS to check MVCC record chain
      DPS_TRANS_ID         _diskRecordTransID ;
      // used for non-transactional operation to track if the operation
      // (update/delete) need to cleanup nodes for this rid in memidxtree.
      // Only need the cleanup when the operation was successfull.
      // Note that because this is for Non-transactional, it's set/reset
      // per record, and per record lock
      BOOLEAN              _nonTransNeedCleanup;
      dpsOldRecordPtr      _recordPtr ;
      dmsRBSOffset         _rbsRecordOffset ;

      /// status var
      UINT32               _csLID ;
      UINT32               _clLID ;
      INT32                _csID ;
      UINT16               _clID ;
      SINT32               _latchedIdxLid ; // which we are holding a latch on
      INT32                _transIsolation ;
      _rtnIXScanner       *_pScanner ;
      oldVersionUnitPtr    _unitPtr ;

      dmsTransRecordInfo   _recordInfo ;

      // index bitmap to indicate which index is updated
      DMS_TRANS_INDEX_BITMAP _indexBitmap ;
   } ;

}

#endif // DMS_TRANS_LOCK_CALLBACK_HPP__

