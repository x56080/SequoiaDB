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

using namespace bson ;

namespace engine
{

   class _dmsMBContext ;
   class _dmsStorageDataCommon ;
   class _dmsStorageData ;
   class _dmsStorageDataCapped ;
   class _mthMatchTreeContext ;
   class _rtnIXScanner ;
   class _pmdEDUCB ;
   class _monAppCB ;
   class dpsTransCB ;
   class _dpsTransExecutor;

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
         _dmsScanner ( _dmsStorageDataCommon *su, _dmsMBContext *context,
                       mthMatchRuntime *matchRuntime,
                       DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH ) ;
         virtual ~_dmsScanner () ;

         BOOLEAN  isReadOnly() const
         {
            return SHARED == _mbLockType ? TRUE : FALSE ;
         }

         virtual dmsTransLockCallback*       callbackHandler() = 0 ;
         virtual const dmsTransRecordInfo*   recordInfo() const = 0 ;

         BOOLEAN needWaitForLock() const { return _waitLock ; }

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) = 0 ;
         virtual void  stop () = 0 ;

         virtual _dmsScannerContext* getScannerContext() = 0 ;

         const dmsRecordID &getAdvancedRecordID()
         {
            return _advancedRecordID ;
         }

      protected:
         void _saveAdvancedRecrodID( const dmsRecordID &recordID, INT32 rc ) ;

      protected:
         _dmsStorageDataCommon  *_pSu ;
         _dmsMBContext          *_context ;
         mthMatchRuntime        *_matchRuntime ;
         DMS_ACCESS_TYPE         _accessType ;
         INT32                   _mbLockType ;

         INT32                   _transIsolation ;
         BOOLEAN                 _waitLock ;
         BOOLEAN                 _useRollbackSegment ;

         dmsRecordID             _advancedRecordID ;
   } ;
   typedef _dmsScanner dmsScanner ;

   class _dmsTBScanner ;
   /*
      _dmsExtScanner define
   */
   class _dmsExtScannerBase : public _dmsScanner
   {
      friend class _dmsTBScanner ;
      public:
         _dmsExtScannerBase ( _dmsStorageDataCommon *su, _dmsMBContext *context,
                              mthMatchRuntime *matchRuntime,
                              dmsExtentID curExtentID,
                              DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                              INT64 maxRecords = -1,
                              INT64 skipNum = 0,
                              INT32 flag = 0 ) ;
         virtual ~_dmsExtScannerBase () ;

         virtual dmsTransLockCallback*       callbackHandler() ;
         virtual const dmsTransRecordInfo*   recordInfo() const ;

         const dmsExtent* curExtent () const { return _extent ; }
         dmsExtentID nextExtentID () const ;
         INT32 stepToNextExtent() ;
         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mhtContext = NULL ) ;
         virtual void  stop () ;

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

         dmsTransLockCallback    _callback ;
   };
   typedef _dmsExtScannerBase dmsExtScannerBase ;

   class _dmsExtScanner : public _dmsExtScannerBase
   {
      public:
         _dmsExtScanner( dmsStorageDataCommon *su, _dmsMBContext *context,
                         mthMatchRuntime *matchRuntime,
                         dmsExtentID curExtentID,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1,
                         INT64 skipNum = 0,
                         INT32 flag = 0 ) ;
         virtual ~_dmsExtScanner() ;

      private:
         virtual INT32 _firstInit( _pmdEDUCB *cb ) ;
         virtual INT32 _fetchNext( dmsRecordID &recordID,
                                   _mthRecordGenerator &generator,
                                   _pmdEDUCB *cb,
                                   _mthMatchTreeContext *mhtContext = NULL) ;
   } ;
   typedef _dmsExtScanner dmsExtScanner ;

   class _dmsCappedExtScanner : public _dmsExtScannerBase
   {
      typedef std::pair<dmsExtentID, dmsExtentID>  EXT_LID_PAIR ;
      typedef ossPoolSet<EXT_LID_PAIR>       EXT_RANGE_SET ;
      typedef EXT_RANGE_SET::iterator              EXT_RANGE_SET_ITR ;

      public:
         _dmsCappedExtScanner ( dmsStorageDataCommon *su,
                                _dmsMBContext *context,
                                mthMatchRuntime *matchRuntime,
                                dmsExtentID curExtentID,
                                DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                                INT64 maxRecords = -1,
                                INT64 skipNum = 0,
                                INT32 flag = 0 ) ;
         virtual ~_dmsCappedExtScanner() ;
         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }

      public:

         const dmsExtent* curExtent () { return _extent ; }
         dmsExtentID nextExtentID () const ;

      protected:
         virtual INT32 _firstInit( _pmdEDUCB *cb ) ;
         virtual INT32 _fetchNext( dmsRecordID &recordID,
                                   _mthRecordGenerator &generator,
                                   _pmdEDUCB *cb,
                                   _mthMatchTreeContext *mhtContext = NULL) ;

         INT32 _initFastScanRange() ;
         INT32 _validateRange( BOOLEAN &inRange ) ;

         OSS_INLINE dmsExtentID _idToExtLID( INT64 id ) ;

      private:
         dmsOffset               _lastOffset ;
         const _dmsExtentInfo    *_workExtInfo ;
         BOOLEAN                 _rangeInit ;
         BOOLEAN                 _fastScanByID ;
         EXT_RANGE_SET           _rangeSet ;
   } ;
   typedef _dmsCappedExtScanner dmsCappedExtScanner ;

   /*
      _dmsTBScanner define
   */
   class _dmsTBScanner : public _dmsScanner
   {
      public:
         _dmsTBScanner ( _dmsStorageDataCommon *su, _dmsMBContext *context,
                         mthMatchRuntime *matchRuntime,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1,
                         INT64 skipNum = 0,
                         INT32 flag = 0  ) ;
         ~_dmsTBScanner () ;

         virtual dmsTransLockCallback*       callbackHandler() ;
         virtual const dmsTransRecordInfo*   recordInfo() const ;

      public:

         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) ;
         virtual void  stop () ;

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
                            INT32 flag = 0 ) ;
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
                                BOOLEAN &skipRecord ) ;

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
   } ;
   typedef _dmsIXSecScanner dmsIXSecScanner ;

   /*
      _dmsIXScanner define
   */
   class _dmsIXScanner : public _dmsScanner
   {
      public:
         _dmsIXScanner ( dmsStorageDataCommon *su,
                         _dmsMBContext *context,
                         mthMatchRuntime *matchRuntime,
                         _rtnIXScanner *scanner,
                         BOOLEAN ownedScanner = FALSE,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1,
                         INT64 skipNum = 0,
                         INT32 flag = 0 ) ;
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

   } ;
   typedef _dmsIXScanner dmsIXScanner ;

   /*
      _dmsExtentItr define
   */
   class _dmsExtentItr : public SDBObject
   {
      public:
         _dmsExtentItr ( _dmsStorageData *su,
                        _dmsMBContext *context,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_QUERY,
                         INT32 direction = 1 ) ;
         ~_dmsExtentItr () ;

         void  reset( INT32 direction ) ;

         INT32 getDirection() const { return _direction ; }

      public:
         INT32    next ( dmsExtentID &extentID, _pmdEDUCB *cb ) ;

      private:
         _dmsStorageData            *_pSu ;
         _dmsMBContext              *_context ;
         dmsExtRW                   _extRW ;
         const dmsExtent            *_curExtent ;
         DMS_ACCESS_TYPE            _accessType ;
         UINT32                     _extentCount ;
         INT32                      _direction ;

   } ;
   typedef _dmsExtentItr dmsExtentItr ;

   class _dmsExtScannerFactory : public SDBObject
   {
      public:
         _dmsExtScannerFactory() ;
         ~_dmsExtScannerFactory() ;

         dmsExtScannerBase* create( dmsStorageDataCommon *su,
                                    dmsMBContext *context,
                                    mthMatchRuntime *matchRuntime,
                                    dmsExtentID curExtentID,
                                    DMS_ACCESS_TYPE accessType,
                                    INT64 maxRecords,
                                    INT64 skipNum,
                                    INT32 flag ) ;
   } ;
   typedef _dmsExtScannerFactory dmsExtScannerFactory ;

   dmsExtScannerFactory* dmsGetScannerFactory() ;


}

#endif //DMSSCANNER_HPP__

