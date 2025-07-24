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

   Source File Name = dmsScanner.hpp

   Descriptive Name = Data Management Service Storage Unit Header

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
#ifndef DMSSCANNER_HPP__
#define DMSSCANNER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "ossTypes.h"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "dmsStorageBase.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dmsStorageData.hpp"
#include "dmsStorageDataCapped.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include "mthMatchRuntime.hpp"
#include "ossMemPool.hpp"
#include "dmsTransLockCallback.hpp"
<<<<<<< HEAD
#include "dmsTransContext.hpp"
=======
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#include "dmsOprHandler.hpp"

using namespace bson ;

namespace engine
{

   // forward declaration
   class _dmsMBContext ;
   class _dmsStorageDataCommon ;
   class _dmsStorageData ;
   class _dmsStorageDataCapped ;
   class _mthMatchTreeContext ;
   class _rtnTBScanner ;
   class _rtnScanner ;
   class _rtnIXScanner ;
   class _pmdEDUCB ;
   class _monAppCB ;
   class dpsTransCB ;
   class _dpsTransExecutor;
   class _dmsSecScanner ;
   class _dmsScanner ;
   class _dmsTBScanner ;

   #define DMS_IS_WRITE_OPR(accessType)   \
      ( DMS_ACCESS_TYPE_UPDATE == accessType || \
        DMS_ACCESS_TYPE_DELETE == accessType ||\
        DMS_ACCESS_TYPE_INSERT == accessType )

   #define DMS_IS_READ_OPR(accessType) \
      ( DMS_ACCESS_TYPE_QUERY == accessType || \
        DMS_ACCESS_TYPE_FETCH == accessType )

   /*
      _dmsScannerContext define
    */
   class _dmsScannerContext : public _IContext
   {
   public:
      _dmsScannerContext( _dmsSecScanner *scanner ) ;
      virtual ~_dmsScannerContext() ;

   public:
      virtual INT32 pause() ;
      virtual INT32 resume() ;

   private:
      BOOLEAN _hasPaused ;
      _dmsSecScanner *_scanner ;
   } ;

   typedef class _dmsScannerContext dmsScannerContext ;
   typedef class _dmsScannerContext dmsTBScannerContext ;
   typedef class _dmsScannerContext dmsIXScannerContext ;

   /*
      _dmsScannerContext define
   */
   class _dmsScanner ;
   class _dmsScannerContext : public _IContext
   {
   public:
      _dmsScannerContext( _dmsScanner *pScanner ) ;
      virtual ~_dmsScannerContext() ;

   public:
      virtual INT32 pause() { return SDB_OK ; }
      virtual INT32 resume() { return SDB_OK ; }

   protected:
      _dmsScanner *_pScanner ;
   };

   /*
      _dmsIXScannerContext define
   */
   class _dmsIXScannerContext : public _dmsScannerContext
   {
   public:
      _dmsIXScannerContext( _dmsScanner *pScanner, _rtnIXScanner *pIXScanner ) ;
      virtual ~_dmsIXScannerContext () ;

   public:
      virtual INT32 pause() ;
      virtual INT32 resume() ;

   private:
      BOOLEAN _hasPaused ;
      _rtnIXScanner *_pIXScanner ;
   };

   /*
      _dmsScanner define
   */
   class _dmsScanner : public utilPooledObject
   {
      public:
         _dmsScanner ( _dmsStorageDataCommon *su,
                       _dmsMBContext *context,
                       mthMatchRuntime *matchRuntime,
<<<<<<< HEAD
                       _pmdEDUCB *cb,
                       DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                       INT64 maxRecords = -1,
                       INT64 skipNum = 0,
                       INT32 flags = 0,
=======
                       DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
                       IDmsOprHandler *opHandler = NULL ) ;
         virtual ~_dmsScanner () ;

         BOOLEAN  isReadOnly() const
         {
            return DMS_IS_WRITE_OPR( _accessType ) ? FALSE : TRUE ;
         }

