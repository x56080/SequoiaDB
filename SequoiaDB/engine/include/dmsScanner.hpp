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
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "dmsStorageBase.hpp"
#include "dmsStorageData.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include "mthMatchTree.hpp"

using namespace bson ;

namespace engine
{

   class _dmsMBContext ;
   class _dmsStorageData ;
   class _mthMatchTreeContext ;
   class _mthMatchTree ;
   class _rtnIXScanner ;
   class _pmdEDUCB ;
   class _monAppCB ;
   class dpsTransCB ;

   /*
      _dmsScanner define
   */
   class _dmsScanner : public SDBObject
   {
      public:
         _dmsScanner ( _dmsStorageData *su, _dmsMBContext *context,
                       _mthMatchTree *match,
                       DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH ) ;
         virtual ~_dmsScanner () ;

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) = 0 ;
         virtual void  stop () = 0 ;

      protected:
         _dmsStorageData         *_pSu ;
         _dmsMBContext           *_context ;
         _mthMatchTree           *_match ;
         DMS_ACCESS_TYPE         _accessType ;

   } ;
   typedef _dmsScanner dmsScanner ;

   class _dmsTBScanner ;
   /*
      _dmsExtScanner define
   */
   class _dmsExtScanner : public _dmsScanner
   {
      friend class _dmsTBScanner ;
      public:
         _dmsExtScanner ( _dmsStorageData *su, _dmsMBContext *context,
                          _mthMatchTree *match, dmsExtentID curExtentID,
                          DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                          INT64 maxRecords = -1, INT64 skipNum = 0 ) ;
         virtual ~_dmsExtScanner () ;

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

      protected:
         INT32 _firstInit( _pmdEDUCB *cb ) ;
         void _checkMaxRecordsNum( _mthRecordGenerator &generator ) ;

      private:
         INT64                _maxRecords ;
         INT64                _skipNum ;
         dmsExtRW             _extRW ;
         const dmsExtent      *_extent ;
         dmsRecordID          _curRID ;
         dmsRecordRW          _recordRW ;
         const dmsRecord      *_curRecordPtr ;
         dmsOffset            _next ;
         BOOLEAN              _firstRun ;
         dpsTransCB           *_pTransCB ;
         BOOLEAN              _recordXLock ;
         BOOLEAN              _needUnLock ;
         _pmdEDUCB            *_cb ;
   };
   typedef _dmsExtScanner dmsExtScanner ;

   /*
      _dmsTBScanner define
   */
   class _dmsTBScanner : public _dmsScanner
   {
      public:
         _dmsTBScanner ( _dmsStorageData *su, _dmsMBContext *context,
                         _mthMatchTree *match,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1, INT64 skipNum = 0 ) ;
         ~_dmsTBScanner () ;

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) ;
         virtual void  stop () ;

      protected:
         void  _resetExtScanner() ;
         INT32 _firstInit() ;

      private:
         dmsExtScanner              _extScanner ;
         dmsExtentID                _curExtentID ;
         BOOLEAN                    _firstRun ;

   };
   typedef _dmsTBScanner dmsTBScanner ;

   class _dmsIXScanner ;
   /*
      _dmsIXSecScanner define
   */
   class _dmsIXSecScanner : public _dmsScanner
   {
      friend class _dmsIXScanner ;
      public:
         _dmsIXSecScanner ( _dmsStorageData *su, _dmsMBContext *context,
                            _mthMatchTree *match, _rtnIXScanner *scanner,
                            DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                            INT64 maxRecords = -1, INT64 skipNum = 0 ) ;
         virtual ~_dmsIXSecScanner () ;

         void  enableIndexBlockScan( const BSONObj &startKey,
                                     const BSONObj &endKey,
                                     const dmsRecordID &startRID,
                                     const dmsRecordID &endRID,
                                     INT32 direction ) ;

         void  enableCountMode() { _countOnly = TRUE ; }

         INT64 getMaxRecords() const { return _maxRecords ; }
         INT64 getSkipNum () const { return _skipNum ; }

         BOOLEAN eof () const { return _eof ; }

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mhtContext = NULL ) ;
         virtual void  stop () ;

      protected:
         INT32 _firstInit( _pmdEDUCB *cb ) ;
         BSONObj* _getStartKey () ;
         BSONObj* _getEndKey () ;
         dmsRecordID* _getStartRID () ;
         dmsRecordID* _getEndRID () ;
         void _updateMaxRecordsNum( _mthRecordGenerator &generator ) ;

      private:
         INT64                _maxRecords ;
         INT64                _skipNum ;
         dmsRecordID          _curRID ;
         dmsRecordRW          _recordRW ;
         const dmsRecord      *_curRecordPtr ;
         BOOLEAN              _firstRun ;
         dpsTransCB           *_pTransCB ;
         BOOLEAN              _recordXLock ;
         BOOLEAN              _needUnLock ;
         _pmdEDUCB            *_cb ;
         _rtnIXScanner        *_scanner ;
         INT64                _onceRestNum ;
         BOOLEAN              _eof ;

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
   } ;
   typedef _dmsIXSecScanner dmsIXSecScanner ;

   /*
      _dmsIXScanner define
   */
   class _dmsIXScanner : public _dmsScanner
   {
      public:
         _dmsIXScanner ( _dmsStorageData *su, _dmsMBContext *context,
                         _mthMatchTree *match, _rtnIXScanner *scanner,
                         BOOLEAN ownedScanner = FALSE,
                         DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH,
                         INT64 maxRecords = -1, INT64 skipNum = 0 ) ;
         ~_dmsIXScanner () ;

         _rtnIXScanner* getScanner () { return _scanner ; }

      public:
         virtual INT32 advance ( dmsRecordID &recordID,
                                 _mthRecordGenerator &generator,
                                 _pmdEDUCB *cb,
                                 _mthMatchTreeContext *mthContext = NULL ) ;
         virtual void  stop () ;

      protected:
         void  _resetIXSecScanner() ;

      private:
         dmsIXSecScanner            _secScanner ;
         _rtnIXScanner              *_scanner ;
         BOOLEAN                    _firstRun ;
         BOOLEAN                    _eof ;
         BOOLEAN                    _ownedScanner ;

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

}

#endif //DMSSCANNER_HPP__

