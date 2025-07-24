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

   Source File Name = dmsStorageDataCapped.hpp

   Descriptive Name = Data Management wrapper for data type.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/11/2016  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_DATACAPPED_HPP
#define DMSSTORAGE_DATACAPPED_HPP

#include "dmsStorageDataCommon.hpp"
#include "ossMemPool.hpp"

namespace engine
{
#define DMS_INVALID_REC_LOGICALID         -1

<<<<<<< HEAD
=======
// Default size threshold of capped collection is 30GB.
// Default record number threshold is set to 0, which means no limit on that.
// Default size threshold of Rollback Segment collection is 128MB each.
#define DMS_DFT_CAPPEDCL_SIZE             (30 * 1024 * 1024 * 1024LL)
#define DMS_DFT_CAPPEDCL_RECNUM           0

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
#define DMS_INVALID_LOGICALID             (-1)

#pragma pack(1)
   class _LogicalIDToInsert : public SDBObject
   {
   public :
      CHAR  _type ;
      CHAR  _id[4] ; // _id + '\0'
      INT64 _logicalID ;
      _LogicalIDToInsert ()
      {
         _type = (CHAR)NumberLong ;
         _id[0] = '_' ;
         _id[1] = 'i' ;
         _id[2] = 'd' ;
         _id[3] = 0 ;
         _logicalID = DMS_INVALID_LOGICALID ;
         SDB_ASSERT ( sizeof ( _LogicalIDToInsert) == 13,
                      "LogicalIDToInsert should be 13 bytes" ) ;
      }
   } ;
   typedef class _LogicalIDToInsert LogicalIDToInsert ;

   /*
      _idToInsert define
   */
   class _LogicalIDToInsertEle : public BSONElement
   {
   public :
      _LogicalIDToInsertEle( CHAR* x ) : BSONElement((CHAR*) ( x )){}
   } ;
   typedef class _LogicalIDToInsertEle LogicalIDToInsertEle ;
#pragma pack()

   class _pmdEDUCB ;
   class _mthModifier ;

   class _dmsStorageDataCapped : public _dmsStorageDataCommon
   {
      friend class _dmsRBSSUMgr ;
      typedef ossPoolMap<UINT32, UINT32> SIZE_REQ_MAP ;
   public:
      _dmsStorageDataCapped( IStorageService *service,
                             dmsSUDescriptor *suDescriptor,
                             const CHAR* pSuFileName,
                             _IDmsEventHolder *pEventHolder ) ;
      virtual ~_dmsStorageDataCapped() ;

      virtual INT32 popRecord ( dmsMBContext *context,
                                INT64 logicalID,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb,
                                INT8 direction = 1,
                                BOOLEAN byNumber = FALSE ) ;

      INT32 fetch ( dmsMBContext      *context,
                    const dmsRecordID &recordID,
                    BSONObj           &dataRecord,
                    _pmdEDUCB         *cb,
                    BOOLEAN            dataOwned = FALSE,
                    DPS_TRANS_ID      *version = NULL ) ;

      virtual INT32 dumpExtOptions( dmsMBContext *context,
                                    BSONObj &extOptions ) ;

      virtual INT32 setExtOptions ( dmsMBContext * context,
                                    const BSONObj & extOptions ) ;

      virtual OSS_LATCH_MODE getWriteLockType() const
      {
         return EXCLUSIVE ;
      }

<<<<<<< HEAD
=======
      OSS_INLINE BOOLEAN spaceEnough( dmsMBContext *context, UINT32 newSize ) ;
      OSS_INLINE BOOLEAN clDataSpaceEnough( dmsMBContext *clContext, UINT32 newSize ) ;
   protected:
      OSS_INLINE void _extLidAndOffset2RecLid( dmsExtentID extID,
                                               dmsOffset offset,
                                               INT64 &logicalID ) ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   private:
      virtual const CHAR* _getEyeCatcher() const ;

      virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                           dmsCreateCLOptions &options ) ;

      virtual INT32 _onAddCollection( const BSONObj *extOption,
                                      dmsExtentID extOptExtent,
                                      UINT32 extentSize,
                                      UINT16 collectionID ) ;

      virtual void _onAllocExtent( dmsMBContext *context,
                                   dmsExtent * extAddr,
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

      virtual void _finalRecordSize( UINT32 &size,
                                     const dmsRecordData &recordData ) ;

      virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;

<<<<<<< HEAD
      virtual INT32 _checkRecordSpace( dmsMBContext *context,
                                       UINT32 size,
                                       dmsRecordID &foundRID,
                                       _pmdEDUCB *cb ) ;

      virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) ;

=======
      virtual INT32 _allocRecordSpaceByPos( dmsMBContext *context,
                                            UINT32 size,
                                            INT64 position,
                                            dmsRecordID &foundRID,
                                            _pmdEDUCB *cb ) ;

      virtual INT32 _extentInsertRecord( dmsMBContext *context,
                                         dmsExtRW &extRW,
                                         dmsRecordRW &recordRW,
                                         const dmsRecordData &recordData,
                                         UINT32 recordSize,
                                         _pmdEDUCB *cb,
                                         BOOLEAN isInsert = TRUE,
                                         const dmsTransRecordInfo *recordInfo = NULL ) ;

