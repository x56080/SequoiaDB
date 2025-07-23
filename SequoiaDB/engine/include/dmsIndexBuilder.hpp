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

   Source File Name = dmsIndexBuilder.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_BUILDER_HPP_
#define DMS_INDEX_BUILDER_HPP_

#include "dmsStorageBase.hpp"
#include "ixmKey.hpp"
#include "dmsExtent.hpp"
#include "dmsOprHandler.hpp"
#include "clsRemoteOperator.hpp"
#include "dmsTaskStatus.hpp"
#include "dmsScanner.hpp"

namespace engine
{
   class _dmsMBContext ;
   class _dmsStorageIndex ;
   class _dmsStorageData ;
   class _pmdEDUCB ;
   class _ixmIndexCB ;
   class _dmsMBContext ;

   // 10 ms
   #define DMS_INDEX_WAITBLOCK_INTERVAL       ( 10 )

   /*
      _dmsDupKeyProcessor define
    */
   class _dmsDupKeyProcessor : public SDBObject
   {
   public:
      _dmsDupKeyProcessor()
      {
      }

      virtual ~_dmsDupKeyProcessor()
      {
      }

   public:
      virtual INT32 processDupKeyRecord( _dmsStorageData *suData,
                                         _dmsMBContext *mbContext,
                                         const dmsRecordID &recordID,
                                         ossValuePtr recordDataPtr,
                                         _pmdEDUCB *eduCB ) = 0 ;
   } ;

   typedef class _dmsDupKeyProcessor dmsDupKeyProcessor ;

   class _dmsIndexBuilder: public utilPooledObject
   {
   public:
      _dmsIndexBuilder( _dmsStorageIndex* indexSU,
                        _dmsStorageData* dataSU,
                        _dmsMBContext* mbContext,
                        _pmdEDUCB* eduCB,
                        dmsExtentID indexExtentID,
                        dmsExtentID indexLogicID,
                        dmsDupKeyProcessor *dkProcessor,
                        dmsIdxTaskStatus* pIdxStatus = NULL ) ;
      virtual ~_dmsIndexBuilder() ;
      INT32 build() ;

      void  setOprHandler( IDmsOprHandler *pOprHander ) ;
      void  setWriteResult( utilWriteResult *pResult ) ;

   protected:
      virtual INT32 _build() = 0 ;
      virtual INT32 _onInit() ;

      // make sure the mbContext is locked before call _beforeExtent()/_afterExtent()
      #define _DMS_SKIP_EXTENT 1
      virtual INT32 _beforeExtent() ;
      virtual INT32 _afterExtent() ;

      INT32 _getKeySet( ossValuePtr recordDataPtr, BSONObjSet& keySet ) ;
      INT32 _insertKey( ossValuePtr recordDataPtr, const dmsRecordID &rid, const Ordering& ordering ) ;
      INT32 _insertKey( const ixmKey &key, const dmsRecordID &rid, const Ordering& ordering ) ;
      INT32 _checkIndexAfterLock( INT32 lockType ) ;
      INT32 _mbLockAndCheck( INT32 lockType ) ;

      INT32 _checkInterrupt() ;
      INT32 _createScannerChecker() ;
      void _releaseScannerChecker() ;

   private:
      INT32 _init() ;
      INT32 _finish() ;

   protected:
      _dmsStorageIndex*  _suIndex ;
      _dmsStorageData*   _suData ;
      _dmsMBContext*     _mbContext ;
      _pmdEDUCB*         _eduCB ;
      dmsExtentID        _indexExtentID ;
      dmsExtentID        _indexLID ;
      _ixmIndexCB*       _indexCB ;
      OID                _indexOID ;
      dmsExtentID        _scanExtLID ;
      dmsExtentID        _currentExtentID ;
      dmsExtentID        _lastExtentID ;
      dmsExtRW           _extRW ;
      const dmsExtent*   _extent ;
      BOOLEAN            _unique ;
      BOOLEAN            _dropDups ;

      IRemoteOperator    *_remoteOperator ;

      IDmsOprHandler     *_pOprHandler ;
      utilWriteResult    *_pResult ;
      bson::BufBuilder   _bufBuilder ;
      dmsDupKeyProcessor *_dkProcessor ;
      dmsIdxTaskStatus* _pIdxStatus ;

      // index key generator
      ixmIndexKeyGen     _keyGen ;

      // scanner checker
      IDmsScannerChecker * _checker ;

   public:
      static _dmsIndexBuilder* createInstance( _dmsStorageIndex* indexSU,
                                               _dmsStorageData* dataSU,
                                               _dmsMBContext* mbContext,
                                               _pmdEDUCB* eduCB,
                                               dmsExtentID indexExtentID,
                                               dmsExtentID indexLogicID,
                                               INT32 sortBufferSize,
                                               UINT16 indexType,
                                               IDmsOprHandler *pOprHandler,
                                               utilWriteResult *pResult,
                                               dmsDupKeyProcessor *dkProcessor,
                                               dmsIdxTaskStatus* pIdxStatus = NULL
                                             ) ;

      static void releaseInstance( _dmsIndexBuilder* builder ) ;
   } ;
   typedef class _dmsIndexBuilder dmsIndexBuilder ;
}

#endif /* DMS_INDEX_BUILDER_HPP_ */

