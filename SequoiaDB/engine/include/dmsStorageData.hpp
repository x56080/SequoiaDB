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

   Source File Name = dmsStorageData.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_DATA_HPP_
#define DMSSTORAGE_DATA_HPP_

#include "dmsStorageDataCommon.hpp"
#include "dmsEventHandler.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _mthModifier ;

   class _dmsStorageData : public _dmsStorageDataCommon
   {
      friend class _dmsMmapStorageUnit ;
   public:
      _dmsStorageData ( IStorageService *service,
                        dmsSUDescriptor *suDescriptor,
                        const CHAR *pSuFileName,
                        _IDmsEventHolder *pEventHolder ) ;
      virtual ~_dmsStorageData () ;

   public:
      virtual void postLoadExt( dmsMBContext *context,
                                dmsExtent *extAddr,
                                SINT32 extentID ) ;

      virtual INT32 dumpExtOptions( dmsMBContext *context,
                                    BSONObj &extOptions ) ;

      virtual INT32 setExtOptions ( dmsMBContext * context,
                                    const BSONObj & extOptions ) ;

      virtual OSS_LATCH_MODE getWriteLockType() const
      {
         return SHARED ;
      }

   private:
      virtual const CHAR* _getEyeCatcher() const ;

      virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                           dmsCreateCLOptions &options ) ;

      virtual INT32 _onAddCollection( const BSONObj *extOption,
                                      dmsExtentID extOptExtent,
                                      UINT32 extentSize,
                                      UINT16 collectionID ) ;

      virtual void _onAllocExtent( dmsMBContext *context,
                                   dmsExtent *extAddr,
                                   SINT32 extentID ) ;

      virtual INT32 _checkInsertData( const BSONObj &record,
                                      BOOLEAN mustOID,
                                      pmdEDUCB *cb,
                                      dmsRecordData &recordData,
                                      BOOLEAN &memReallocate,
                                      INT64 position ) ;

      virtual INT32 _prepareInsert( const dmsRecordID &recordID,
                                    const dmsRecordData &recordData ) ;

      virtual INT32 _getRecordPosition( dmsMBContext *context,
                                        const dmsRecordID &rid,
                                        const dmsRecordData &recordData,
                                        pmdEDUCB *cb,
                                        INT64 &position ) ;

      virtual INT32 _checkReusePosition( dmsMBContext *context,
                                         const DPS_TRANS_ID &transID,
                                         pmdEDUCB *cb,
                                         INT64 &position,
                                         dmsRecordID &foundRID ) ;

      virtual INT32 _getRecordPosition( const dmsRecordID &rid,
                                        const dmsRecordData &recordData,
                                        INT64 &position ) ;

      virtual INT32 _checkMarkInsert( dmsMBContext *context,
                                      const DPS_TRANS_ID &transID,
                                      const BSONObj &insertObj,
                                      pmdEDUCB *cb,
                                      INT64 &position,
                                      BOOLEAN &markInsert,
                                      dmsRecordID &foundRID,
                                      dmsRecordData &recordData,
                                      dmsRecordRW &recordRW ) ;

      virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;

<<<<<<< HEAD
      virtual INT32 _checkRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;
=======
      virtual void _postInsertRecord( dmsMBContext *context,
                                      dmsExtRW &extRW,
                                      dmsRecordRW &recordRW,
                                      const dmsRecordData &recordData,
                                      UINT32 recordSize,
                                      _pmdEDUCB *cb ) ;

      virtual INT32 _allocRecordSpaceByPos( dmsMBContext *context,
                                            UINT32 size,
                                            INT64 position,
                                            dmsRecordID &foundRID,
                                            _pmdEDUCB *cb ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

      virtual void _finalRecordSize( UINT32 &size,
                                     const dmsRecordData &recordData ) ;

<<<<<<< HEAD
=======
      virtual INT32 _onInsertFail( dmsMBContext *context,
                                   BOOLEAN hasInsert,
                                   dmsRecordID rid,
                                   SDB_DPSCB *dpscb,
                                   ossValuePtr dataPtr,
                                   _pmdEDUCB *cb,
                                   const dmsTransRecordInfo *pInfo ) ;

      virtual INT32 extractData( const dmsMBContext *mbContext,
                                 const dmsRecordRW &recordRW,
                                 _pmdEDUCB *cb,
                                 dmsRecordData &recordData,
                                 BOOLEAN needIncDataRead = TRUE ) ;

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) ;


   private:
      //   must be hold the mb EXCLUSIVE lock in this functions :

      /*
         When recordSize == 0, will not change the delete record size
      */
      INT32 _saveDeletedRecord ( dmsMB *mb,
                                 const dmsRecordID &recordID,
                                 INT32 recordSize = 0 ) ;

      INT32 _saveDeletedRecord ( dmsMB *mb,
                                 const dmsRecordID &rid,
                                 INT32 recordSize,
                                 dmsExtent *extAddr,
                                 dmsDeletedRecord *pRecord,
                                 BOOLEAN isRecycled ) ;

      void  _mapExtent2DelList ( dmsMB *mb, dmsExtent *extAddr,
                                 SINT32 extentID ) ;

      INT32 _freeExtent ( dmsExtentID extentID,
                          INT32 collectionID ) ;

      INT32 _reserveFromDeleteList ( dmsMBContext *context,
                                     UINT32 requiredSize,
                                     dmsRecordID &resultID,
                                     _pmdEDUCB *cb ) ;

      INT32 _truncateCollection ( dmsMBContext *context,
                                  BOOLEAN needChangeCLID = TRUE ) ;

      INT32 _truncateCollectionLoads( dmsMBContext *context ) ;
<<<<<<< HEAD
=======

      INT32 _extentInsertRecord ( dmsMBContext *context,
                                  dmsExtRW &extRW,
                                  dmsRecordRW &recordRW,
                                  const dmsRecordData &recordData,
                                  UINT32 needRecordSize,
                                  _pmdEDUCB *cb,
                                  BOOLEAN isInsert = TRUE,
                                  const dmsTransRecordInfo *recordInfo = NULL ) ;

      // must hold mb exclusive lock
      INT32 _extentRemoveRecord ( dmsMBContext *context,
                                  dmsExtRW &extRW,
                                  dmsRecordRW &recordRW,
                                  _pmdEDUCB *cb,
                                  BOOLEAN decCount = TRUE,
                                  const dmsTransRecordInfo *recordInfo = NULL ) ;

      // must hold mb exclusive lock
      INT32 _extentUpdatedRecord ( dmsMBContext *context,
                                   dmsExtRW &extRW,
                                   dmsRecordRW &recordRW,
                                   const dmsRecordData &recordData,
                                   const BSONObj &newObj,
                                   _pmdEDUCB *cb,
                                   IDmsOprHandler *pHandler,
                                   utilUpdateResult *pResult,
                                   dpsUnqIdxHashArray *pNewUnqIdxHashArray,
                                   dpsUnqIdxHashArray *pOldUnqIdxHashArray,
                                   const ixmIdxHashBitmap &idxHashBitmap ) ;

      // must hold mb exclusive lock
      // set or restore global transID for record ( and the 
      // overflow to record when it is required ). 
      INT32 _setRecordGlobTransID( dmsMBContext *context,
                                   dmsRecordRW  &recordRW,
                                   _pmdEDUCB    *cb,
                                   BOOLEAN       bSetOvfRecrd ) ;

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   } ;
   typedef _dmsStorageData dmsStorageData ;
}

#endif /* DMSSTORAGE_DATANORMAL_HPP */