      virtual void _postInsertRecord( dmsMBContext *context,
                                      dmsExtRW &extRW,
                                      dmsRecordRW &recordRW,
                                      const dmsRecordData &recordData,
                                      UINT32 recordSize,
                                      _pmdEDUCB *cb )
      {
         // do nothing
      }

      virtual INT32 _extentUpdatedRecord( dmsMBContext *context,
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

      virtual INT32 _extentRemoveRecord( dmsMBContext *context,
                                         dmsExtRW &extRW,
                                         dmsRecordRW &recordRW,
                                         _pmdEDUCB *cb,
                                         BOOLEAN decCount = TRUE,
                                         const dmsTransRecordInfo *recordInfo = NULL ) ;

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

      virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) ;

      virtual void _onAllocSpaceReady( dmsContext *context, BOOLEAN &doit ) ;

      virtual INT32 _setRecordGlobTransID( dmsMBContext *context,
                                           dmsRecordRW  &recordRW,
                                           _pmdEDUCB    *cb,
                                           BOOLEAN      bSetOvfRecrd ) ;

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      INT32 _parseExtendOptions( const BSONObj *extOptions,
                                 dmsCappedCLOptions &options ) ;

      OSS_INLINE void _updateCLStat( dmsMBStatInfo &mbStat, UINT32 totalSize,
                                     const dmsRecordData &recordData ) ;
      OSS_INLINE void _updateStatInfo( dmsMBContext *context,
                                       UINT32 recordSize,
                                       const dmsRecordData &recordData ) ;

      OSS_INLINE BOOLEAN _numExceedLimit( dmsMBContext *context, UINT32 size ) ;
      OSS_INLINE BOOLEAN _spaceExceedLimit( dmsMBContext *context, UINT32 newSize ) ;
      OSS_INLINE BOOLEAN _overwriteOnExceed( dmsMBContext *context ) ;

      INT32 _limitProcess( dmsMBContext *context, UINT32 sizeReq, pmdEDUCB *cb ) ;

      INT32 _popRecordByLID( dmsMBContext *context, INT64 logicalID,
                             pmdEDUCB *cb, SDB_DPSCB *dpscb,
                             INT8 direction = 1 ) ;

      INT32 _popRecordByNumber( dmsMBContext *context, INT64 number,
                                pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                INT8 direction = 1 ) ;

      INT32 _popRecord( dmsMBContext *context,
                        INT8 direction,
                        pmdEDUCB *cb,
                        SDB_DPSCB *dpscb ) ;
   } ;
   typedef _dmsStorageDataCapped dmsStorageDataCapped ;

   OSS_INLINE void _dmsStorageDataCapped::_updateCLStat( dmsMBStatInfo &mbStat,
                                                         UINT32 totalSize,
                                                         const dmsRecordData &recordData )
   {
      mbStat._totalRecords.inc() ;
      mbStat._rcTotalRecords.inc() ;
      mbStat._totalDataFreeSpace -= totalSize ;
      mbStat._totalOrgDataLen.add( totalSize ) ;
      mbStat._totalDataLen.add( totalSize ) ;
   }

   OSS_INLINE void _dmsStorageDataCapped::_updateStatInfo( dmsMBContext *context,
                                                           UINT32 recordSize,
                                                           const dmsRecordData &recordData )
   {
      _updateCLStat( _mbStatInfo[ context->mbID() ], recordSize, recordData ) ;
   }

   OSS_INLINE BOOLEAN _dmsStorageDataCapped::_spaceExceedLimit( dmsMBContext *context,
                                                                UINT32 newSize )
   {
      return ( context->mb()->_maxSize > 0 ) &&
             ( ( context->mbStat()->_totalOrgDataLen.fetch() + newSize ) >
               (UINT64)( context->mb()->_maxSize ) ) ;
   }

   OSS_INLINE BOOLEAN _dmsStorageDataCapped::clDataSpaceEnough( dmsMBContext *context,
                                                          UINT32 newSize )
   {
      const dmsMBStatInfo *mbStatInfo = getMBStatInfo( context->mbID() ) ;
      SDB_ASSERT( mbStatInfo, "mbStatInfo should not be NULL" ) ;
      // enough data space or has room to grow
      return ( ( (UINT64)mbStatInfo->_totalDataFreeSpace >= newSize ) &&
               spaceEnough( context, newSize )                        &&
               !_numExceedLimit(context, 1 ) ) ;
   }

   OSS_INLINE BOOLEAN _dmsStorageDataCapped::_numExceedLimit( dmsMBContext *context,
                                                              UINT32 newNum )
   {
      return ( context->mb()->_maxRecNum > 0 ) &&
             ( ( context->mbStat()->_totalRecords.fetch() + newNum ) >
               (UINT64)( context->mb()->_maxRecNum ) ) ;
   }

   OSS_INLINE BOOLEAN _dmsStorageDataCapped::_overwriteOnExceed( dmsMBContext *context )
   {
      return context->mb()->_overwrite ;
   }

}

#endif /* DMSSTORAGE_DATACAPPED_HPP */