         virtual dmsTransLockCallback*       callbackHandler() = 0 ;
         virtual const dmsTransRecordInfo*   recordInfo() const = 0 ;

<<<<<<< HEAD
=======
         BOOLEAN needWaitForLock() const { return _waitLock ; }

      protected: 
         INT32  getTransIsolation() { return _transIsolation ; }
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) = 0 ;
         virtual void  stop () = 0 ;

<<<<<<< HEAD
         virtual dmsScannerContext *getScannerContext() = 0 ;
=======
         virtual _dmsScannerContext* getScannerContext() = 0 ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

         const dmsRecordID &getAdvancedRecordID()
         {
            return _advancedRecordID ;
         }

<<<<<<< HEAD
         INT32 getMBLockType() const
         {
            return _mbLockType ;
         }

         virtual void initLockInfo( INT32 isolation,
                                    DPS_TRANSLOCK_TYPE lockType,
                                    DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode ) = 0 ;

         virtual void enableCountMode() {}

         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }

      protected:
         void _saveAdvancedRecrodID( const dmsRecordID &recordID, INT32 rc ) ;
         void _checkMaxRecordsNum( _mthRecordGenerator &generator ) ;
=======
      protected:
         void _saveAdvancedRecrodID( const dmsRecordID &recordID, INT32 rc ) ;

         INT32 _checkGlobTransAvailable( _pmdEDUCB *cb ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

      protected:
         _dmsStorageDataCommon  *_pSu ;
         _dmsMBContext          *_context ;
         mthMatchRuntime        *_matchRuntime ;
         DMS_ACCESS_TYPE         _accessType ;
         INT32                   _mbLockType ;

         dmsRecordID             _advancedRecordID ;
         IDmsOprHandler         *_opHandler ;

<<<<<<< HEAD
         INT64                   _maxRecords ;
         INT64                   _skipNum ;
         INT32                   _flags ;
=======
         dmsRecordID             _advancedRecordID ;
         IDmsOprHandler         *_opHandler ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;
   typedef _dmsScanner dmsScanner ;

   /*
      _dmsScannerLockHandler define
    */
   class _dmsScannerLockHandler
   {
<<<<<<< HEAD
   public:
      _dmsScannerLockHandler( IDmsOprHandler *opHandler, INT32 flags ) ;
      virtual ~_dmsScannerLockHandler() ;

   protected:
      INT32 _acquireCSCLLock( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              pmdEDUCB *cb,
                              IContext *transContext ) ;
      void  _releaseCSCLLock( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              pmdEDUCB *cb ) ;

      void _initLockInfo( _dmsStorageDataCommon *su,
                          _dmsMBContext *mbContext,
                          DMS_ACCESS_TYPE accessType,
                          pmdEDUCB *cb ) ;

      void _initLockInfo( INT32 isolation,
                          DPS_TRANSLOCK_TYPE lockType,
                          DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode ) ;

      INT32 _checkTransLock( _dmsStorageDataCommon *su,
                             _dmsMBContext *mbContext,
                             const dmsRecordID &curRID,
                             pmdEDUCB *cb,
                             dmsScanTransContext *transContext,
                             dmsRecordRW &recordRW,
                             dmsRecordID &waitUnlockRID,
                             BOOLEAN &skipRecord ) ;

      void _releaseTransLock( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              const dmsRecordID &curRID,
                              pmdEDUCB *cb ) ;

      void _releaseAllLocks( _dmsStorageDataCommon *su,
                              _dmsMBContext *mbContext,
                              const dmsRecordID &curRID,
                              pmdEDUCB *cb ) ;

      virtual void _onRecordSkipped( const dmsRecordID &curRID,
                                     dmsScanTransContext *transContext )
      {
      }

      virtual void _onRecordLocked( const dmsRecordID &curRID,
                                    dmsScanTransContext *transContext,
                                    BOOLEAN &skipRecord )
      {
      }

   protected:
      // lock info
      BOOLEAN                 _isInited ;
      dpsTransCB             *_pTransCB ;
      INT32                   _transIsolation ;
      BOOLEAN                 _waitLock ;
      BOOLEAN                 _useRollbackSegment ;
      BOOLEAN                 _needEscalation ;
      BOOLEAN                 _hasLockedRecord ;
      INT8                    _recordLock ;
      INT8                    _selectLockMode ;
      INT8                    _lockOpMode ;
      BOOLEAN                 _needUnLock ;
      BOOLEAN                 _CSCLLockHeld ;
      dmsTransLockCallback    _callback ;
   } ;
=======
      friend class _dmsTBScanner ;
      public:
         _dmsExtScannerBase ( _dmsStorageDataCommon *su, _dmsMBContext *context,
                              mthMatchRuntime *matchRuntime,
                              dmsExtentID curExtentID,
                              dmsExtentID lastExtentID = DMS_INVALID_EXTENT,
                              DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                              INT64 maxRecords = -1,
                              INT64 skipNum = 0,
                              INT32 flag = 0,
                              IDmsOprHandler *handler = NULL ) ;
         virtual ~_dmsExtScannerBase () ;

         virtual dmsTransLockCallback*       callbackHandler() ;
         virtual const dmsTransRecordInfo*   recordInfo() const ;

         const dmsExtent* curExtent () const { return _extent ; }
         dmsExtentID curExtentID () const ;
         dmsExtentID nextExtentID () const ;
         INT32 stepToNextExtent() ;
         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   typedef class _dmsScannerLockHandler dmsScannerLockHandler ;

<<<<<<< HEAD
   /*
      _dmsSecScanner define
    */
   class _dmsSecScanner : public _dmsScanner, public _dmsScannerLockHandler
   {
   public:
      _dmsSecScanner( _dmsStorageDataCommon *su,
                      _dmsMBContext *context,
                      _rtnScanner *scanner,
                      mthMatchRuntime *matchRuntime,
                      DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                      INT64 maxRecords = -1,
                      INT64 skipNum = 0,
                      INT32 flag = 0,
                      IDmsOprHandler *handler = NULL ) ;
      virtual ~_dmsSecScanner() ;

      const dmsRecordID &getCurRID() const
      {
         return _curRID ;
      }

      virtual INT32 advance( dmsRecordID &recordID,
                             _mthRecordGenerator &generator,
                             _pmdEDUCB *cb,
                             _mthMatchTreeContext *mthContext = NULL ) ;
      virtual void pause() ;
      virtual void stop() ;

      virtual dmsTransLockCallback *callbackHandler() ;
      virtual const dmsTransRecordInfo *recordInfo() const ;

      virtual void initLockInfo( INT32 isolation,
                                 DPS_TRANSLOCK_TYPE lockType,
                                 DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode )
      {
         _initLockInfo( isolation, lockType, lockOpMode ) ;
      }

      virtual void enableCountMode()
      {
         _isCountOnly = TRUE ;
      }

      virtual dmsScannerContext* getScannerContext()
      {
         return &_scannerContext ;
      }

      BOOLEAN isHitEnd() const ;

      _pmdEDUCB *getEDUCB()
      {
         return _cb ;
      }

      _rtnScanner *getScanner()
      {
         return _scanner ;
      }

      _dmsStorageDataCommon *getDataSU()
      {
         return _pSu ;
      }

      _dmsMBContext *getMBContext()
      {
         return _context ;
      }

   protected:
      INT32 _firstInit( pmdEDUCB *cb ) ;
      INT32 _fetchNext( dmsRecordID &recordID,
                        _mthRecordGenerator &generator,
                        _pmdEDUCB *cb,
                        _mthMatchTreeContext *mthContext = NULL ) ;

      virtual INT32 _onFirstInit( _pmdEDUCB *cb ) = 0 ;
      virtual INT32 _advanceScanner( _pmdEDUCB *cb, dmsRecordID &rid ) = 0 ;
      virtual INT32 _checkSnapshotID( BOOLEAN &isSnapshotSame ) = 0 ;
      virtual INT32 _getCurrentRID( dmsRecordID &nextRID ) = 0 ;
      virtual INT32 _getCurrentRecord( dmsRecordData &recordData ) = 0 ;

      virtual UINT64 _getOnceRestNum() const = 0 ;

      // for _dmsScannerLockHandler
      virtual void _onRecordSkipped( const dmsRecordID &curRID,
                                     dmsScanTransContext *transContext ) ;
      virtual void _onRecordLocked( const dmsRecordID &curRID,
                                    dmsScanTransContext *transContext,
                                    BOOLEAN &skipRecord ) ;

   protected:
      _rtnScanner          *_scanner ;
      dmsScanTransContext  _transContext ;
      dmsScannerContext    _scannerContext ;
      dmsRecordID          _curRID ;
      BOOLEAN              _isCountOnly ;
      BOOLEAN              _firstRun ;
      UINT64               _onceRestNum ;
      _pmdEDUCB            *_cb ;
      dmsRecordData        _recordData ;
=======
         virtual _dmsScannerContext* getScannerContext()
         {
            return &_scannerContext ;
         }

      protected:
         virtual INT32 _firstInit( _pmdEDUCB *cb ) = 0 ;
         virtual INT32 _fetchNext( dmsRecordID &recordID,
                                   _mthRecordGenerator &generator,
                                   _pmdEDUCB *cb,
                                   _mthMatchTreeContext *mhtContext = NULL) = 0 ;
         void _checkMaxRecordsNum( _mthRecordGenerator &generator ) ;
         INT32 acquireCSCLLock() ;
         void  releaseCSCLLock() ;

      protected:
         INT64                _maxRecords ;
         INT64                _skipNum ;
         dmsExtRW             _extRW ;
         const dmsExtent      *_extent ;
         dmsRecordID          _curRID ;
         dmsRecordRW          _recordRW ;
         const dmsRecord      *_curRecordPtr ;
         dmsOffset            _next ;
         BOOLEAN              _firstRun ;
         BOOLEAN              _hasLockedRecord ;
         dpsTransCB           *_pTransCB ;
         INT8                 _recordLock ;
         INT8                 _selectLockMode ;
         BOOLEAN              _needUnLock ;
         BOOLEAN              _needEscalation ;
         BOOLEAN              _CSCLLockHeld ;
         _pmdEDUCB            *_cb ;
         _dmsScannerContext   _scannerContext ;
         dmsExtentID          _lastExtentID ;
         dmsTransLockCallback _callback ;
   };
   typedef _dmsExtScannerBase dmsExtScannerBase ;

   class _dmsExtScanner : public _dmsExtScannerBase
   {
      public:
         _dmsExtScanner( dmsStorageDataCommon *su, _dmsMBContext *context,
                         mthMatchRuntime *matchRuntime,
                         dmsExtentID curExtentID,
                         dmsExtentID lastExtentID = DMS_INVALID_EXTENT,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1,
                         INT64 skipNum = 0,
                         INT32 flag = 0,
                         IDmsOprHandler *handler = NULL ) ;
         virtual ~_dmsExtScanner() ;

      private:
         virtual INT32 _firstInit( _pmdEDUCB *cb ) ;
         virtual INT32 _fetchNext( dmsRecordID &recordID,
                                   _mthRecordGenerator &generator,
                                   _pmdEDUCB *cb,
                                   _mthMatchTreeContext *mhtContext = NULL) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;
   typedef class _dmsSecScanner dmsSecScanner ;

   /*
      _dmsDataScanner define
    */
   class _dmsDataScanner : public _dmsSecScanner
   {
   public:
      _dmsDataScanner( _dmsStorageDataCommon *su,
                       _dmsMBContext *context,
                       _rtnTBScanner *scanner,
                       mthMatchRuntime *matchRuntime,
                       DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                       INT64 maxRecords = -1,
                       INT64 skipNum = 0,
                       INT32 flags = 0,
                       IDmsOprHandler *opHandler = NULL ) ;
      virtual ~_dmsDataScanner() = default ;

   protected:
      virtual INT32 _onFirstInit( _pmdEDUCB *cb ) ;
      virtual INT32 _advanceScanner( _pmdEDUCB *cb, dmsRecordID &rid ) ;
      virtual INT32 _checkSnapshotID( BOOLEAN &isSnapshotSame ) ;
      virtual INT32 _getCurrentRID( dmsRecordID &nextRID ) ;
      virtual INT32 _getCurrentRecord( dmsRecordData &recordData ) ;

      virtual UINT64 _getOnceRestNum() const ;

   protected:
      _rtnTBScanner *_scanner ;
   } ;

<<<<<<< HEAD
   typedef class _dmsDataScanner dmsDataScanner ;
=======
      public:
         _dmsCappedExtScanner ( dmsStorageDataCommon *su,
                                _dmsMBContext *context,
                                mthMatchRuntime *matchRuntime,
                                dmsExtentID curExtentID,
                                dmsExtentID lastExtentID = DMS_INVALID_EXTENT,
                                DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                                INT64 maxRecords = -1,
                                INT64 skipNum = 0,
                                INT32 flag = 0,
                                IDmsOprHandler *handler = NULL ) ;
         virtual ~_dmsCappedExtScanner() ;
         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   /*
      _dmsIndexScanner define
    */
   class _dmsIndexScanner : public _dmsSecScanner
   {
   public:
      _dmsIndexScanner( _dmsStorageDataCommon *su,
                        _dmsMBContext *context,
                        _rtnIXScanner *scanner,
                        mthMatchRuntime *matchRuntime,
                        DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                        INT64 maxRecords = -1,
                        INT64 skipNum = 0,
                        INT32 flags = 0,
                        IDmsOprHandler *opHandler = NULL ) ;
      virtual ~_dmsIndexScanner() = default ;

   protected:
      virtual INT32 _onFirstInit( _pmdEDUCB *cb ) ;

      virtual INT32 _advanceScanner( _pmdEDUCB *cb, dmsRecordID &rid ) ;
      virtual INT32 _checkSnapshotID( BOOLEAN &isSnapshotSame ) ;
      virtual INT32 _getCurrentRID( dmsRecordID &nextRID ) ;
      virtual INT32 _getCurrentRecord( dmsRecordData &recordData ) ;

      virtual UINT64 _getOnceRestNum() const ;

      const CHAR *_buildIndexRecord() ;

   protected:
      _rtnIXScanner *_scanner ;
   } ;

   typedef class _dmsIndexScanner dmsIndexScanner ;

   /*
      _dmsEntireScanner define
    */
   class _dmsEntireScanner : public _dmsScanner
   {
   public:
      _dmsEntireScanner( _dmsStorageDataCommon *su,
                         _dmsMBContext *context,
                         mthMatchRuntime *matchRuntime,
                         dmsSecScanner &secScanner,
                         _rtnScanner *scanner,
                         BOOLEAN ownedScanner,
                         dmsScannerContext &scannerContext,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1,
                         INT64 skipNum = 0,
                         INT32 flag = 0,
                         IDmsOprHandler *opHandler = NULL ) ;
<<<<<<< HEAD
      virtual ~_dmsEntireScanner() ;

      virtual dmsTransLockCallback *callbackHandler()
      {
         return _secScanner.callbackHandler() ;
      }

      virtual const dmsTransRecordInfo *recordInfo() const
      {
         return _secScanner.recordInfo() ;
      }

      virtual INT32 advance( dmsRecordID &recordID,
                             _mthRecordGenerator &generator,
                             _pmdEDUCB *cb,
                             _mthMatchTreeContext *mthContext = NULL ) ;
      virtual void  stop() ;

      virtual dmsScannerContext *getScannerContext()
      {
         return &_scannerContext ;
      }

      virtual void initLockInfo( INT32 isolation,
                                 DPS_TRANSLOCK_TYPE lockType,
                                 DPS_TRANSLOCK_OP_MODE_TYPE lockOpMode )
      {
         if ( !_lockInited )
         {
            _isolation = isolation ;
            _lockType = lockType ;
            _lockOpMode = lockOpMode ;
            _lockInited = TRUE ;
         }
         _secScanner.initLockInfo( isolation, lockType, lockOpMode ) ;
      }

      _rtnScanner *getScanner()
      {
         return _scanner ;
      }

   protected:
      void  _pauseInnerScanner() ;
      INT32 _firstInit() ;

      virtual INT32 _onInit() = 0 ;

   protected:
      dmsSecScanner &            _secScanner ;
      _rtnScanner *              _scanner ;
      BOOLEAN                    _ownedScanner ;
      BOOLEAN                    _firstRun ;
      dmsScannerContext &        _scannerContext ;

      BOOLEAN                    _lockInited ;
      INT32                      _isolation ;
      DPS_TRANSLOCK_TYPE         _lockType ;
      DPS_TRANSLOCK_OP_MODE_TYPE _lockOpMode ;
   } ;
=======
         ~_dmsTBScanner () ;

         virtual dmsTransLockCallback*       callbackHandler() ;
         virtual const dmsTransRecordInfo*   recordInfo() const ;

      public:
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   typedef class _dmsEntireScanner dmsEntireScanner ;

<<<<<<< HEAD
   /*
      _dmsTBScanner define
    */
   class _dmsTBScanner : public _dmsEntireScanner
   {
   public:
      _dmsTBScanner( _dmsStorageDataCommon *su,
                     _dmsMBContext *context,
                     mthMatchRuntime *matchRuntime,
                     _rtnTBScanner *scanner,
                     BOOLEAN ownedScanner = TRUE,
                     DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                     INT64 maxRecords = -1,
                     INT64 skipNum = 0,
                     INT32 flag = 0,
                     IDmsOprHandler *opHandler = NULL ) ;
      virtual ~_dmsTBScanner() = default ;

   protected:
      virtual INT32 _onInit() ;

   private:
      dmsDataScanner _secScanner ;
      dmsTBScannerContext _scannerContext ;
=======
         virtual _dmsScannerContext* getScannerContext()
         {
            return &_scannerContext ;
         }

      protected:
         void  _resetExtScanner() ;
         INT32 _firstInit() ;

      private:
         INT32 _getExtScanner() ;

      private:
         dmsExtScannerBase         *_extScanner ;
         dmsExtentID                _curExtentID ;
         BOOLEAN                    _firstRun ;
         INT64                      _maxRecords ;
         INT64                      _skipNum ;
         INT32                      _flag ;
         _dmsScannerContext         _scannerContext ;
   };
   typedef _dmsTBScanner dmsTBScanner ;

   class _dmsIXScanner ;
   /*
      _dmsIXSecScanner define
      dms index section scanner
   */
   #define  DMS_SCAN_STEP_SCALE_THRESH  ( (INT64) 100 )
   #define  DMS_SCAN_STEP_SCALE_FACTOR  ( (INT64) 10 )
   #define  DMS_DFT_INDEX_SCAN_STEP     (100)

   class _dmsIXSecScanner : public _dmsScanner
   {
      friend class _dmsIXScanner ;

      class _SimpleBSONBuilder ;
      typedef _SimpleBSONBuilder SimpleBSONBuilder ;

      public:
         _dmsIXSecScanner ( dmsStorageDataCommon *su,
                            _dmsMBContext *context,
                            mthMatchRuntime *matchRuntime,
                            _rtnIXScanner *scanner,
                            DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                            INT64 maxRecords = -1,
                            INT64 skipNum = 0,
                            INT32 flag = 0,
                            IDmsOprHandler *opHandler = NULL ) ;
         virtual ~_dmsIXSecScanner () ;

         virtual dmsTransLockCallback*       callbackHandler() ;
         virtual const dmsTransRecordInfo*   recordInfo() const ;

         void  enableIndexBlockScan( const BSONObj &startKey,
                                     const BSONObj &endKey,
                                     const dmsRecordID &startRID,
                                     const dmsRecordID &endRID,
                                     INT32 direction ) ;

         void  enableCountMode() { _countOnly = TRUE ; }
         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }
         BOOLEAN eof () const { return _eof ; }

         void release() ;

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mhtContext = NULL ) ;
         virtual void  stop () ;

         virtual _dmsScannerContext* getScannerContext()
         {
            return &_ixScannerContext ;
         }

      protected:
         INT32 _firstInit( _pmdEDUCB *cb ) ;
         BSONObj* _getStartKey () ;
         BSONObj* _getEndKey () ;
         dmsRecordID* _getStartRID () ;
         dmsRecordID* _getEndRID () ;
         void _updateMaxRecordsNum( _mthRecordGenerator &generator ) ;
         INT32 acquireCSCLLock( ) ;
         void  releaseCSCLLock( ) ;

         // test or acquire transaction lock, acquire old version
         INT32 _checkTransLock( pmdEDUCB *cb,
                                dmsRecordID &waitUnlockRID,
                                dmsRecordData *recordData,
                                BOOLEAN &skipRecord,
                                BOOLEAN *needData = NULL ) ;

         BOOLEAN _buildObj( ixmIndexNode *node,
                            IXM_ELE_RAWDATA_ARRAY& value,
                            SimpleBSONBuilder& builder ) ;

         const CHAR* _buildIndexRecord() ;

      private:
         INT64                _maxRecords ;
         INT64                _skipNum ;
         dmsRecordID          _curRID ;
         dmsRecordRW          _recordRW ;
         const dmsRecord      *_curRecordPtr ;
         BOOLEAN              _firstRun ;
         BOOLEAN              _hasLockedRecord ;
         dpsTransCB           *_pTransCB ;
         INT8                 _recordLock ;
         INT8                 _selectLockMode ;
         BOOLEAN              _needUnLock ;
         BOOLEAN              _needEscalation ;
         _pmdEDUCB            *_cb ;
         _rtnIXScanner        *_scanner ;
         INT64                _onceRestNum ;
         BOOLEAN              _eof ;

         dmsTransLockCallback _callback ;

         BSONObj              _startKey ;
         BSONObj              _endKey ;
         dmsRecordID          _startRID ;
         dmsRecordID          _endRID ;
         BOOLEAN              _indexBlockScan ;
         INT32                _blockScanDir ;
         BOOLEAN              _judgeStartKey ;
         BOOLEAN              _includeStartKey ;
         BOOLEAN              _includeEndKey ;
         BOOLEAN              _countOnly ;
         BOOLEAN              _CSCLLockHeld ;
         _dmsIXScannerContext _ixScannerContext ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;

   typedef class _dmsTBScanner dmsTBScanner ;

   /*
      _dmsIXScanner define
   */
   class _dmsIXScanner : public _dmsEntireScanner
   {
<<<<<<< HEAD
   public:
      _dmsIXScanner( dmsStorageDataCommon *su,
                     _dmsMBContext *context,
                     mthMatchRuntime *matchRuntime,
                     _rtnIXScanner *scanner,
                     BOOLEAN ownedScanner = FALSE,
                     DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                     INT64 maxRecords = -1,
                     INT64 skipNum = 0,
                     INT32 flag = 0,
                     IDmsOprHandler *opHandler = NULL ) ;
      ~_dmsIXScanner() = default ;

   protected:
      INT32 _onInit() ;

   private:
      dmsIndexScanner _secScanner ;
      dmsIXScannerContext _scannerContext ;
=======
      public:
         _dmsIXScanner ( dmsStorageDataCommon *su,
                         _dmsMBContext *context,
                         mthMatchRuntime *matchRuntime,
                         _rtnIXScanner *scanner,
                         BOOLEAN ownedScanner = FALSE,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1,
                         INT64 skipNum = 0,
                         INT32 flag = 0,
                         IDmsOprHandler *opHandler = NULL ) ;
         ~_dmsIXScanner () ;

         virtual dmsTransLockCallback*       callbackHandler() ;
         virtual const dmsTransRecordInfo*   recordInfo() const ;

         _rtnIXScanner* getScanner () { return _scanner ; }

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) ;
         virtual void  stop () ;

         virtual _dmsScannerContext* getScannerContext()
         {
            return &_ixScannerContext ;
         }

      protected:
         void  _resetIXSecScanner() ;

      private:
         dmsIXSecScanner            _secScanner ;
         _rtnIXScanner              *_scanner ;
         BOOLEAN                    _eof ;
         BOOLEAN                    _ownedScanner ;
         _dmsIXScannerContext       _ixScannerContext ;

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;
   typedef class _dmsIXScanner dmsIXScanner ;

   /*
      _IDmsScannerChecker define
    */
   // scanner checker to check if scanner is interrupted
   class _IDmsScannerChecker
   {
   public:
      _IDmsScannerChecker() {}
      virtual ~_IDmsScannerChecker() {}

   public:
      virtual BOOLEAN needInterrupt() = 0 ;
   } ;
   typedef class _IDmsScannerChecker IDmsScannerChecker ;

   /*
      _IDmsScannerCheckerCreator define
    */
   class _IDmsScannerCheckerCreator
   {
<<<<<<< HEAD
   private:
      // disallow copy and assign
      _IDmsScannerCheckerCreator( const _IDmsScannerCheckerCreator& ) ;
      void operator=( const _IDmsScannerCheckerCreator & ) ;

   protected:
      _IDmsScannerCheckerCreator() {}

   public:
      virtual ~_IDmsScannerCheckerCreator() {}
      virtual INT32 createChecker( UINT32 suLID,
                                   UINT32 mbLID,
                                   const CHAR *csName,
                                   const CHAR *clShortName,
                                   const CHAR *optrDesc,
                                   _pmdEDUCB *cb,
                                   IDmsScannerChecker **ppChecker ) = 0 ;
      virtual void releaseChecker( IDmsScannerChecker *pChecker ) = 0 ;
   } ;
=======
      public:
         _dmsExtScannerFactory() ;
         ~_dmsExtScannerFactory() ;

         dmsExtScannerBase* create( dmsStorageDataCommon *su,
                                    dmsMBContext *context,
                                    mthMatchRuntime *matchRuntime,
                                    dmsExtentID curExtentID,
                                    dmsExtentID lastExtentID,
                                    DMS_ACCESS_TYPE accessType,
                                    INT64 maxRecords,
                                    INT64 skipNum,
                                    INT32 flag,
                                    IDmsOprHandler *opHandler = NULL ) ;
   } ;
   typedef _dmsExtScannerFactory dmsExtScannerFactory ;

   dmsExtScannerFactory* dmsGetScannerFactory() ;

   /*
      _IDmsScannerChecker define
    */
   // scanner checker to check if scanner is interrupted
   class _IDmsScannerChecker
   {
   public:
      _IDmsScannerChecker() {}
      virtual ~_IDmsScannerChecker() {}

   public:
      virtual BOOLEAN needInterrupt() = 0 ;
   } ;
   typedef class _IDmsScannerChecker IDmsScannerChecker ;

   /*
      _IDmsScannerCheckerCreator define
    */
   class _IDmsScannerCheckerCreator
   {
   private:
      // disallow copy and assign
      _IDmsScannerCheckerCreator( const _IDmsScannerCheckerCreator& ) ;
      void operator=( const _IDmsScannerCheckerCreator & ) ;

   protected:
      _IDmsScannerCheckerCreator() {}

   public:
      virtual ~_IDmsScannerCheckerCreator() {}
      virtual INT32 createChecker( UINT32 suLID,
                                   UINT32 mbLID,
                                   const CHAR *csName,
                                   const CHAR *clShortName,
                                   const CHAR *optrDesc,
                                   _pmdEDUCB *cb,
                                   IDmsScannerChecker **ppChecker ) = 0 ;
      virtual void releaseChecker( IDmsScannerChecker *pChecker ) = 0 ;
   } ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   typedef class _IDmsScannerCheckerCreator IDmsScannerCheckerCreator ;

}

#endif //DMSSCANNER_HPP__

