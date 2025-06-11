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

   Source File Name = dmsStorageData.cpp

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

#include "dmsStorageData.hpp"
#include "dmsStorageIndex.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "mthModifier.hpp"
#include "dpsOp2Record.hpp"
#include "pdSecure.hpp"

namespace engine
{

   _dmsStorageData::_dmsStorageData( const CHAR *pSuFileName,
                                     dmsStorageInfo *pInfo,
                                     _IDmsEventHolder *pEventHolder )
   : _dmsStorageDataCommon( pSuFileName, pInfo, pEventHolder )
   {
   }

   _dmsStorageData::~_dmsStorageData()
   {
   }

   INT32 _dmsStorageData::_prepareAddCollection( const BSONObj *extOption,
                                                 dmsExtentID &extOptExtent,
                                                 UINT16 &extentPageNum )
   {
      return SDB_OK ;
   }

   INT32 _dmsStorageData::_onAddCollection( const BSONObj *extOption,
                                            dmsExtentID extOptExtent,
                                            UINT32 extentSize,
                                            UINT16 collectionID )
   {
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONALLOCEXTENT, "_dmsStorageData::_onAllocExtent" )
   void _dmsStorageData::_onAllocExtent( dmsMBContext *context,
                                         dmsExtent *extAddr,
                                         SINT32 extentID )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__ONALLOCEXTENT ) ;
      SDB_ASSERT( context, "Context should not be NULL" ) ;
      SDB_ASSERT( extAddr, "Extent address should not be NULL" ) ;

      _mapExtent2DelList( context, extAddr, extentID ) ;

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA__ONALLOCEXTENT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__PREPAREINSERTDATA, "_dmsStorageData::_prepareInsertData" )
   INT32 _dmsStorageData::_prepareInsertData( const BSONObj &record,
                                              BOOLEAN mustOID,
                                              pmdEDUCB *cb,
                                              dmsRecordData &recordData,
                                              BOOLEAN &memReallocate,
                                              INT64 position )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__PREPAREINSERTDATA ) ;
      IDToInsert oid ;
      idToInsertEle oidEle((CHAR*)(&oid)) ;
      CHAR *pMergedData = NULL ;

      try
      {
         // Step 1: Prepare the data, add OID and compress if necessary.
         recordData.setData( record.objdata(), record.objsize(),
                             UTIL_COMPRESSOR_INVALID, TRUE ) ;

         BSONElement ele = record.getField( DMS_ID_KEY_NAME ) ;
         // check ID index for normal update
         // NOTE: for sequoiadb upgrade, if the old data before upgrade
         //       contains invalid _id field, we could not report error,
         //       we need to allow update if _id field is not changed
         if( !cb->isDoReplay() &&
             !cb->isInTransRollback() &&
             !cb->isDoRollback() )
         {
            const CHAR *pCheckErr = "" ;
            if ( !dmsIsRecordIDValid( ele, TRUE, &pCheckErr ) )
            {
               PD_LOG_MSG( PDERROR, "_id is error: %s", pCheckErr ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         // judge must oid
         if ( mustOID && ele.eoo() )
         {
            oid._oid.init() ;
            rc = cb->allocBuff( oidEle.size() + record.objsize(),
                                &pMergedData ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc memory[size:%u] failed, rc: %d",
                       oidEle.size() + record.objsize(), rc ) ;
               goto error ;
            }
            /// copy to new data
            *(UINT32*)pMergedData = oidEle.size() + record.objsize() ;
            ossMemcpy( pMergedData + sizeof(UINT32), oidEle.rawdata(),
                       oidEle.size() ) ;
            ossMemcpy( pMergedData + sizeof(UINT32) + oidEle.size(),
                       record.objdata() + sizeof(UINT32),
                       record.objsize() - sizeof(UINT32) ) ;
            recordData.setData( pMergedData,
                                oidEle.size() + record.objsize(),
                                UTIL_COMPRESSOR_INVALID, TRUE ) ;
            memReallocate = TRUE ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__PREPAREINSERTDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__GETRECPOS, "_dmsStorageData::_getRecordPosition" )
   INT32 _dmsStorageData::_getRecordPosition( const dmsRecordID &rid,
                                              const dmsRecordData &recordData,
                                              INT64 &position )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__GETRECPOS ) ;

      if ( rid.isValid() )
      {
         position = (INT64)( ossPack32To64( (UINT32)( rid._extent ),
                                            (UINT32)( rid._offset ) ) ) ;
      }
      else
      {
         position = -1 ;
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__GETRECPOS, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__CHKMARKINST, "_dmsStorageData::_checkMarkInsert" )
   INT32 _dmsStorageData::_checkMarkInsert( dmsMBContext *context,
                                            const DPS_TRANS_ID &transID,
                                            const BSONObj &insertObj,
                                            pmdEDUCB *cb,
                                            INT64 &position,
                                            BOOLEAN &markInsert,
                                            dmsRecordID &foundRID,
                                            dmsRecordData &recordData,
                                            dmsRecordRW &recordRW )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__CHKMARKINST ) ;

      markInsert = FALSE ;

      /// when is rollback, and the rid is found
      if ( -1 != position &&
           DPS_INVALID_TRANS_ID != transID &&
           cb->isInTransRollback() &&
           !cb->isTakeOverTransRB() )
      {
         // use the given position instead
         ossUnpack32From64( (UINT64)position,
                            (UINT32 &)( foundRID._extent ),
                            (UINT32 &)( foundRID._offset ) ) ;

         markInsert = TRUE ;
         const dmsRecord *pRecord = NULL ;

         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d",
                      rc ) ;

         recordRW = record2RW( foundRID, context->mbID() ) ;

         /// 1. check status
         pRecord = recordRW.readPtr<dmsRecord>() ;
         if ( !pRecord->isDeleting() )
         {
            SDB_ASSERT( cb->getTransExecutor()->isLockEscalated(
                                                      LOCKMGR_TRANS_LOCK ),
                        "should be lock escalated" ) ;
            markInsert = FALSE ;
         }

         context->mbUnlock() ;
      }

      // can not use mark insert, the position should be cleared
      if ( !markInsert && -1 != position )
      {
         position = -1 ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__CHKMARKINST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__DOMARKINST, "_dmsStorageData::_doMarkInsert" )
   INT32 _dmsStorageData::_doMarkInsert( dmsMBContext *context,
                                         pmdEDUCB *cb,
                                         dmsExtRW &extRW,
                                         dmsRecordID &foundRID,
                                         UINT32 dmsRecordSize,
                                         dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__DOMARKINST ) ;

      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      dmsExtent *pExtent = extRW.writePtr<dmsExtent>() ;
      dmsRecordRW recordRW ;
      dmsRecord *pRecord = NULL ;
      dmsRecordID foundDeletedID ;
      dmsExtRW newExtRW ;
      dmsRecordRW newRecordRW ;
      const dmsExtent *pNewExtent = NULL ;
      dmsRecord *pNewRecord = NULL ;

      dmsRecordID ovfRID ;
      dmsExtRW ovfExtRW ;
      dmsRecordRW ovfRW ;
      dmsRecord *pOvfRecord = NULL ;

      recordRW = record2RW( foundRID, context->mbID() ) ;
      pRecord = recordRW.writePtr< dmsRecord >() ;

      if ( pRecord->isOvf() )
      {
         ovfRID = pRecord->getOvfRID() ;
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
         ovfExtRW = extent2RW( ovfRID._extent, context->mbID() ) ;
         ovfRW = record2RW( ovfRID, context->mbID() ) ;
         pOvfRecord = ovfRW.writePtr( 0 ) ;
         SDB_ASSERT( pOvfRecord->isOvt(), "Record must be ovt" ) ;
      }

      /// when not in deleting list, will do nothing in eraseFromDeletingList and return SDB_OK
      rc = eraseFromDeletingList( context, pRecord ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Erase from deleting list for RID(%d,%d) failed, rc: %d",
                 foundRID._extent, foundRID._offset, rc ) ;
         goto error ;
      }

      if ( dmsRecordSize <= pRecord->getSize() )
      {
         pRecord->setData( recordData ) ;
         // increase data write counter for deleting marking
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

         if ( ovfRID.isValid() )
         {
            _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
            pRecord->setNormal() ;
            /// when deleting record not be tracked in overflow record, so don't --overflow records
         }
      }
      else if ( pOvfRecord && dmsRecordSize <= pOvfRecord->getSize() )
      {
         pOvfRecord->setData( recordData ) ;
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;
         /// when deleting record not be tracked in overflow record, so need add it
         ++( context->mbStat()->_totalOverflowRecords ) ;
      }
      else
      {
         // find a free spot from delete list
         rc = _reserveFromDeleteList ( context, dmsRecordSize,
                                       foundDeletedID, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to reserve delete record, rc: %d", rc ) ;
            goto error ;
         }
         newExtRW = extent2RW( foundDeletedID._extent, context->mbID() ) ;
         newRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
         pNewExtent = newExtRW.readPtr<dmsExtent>() ;
         pNewRecord = newRecordRW.writePtr() ;

         if ( !pNewExtent->validate( context->mbID() ) )
         {
            PD_LOG ( PDERROR, "Invalid extent is detected for RID(%d,%d), rc: %d",
                     foundDeletedID._extent, foundDeletedID._offset, rc ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         // pass FALSE to addIntoList so that we don't add the record into
         // target extent's list
         rc = _extentInsertRecord ( context, newExtRW, newRecordRW,
                                    recordData, dmsRecordSize,
                                    cb, FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to append record due to %d", rc ) ;
            goto error ;
         }

         _postInsertRecord( context, newExtRW, newRecordRW, recordData,
                            dmsRecordSize, cb ) ;

         // set remote record as overflowed to
         pNewRecord->setOvt() ;
         pRecord->setOvf() ;
         pRecord->setOvfRID( foundDeletedID ) ;
         /// when deleting record not be tracked in overflow record, so need add it
         ++( context->mbStat()->_totalOverflowRecords ) ;

         if ( ovfRID.isValid() )
         {
            _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
         }
      }

      pRecord->unsetDeleting() ; /// unset deleting
      ++( pExtent->_recCount ) ;
      _increaseMBStat( context->mbStat()->_clUniqueID, context->mbStat(), cb ) ;
      context->mbStat()->_totalDataLen += recordData.len() ;
      context->mbStat()->_totalOrgDataLen += recordData.orgLen() ;

#if defined (_DEBUG)
      PD_LOG( PDDEBUG, "Mark insert for record (extent: %d; offset: %d) "
               "in collection [%s.%s] to rollback transaction [%s]",
               foundRID._extent, foundRID._offset,
               getSuName(), context->mbStat()->_collectionName,
               dpsTransIDToString( DPS_TRANS_GET_ID( cb->getTransID() ) ).c_str() ) ;
#endif
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__DOMARKINST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ALLOCRECORDSPACE, "_dmsStorageData::_allocRecordSpace" )
   INT32 _dmsStorageData::_allocRecordSpace( dmsMBContext *context,
                                             UINT32 size,
                                             dmsRecordID &foundRID,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__ALLOCRECORDSPACE ) ;

      rc = _reserveFromDeleteList( context, size, foundRID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Reserve delete record failed, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__ALLOCRECORDSPACE, rc ) ;
      return rc ;
   error:
      goto done;
   }

   INT32 _dmsStorageData::_allocRecordSpaceByPos( dmsMBContext *context,
                                                  UINT32 size,
                                                  INT64 position,
                                                  dmsRecordID &foundRID,
                                                  pmdEDUCB *cb )
   {
      return SDB_OPERATION_INCOMPATIBLE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__POSTINSERTRECORD, "_dmsStorageData::_postInsertRecord" )
   void _dmsStorageData::_postInsertRecord( dmsMBContext *context,
                                            dmsExtRW &extRW,
                                            dmsRecordRW &recordRW,
                                            const dmsRecordData &recordData,
                                            UINT32 recordSize,
                                            _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__POSTINSERTRECORD ) ;

      // and then need to check if we need to split deleted record
      dmsRecord* pRecord = recordRW.writePtr( recordSize ) ;
      dmsOffset  myOffset = pRecord->getMyOffset() ;
      SDB_ASSERT( pRecord->getSize() >= recordSize, "invalid record size" ) ;
      UINT32 remainSize = pRecord->getSize() - recordSize ;

      if ( remainSize > DMS_MIN_RECORD_SZ )
      {
         // to avoid small deleted record which can not be reused by average
         // size of records in the same collection, we only split the record if
         // the remain size can at least save the record with average size,
         // or current record ( scale down to 0.8x )
         UINT32 avgDataSize = context->mbStat()->getAvgDataSize() ;
         UINT32 minRemainSize = ( 0 == avgDataSize ) ?
                                ( recordSize ) :
                                ( OSS_MIN( recordSize, avgDataSize ) ) ;
         // scale down to 0.8
         minRemainSize = (UINT32)( (FLOAT64)( minRemainSize ) *
                                   DMS_REMAIN_SIZE_RATIO ) ;
         if ( remainSize > minRemainSize )
         {
            // original offset+new size = new delete offset
            dmsOffset remainOffset = myOffset + recordSize ;
            // original size - new size = new delete size
            dmsRecordID remainRID = recordRW.getRecordID() ;
            remainRID._offset = remainOffset ;
            INT32 rc = _saveDeletedRecord( context->mb(), remainRID, remainSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDWARNING, "Failed to save deleted record, rc: %d", rc ) ;
            }
            else
            {
               // set the original place with new dmsrecordSize
               pRecord->setSize( recordSize ) ;
            }
         }
      }
      // if the leftover space is not good enough for a min_record, then we
      // don't change the record size

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA__POSTINSERTRECORD ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD, "_dmsStorageData::_extentUpdatedRecord" )
   INT32 _dmsStorageData::_extentUpdatedRecord( dmsMBContext *context,
                                                dmsExtRW &extRW,
                                                dmsRecordRW &recordRW,
                                                const dmsRecordData &recordData,
                                                const BSONObj &newObj,
                                                _pmdEDUCB *cb,
                                                IDmsOprHandler *pHandler,
                                                utilUpdateResult *pResult,
                                                dpsUnqIdxHashArray *pNewUnqIdxHashArray,
                                                dpsUnqIdxHashArray *pOldUnqIdxHashArray,
                                                const ixmIdxHashBitmap &idxHashBitmap )
   {
      INT32 rc                     = SDB_OK ;
      UINT32 dmsRecordSize         = 0 ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD ) ;
      monAppCB * pMonAppCB         = cb ? cb->getMonAppCB() : NULL ;
      dmsCompressorEntry *compressorEntry = &_compressorEntry[context->mbID()] ;
      const dmsExtent *pExtent     = NULL ;
      dmsRecord *pRecord           = NULL ;

      dmsRecordID ovfRID ;
      dmsExtRW ovfExtRW ;
      dmsRecordRW ovfRW ;
      dmsRecord *pOvfRecord        = NULL ;
      dmsRecordData newRecordData ;

      BOOLEAN needUndoIndex        = FALSE ;
      UINT32 headerSize            = DMS_RECORD_METADATA_SZ ;

      _sdbRemoteOpCtrlAssist ctrlAssist( cb->getRemoteOpCtrl() ) ;

      SDB_ASSERT ( !recordData.isEmpty(), "recordData can't be empty" ) ;

      // Check the new object size
      if ( newObj.objsize() + headerSize > DMS_RECORD_USER_MAX_SZ )
      {
         PD_LOG ( PDERROR, "record is too big: %d", newObj.objsize() ) ;
         rc = SDB_DMS_RECORD_TOO_BIG ;
         goto error ;
      }

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      try
      {
         pExtent = extRW.readPtr<dmsExtent>() ;
         pRecord = recordRW.writePtr( 0 ) ;

         headerSize = pRecord->getHeaderSize();
         // double check
         if ( newObj.objsize() + headerSize > DMS_RECORD_USER_MAX_SZ )
         {
            PD_LOG ( PDERROR, "record is too big: %d", newObj.objsize() ) ;
            rc = SDB_DMS_RECORD_TOO_BIG ;
            goto error ;
         }

         newRecordData.setData( newObj.objdata(), newObj.objsize(),
                                UTIL_COMPRESSOR_INVALID, TRUE ) ;
         dmsRecordSize = newRecordData.len() ;

         // compress data
         if ( compressorEntry->ready() )
         {
            INT32 compressedDataSize     = 0 ;
            const CHAR *compressedData   = NULL ;
            UINT8 compressRatio          = 0 ;
            rc = dmsCompress( cb, compressorEntry,
                              newObj, NULL, 0,
                              &compressedData, &compressedDataSize,
                              compressRatio ) ;
            // Compression is valid and ratio is less the threshold
            if ( SDB_OK == rc &&
                 compressedDataSize + sizeof(UINT32) < newRecordData.orgLen() &&
                 compressRatio < UTIL_COMPRESSOR_DFT_MIN_RATIO )
            {
               // 4 bytes len + compressed record
               dmsRecordSize = compressedDataSize + sizeof(UINT32) ;
               PD_TRACE2 ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD,
                           PD_PACK_STRING ( "size after compress" ),
                           PD_PACK_UINT ( dmsRecordSize ) ) ;

               // set the compression data
               newRecordData.setData( compressedData, compressedDataSize,
                                      compressorEntry->getCompressorType(),
                                      FALSE ) ;
            }
         }

         // add metadata to size
         dmsRecordSize += headerSize ;
         {
            // before moving on, let's first make sure the new object doesn't
            // violate any index unique rule
            BSONObj oriObj( recordData.data() ) ;
            BSONObj newObj( newRecordData.orgData() ) ;

            // check ID index for normal update
            // NOTE: for sequoiadb upgrade, if the old data before upgrade
            //       contains invalid _id field, we could not report error,
            //       we need to allow update if _id field is not changed
            if( !cb->isDoReplay() &&
                !cb->isInTransRollback() &&
                !cb->isDoRollback() )
            {
               BSONElement newId = newObj.getField( DMS_ID_KEY_NAME ) ;
               const CHAR *pCheckErr = "" ;
               if( !dmsIsRecordIDValid( newId, TRUE, &pCheckErr ) )
               {
                  if( Array == newId.type() &&
                      0 == newId.woCompare( oriObj.getField( DMS_ID_KEY_NAME ) ) )
                  {
                  }
                  else
                  {
                     PD_LOG_MSG( PDERROR, "_id is error: %s", pCheckErr ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
               }
            }
            rc = _pIdxSU->indexesUpdate( context, pExtent->_logicID,
                                         oriObj, newObj,
                                         recordRW.getRecordID(),
                                         cb, FALSE, pHandler, idxHashBitmap,
                                         pResult,
                                         pNewUnqIdxHashArray,
                                         pOldUnqIdxHashArray ) ;
            needUndoIndex = TRUE ;
            if ( rc )
            {
               PD_LOG ( PDWARNING, "Failed to update object(%s) index, rc: %d",
                        PD_SECURE_OBJ( newObj ), rc ) ;
               goto error ;
            }
         }

         if ( pRecord->isOvf() )
         {
            ovfRID = pRecord->getOvfRID() ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
            ovfExtRW = extent2RW( ovfRID._extent, context->mbID() ) ;
            ovfRW = record2RW( ovfRID, context->mbID() ) ;
            pOvfRecord = ovfRW.writePtr( 0 ) ;
            SDB_ASSERT( pOvfRecord->isOvt(), "Record must be ovt" ) ;
         }

         // if the current space is big enough for the whole record,
         // let's put it here and return rightaway
         if ( dmsRecordSize <= pRecord->getSize() )
         {
            pRecord->setData( newRecordData ) ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

            if ( ovfRID.isValid() )
            {
               _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
               pRecord->setNormal() ;
               /// for old version, the ovf-record not be tracked, so it needs to be protected
               /// to prevent its value from being less than 0
               if ( context->mbStat()->_totalOverflowRecords > 0 )
               {
                  --( context->mbStat()->_totalOverflowRecords ) ;
               }
            }
            /// sub the remove data info
            //if the record has compresssed,the orgLen mean the record size
            //in DB,len mean the uncompress size. So when we substract the
            //size,we should swap them.
            context->mbStat()->_lastCompressRatio =
               (UINT8)( newRecordData.getCompressRatio() * 100 ) ;
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            context->mbStat()->_totalDataLen += newRecordData.len() ;
            context->mbStat()->_totalOrgDataLen += newRecordData.orgLen() ;
            goto done ;
         }
         else if ( pOvfRecord && dmsRecordSize <= pOvfRecord->getSize() )
         {
            pOvfRecord->setData( newRecordData ) ;
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;
            /// sub the remove data info
            //if the record has compresssed,the orgLen mean the record size
            //in DB,len mean the uncompress size. So when we substract the
            //size,we should swap them.
            context->mbStat()->_lastCompressRatio =
               (UINT8)( newRecordData.getCompressRatio() * 100 ) ;
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            context->mbStat()->_totalDataLen += newRecordData.len() ;
            context->mbStat()->_totalOrgDataLen += newRecordData.orgLen() ;
            goto done ;
         }
         // over-flow recrod
         else
         {
            dmsRecordID foundDeletedID ;
            dmsExtRW newExtRW ;
            dmsRecordRW newRecordRW ;
            const dmsExtent *pNewExtent = NULL ;
            dmsRecord *pNewRecord = NULL ;

            dmsRecordSize -= headerSize ;
            // get the recordsize that we have to allocate
            _overflowSize( dmsRecordSize ) ;
            dmsRecordSize += headerSize ;
            // record is ALWAYS 4 bytes aligned
            dmsRecordSize = OSS_MIN( DMS_RECORD_MAX_SZ,
                                     ossAlignX ( dmsRecordSize, 4 ) ) ;

            // find a free spot from delete list
            rc = _reserveFromDeleteList ( context, dmsRecordSize,
                                          foundDeletedID, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserve delete record, rc: %d", rc ) ;
               goto error ;
            }
            newExtRW = extent2RW( foundDeletedID._extent, context->mbID() ) ;
            newRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
            pNewExtent = newExtRW.readPtr<dmsExtent>() ;
            pNewRecord = newRecordRW.writePtr() ;

            if ( !pNewExtent->validate( context->mbID() ) )
            {
               rc = SDB_SYS ;
               PD_LOG ( PDERROR, "Invalid extent is detected for RID(%d,%d), rc: %d",
                        foundDeletedID._extent, foundDeletedID._offset, rc ) ;
               goto error ;
            }
            // pass FALSE to addIntoList so that we don't add the record into
            // target extent's list
            rc = _extentInsertRecord ( context, newExtRW, newRecordRW,
                                       newRecordData, dmsRecordSize,
                                       cb, FALSE ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to append record due to %d", rc ) ;
               goto error ;
            }

            _postInsertRecord( context, newExtRW, newRecordRW, newRecordData,
                               dmsRecordSize, cb ) ;

            // set remote record as overflowed to
            pNewRecord->setOvt() ;
            pRecord->setOvf() ;
            pRecord->setOvfRID( foundDeletedID ) ;
            if ( ovfRID.isValid() )
            {
               // overflowed record removal is done here, and it will mark the
               // segment dirty in the function
               _extentRemoveRecord( context, ovfExtRW, ovfRW, cb, FALSE ) ;
            }
            else
            {
               ++( context->mbStat()->_totalOverflowRecords ) ;
            }

            /// sub the remove data info
            //if the record has compresssed,the orgLen mean the record size
            //in DB,len mean the uncompress size. So when we substract the
            //size,we should swap them.
            context->mbStat()->_totalDataLen -= recordData.orgLen() ;
            context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            // NOTE: last compress ratio is updated with insert of overflow-to record
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTUPDATERECORD, rc ) ;
      return rc ;
   error :
      if( needUndoIndex )
      {
         ctrlAssist.switchToUndo() ;
         BSONObj oriObj( recordData.data() ) ;
         BSONObj newObj( newRecordData.orgData() ) ;
         // rollback the change on index by switching obj and oriObj
         INT32 rc1 = _pIdxSU->indexesUpdate( context, pExtent->_logicID,
                                             newObj, oriObj,
                                             recordRW.getRecordID(),
                                             cb, TRUE, NULL, idxHashBitmap ) ;
         if ( rc1 )
         {
            if ( !ctrlAssist.isUndoFinished() )
            {
               // undo is not finished
               if ( SDB_OK == cb->getTransRC() )
               {
                  cb->setTransRC( rc ) ;
               }
            }
            PD_LOG ( PDERROR, "Failed to rollback update due to rc %d", rc1 ) ;
         }
      }

      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST, "_dmsStorageData::_reserveFromDeleteList" )
   INT32 _dmsStorageData::_reserveFromDeleteList( dmsMBContext *context,
                                                  UINT32 requiredSize,
                                                  dmsRecordID &resultID,
                                                  pmdEDUCB * cb )
   {
      INT32 rc                      = SDB_OK ;
      UINT8  deleteRecordSlot       = 0 ;
      const static INT32 s_maxSearch = 3 ;

      INT32  j                      = 0 ;
      INT32  i                      = 0 ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      dpsTransRetInfo retInfo ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST ) ;
      PD_TRACE1 ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST,
                  PD_PACK_UINT ( requiredSize ) ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      deleteRecordSlot = dmsMBGetSpaceSlot( requiredSize ) ;
      SDB_ASSERT( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      if ( deleteRecordSlot >= dmsMB::_max )
      {
         PD_LOG( PDERROR, "Invalid record size: %u", requiredSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   retry:
      rc = SDB_DMS_NOSPC ;
      try
      {
         for ( j = deleteRecordSlot ; j < dmsMB::_max ; ++j )
         {
            dmsRecordID foundDeletedID  ;
            dmsRecordRW preRW ;
            BOOLEAN startFromHead = FALSE ;

            // if searching the slot of the last search position,
            // we can try to start from last search position
            if ( j == context->mbStat()->_lastSearchSlot )
            {
               if ( j == deleteRecordSlot )
               {
                  // it is the first loop, we can use last search position
                  dmsRecordID lastRID = context->mbStat()->_lastSearchRID ;
                  if ( lastRID.isValid() )
                  {
                     dmsRecordRW lastRW = record2RW( lastRID, context->mbID() ) ;
                     const dmsDeletedRecord *pLast =
                                             lastRW.readPtr<dmsDeletedRecord>() ;
                     SDB_ASSERT( pLast->isDeleted(),
                                 "last search position should be deleted" ) ;
                     if ( pLast->isDeleted() )
                     {
                        foundDeletedID = pLast->getNextRID() ;
                        if ( foundDeletedID.isValid() )
                        {
                           preRW = lastRW ;
                        }
                     }
                     else
                     {
                        // the last search position is not deleted any more,
                        // just clear last search info
                        context->mbStat()->_lastSearchSlot = dmsMB::_max ;
                        context->mbStat()->_lastSearchRID.reset() ;
                     }
                  }
               }
               else
               {
                  // not the first loop, if we start from the search position,
                  // the header record of this slot may never be touched, so we
                  // clear the last search position, and start from the header
                  context->mbStat()->_lastSearchSlot = dmsMB::_max ;
                  context->mbStat()->_lastSearchRID.reset() ;
               }
            }

            if ( foundDeletedID.isNull() )
            {
               // get the first delete record from delete list
               foundDeletedID = context->mb()->_deleteList[j] ;
               // mark start from head
               startFromHead = TRUE ;
            }

            for ( i = 0 ; i < s_maxSearch ; ++i )
            {
               dmsRecordRW delRecordRW ;
               const dmsDeletedRecord* pRead = NULL ;

               if ( foundDeletedID.isNull() )
               {
                  // if we don't get a valid record id
                  if ( startFromHead )
                  {
                     // we already started from head, break to get next slot
                     break ;
                  }
                  else
                  {
                     // we started from middle, try restart
                     foundDeletedID = context->mb()->_deleteList[j] ;
                     preRW = dmsRecordRW() ;
                     // mark start from head ( we only restart once )
                     startFromHead = TRUE ;
                  }
               }

               delRecordRW = record2RW( foundDeletedID, context->mbID() ) ;
               pRead = delRecordRW.readPtr<dmsDeletedRecord>() ;

               // once the extent is valid, let's check the record is deleted
               // and got sufficient size for us
               if( pRead->isDeleted() && pRead->getSize() >= requiredSize )
               {
                  if ( !isTransSupport( context ) ||
                       SDB_OK == pTransCB->transLockTestX( cb, _logicalCSID,
                                                           context->mbID(),
                                                           &foundDeletedID,
                                                           &retInfo,
                                                           NULL,
                                                           FALSE ) )
                  {
                     if ( preRW.isEmpty() )
                     {
                        // it's just the first one from delete list, let's get it
                        context->mb()->_deleteList[j] = pRead->getNextRID() ;
                        // save the last search position
                        if ( j == deleteRecordSlot )
                        {
                           context->mbStat()->_lastSearchSlot = deleteRecordSlot ;
                           context->mbStat()->_lastSearchRID.reset() ;
                        }
                     }
                     else
                     {
                        dmsDeletedRecord *preWrite =
                           preRW.writePtr<dmsDeletedRecord>() ;
                        // we need to link the previous delete record to the next
                        preWrite->setNextRID( pRead->getNextRID() ) ;
                        // save the last search position
                        if ( j == deleteRecordSlot )
                        {
                           context->mbStat()->_lastSearchSlot = deleteRecordSlot ;
                           context->mbStat()->_lastSearchRID = preRW.getRecordID() ;
                        }
                     }

                     // change extent free space
                     dmsExtRW rw = extent2RW( foundDeletedID._extent,
                                              context->mbID() ) ;
                     dmsExtent *pExtent = rw.writePtr<dmsExtent>() ;
                     pExtent->_freeSpace -= pRead->getSize() ;
                     context->mbStat()->_totalDataFreeSpace -= pRead->getSize() ;

                     resultID = foundDeletedID ;
                     rc = SDB_OK ;
                     goto done ;
                  }
                  else
                  {
                     // can't increase i counter
                     --i ;
                  }
               }

               //for some reason this slot can't be reused, let's get to the next
               preRW = delRecordRW ;
               foundDeletedID = pRead->getNextRID() ;
            }

            // save the last search position
            if ( j == deleteRecordSlot )
            {
               if ( preRW.isEmpty() )
               {
                  // has no previous record, we start the next search from
                  // the header of delete list anywat
                  // so no need to save the last search position
                  context->mbStat()->_lastSearchSlot = dmsMB::_max ;
                  context->mbStat()->_lastSearchRID.reset() ;
               }
               else
               {
                  // has previous record, we can start the next search from
                  // this previous record
                  // so save the last search position with previous record
                  context->mbStat()->_lastSearchSlot = deleteRecordSlot ;
                  context->mbStat()->_lastSearchRID = preRW.getRecordID() ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // no space, need to allocate extent
      {
         UINT32 expandSize = requiredSize << DMS_RECORDS_PER_EXTENT_SQUARE ;
         if ( expandSize > DMS_BEST_UP_EXTENT_SZ )
         {
            expandSize = requiredSize < DMS_BEST_UP_EXTENT_SZ ?
                         DMS_BEST_UP_EXTENT_SZ : requiredSize ;
         }
         UINT32 reqPages = ( expandSize + DMS_EXTENT_METADATA_SZ +
                             pageSize() - 1 ) >> pageSizeSquareRoot() ;
         if ( reqPages > segmentPages() )
         {
            reqPages = segmentPages() ;
         }
         if ( reqPages < 1 )
         {
            reqPages = 1 ;
         }

         rc = _allocateExtent( context, reqPages, TRUE, FALSE, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Unable to allocate %d pages extent to the "
                      "collection, rc: %d", reqPages, rc ) ;
         goto retry ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__RESERVEFROMDELETELIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD, "_dmsStorageData::_saveDeletedRecord" )
   INT32 _dmsStorageData::_saveDeletedRecord( dmsMB *mb,
                                              const dmsRecordID &rid,
                                              INT32 recordSize,
                                              dmsExtent *extAddr,
                                              dmsDeletedRecord *pRecord,
                                              BOOLEAN isRecycle )
   {
      UINT8 deleteRecordSlot = 0 ;
      BOOLEAN isSaved = FALSE ;
      UINT16 mbID = mb->_blockID ;
      dmsMBStatInfo &mbStatInfo = _mbStatInfo[ mbID ] ;

      SDB_ASSERT( extAddr && pRecord, "NULL Pointer" ) ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD ) ;
      PD_TRACE6 ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD,
                  PD_PACK_STRING ( "offset" ),
                  PD_PACK_INT ( rid._offset ),
                  PD_PACK_STRING ( "recordSize" ),
                  PD_PACK_INT ( recordSize ),
                  PD_PACK_STRING ( "extentID" ),
                  PD_PACK_INT ( rid._extent ) ) ;

      // assign flags to the memory
      pRecord->setDeleted() ;
      if ( recordSize > 0 )
      {
         pRecord->setSize( recordSize ) ;
      }
      else
      {
         recordSize = pRecord->getSize() ;
      }
      pRecord->setMyOffset( rid._offset ) ;

      // change free space
      extAddr->_freeSpace += recordSize ;
      mbStatInfo._totalDataFreeSpace += recordSize ;

      // let's count which delete slots it fits
      deleteRecordSlot = dmsMBGetSpaceSlot( recordSize ) ;
      // make sure we don't mis calculated it
      SDB_ASSERT ( deleteRecordSlot < dmsMB::_max, "Invalid record size" ) ;

      if ( isRecycle &&
           mbStatInfo._lastSearchSlot == deleteRecordSlot &&
           mbStatInfo._lastSearchRID.isValid() )
      {
         // insert the current record to the last search position,
         // so the next search can check from this position
         dmsRecordRW lastRecordRW = record2RW( mbStatInfo._lastSearchRID,
                                               mbID ) ;
         dmsDeletedRecord* lastRecord =
                                 lastRecordRW.writePtr<dmsDeletedRecord>() ;
         SDB_ASSERT( lastRecord->isDeleted(),
                     "last search position should be deleted" ) ;
         if ( lastRecord->isDeleted() )
         {
            pRecord->setNextRID( lastRecord->getNextRID() ) ;
            lastRecord->setNextRID( rid ) ;
            isSaved = TRUE ;
         }
      }

      if ( !isSaved )
      {
         // set the first matching delete slot to the
         // next rid for the deleted record
         pRecord->setNextRID( mb->_deleteList [ deleteRecordSlot ] ) ;
         // Then assign MB delete slot to the extent and offset
         mb->_deleteList[ deleteRecordSlot ] = rid ;
      }

      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1, "_dmsStorageData::_saveDeletedRecord" )
   INT32 _dmsStorageData::_saveDeletedRecord( dmsMB * mb,
                                              const dmsRecordID &recordID,
                                              INT32 recordSize )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW rw ;
      dmsRecordRW recordRW ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1 ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1,
                  PD_PACK_INT ( recordID._extent ),
                  PD_PACK_INT ( recordID._offset ) ) ;
      if ( recordID.isNull() )
      {
         rc = SDB_INVALIDARG ;
         goto done ;
      }

      rw = extent2RW( recordID._extent, mb->_blockID ) ;
      recordRW = record2RW( recordID, mb->_blockID ) ;

      try
      {
         dmsExtent *pExtent = rw.writePtr<dmsExtent>() ;
         dmsDeletedRecord* pRecord = recordRW.writePtr<dmsDeletedRecord>() ;
         rc = _saveDeletedRecord ( mb, recordID, recordSize,
                                   pExtent, pRecord, TRUE ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__SAVEDELETEDRECORD1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST, "_dmsStorageData::_mapExtent2DelList" )
   void _dmsStorageData::_mapExtent2DelList( dmsMBContext *context, dmsExtent *extAddr,
                                             SINT32 extentID )
   {
      INT32 extentSize         = 0 ;
      INT32 extentUseableSpace = 0 ;
      INT32 deleteRecordSize   = 0 ;
      dmsOffset recordOffset   = 0 ;
      INT32 curUseableSpace    = 0 ;

      dmsMB *mb = context->mb() ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST ) ;

      if ( (UINT32)extAddr->_freeSpace < DMS_MIN_RECORD_SZ )
      {
         if ( extAddr->_freeSpace != 0 )
         {
            PD_LOG( PDINFO, "Collection[%s, mbID: %d]'s extent[%d] free "
                    "space[%d] is less than min record size[%d]",
                    context->mbStat()->_collectionName, mb->_blockID, extentID,
                    extAddr->_freeSpace, DMS_MIN_RECORD_SZ ) ;
         }
         goto done ;
      }

      // calculate the delete record size we need to use
      extentSize          = extAddr->_blockSize << pageSizeSquareRoot() ;
      extentUseableSpace  = extAddr->_freeSpace ;
      extAddr->_freeSpace = 0 ;

      // make sure the delete record is not greater 16MB
      deleteRecordSize    = OSS_MIN ( extentUseableSpace,
                                      DMS_RECORD_MAX_SZ ) ;
      // place first record offset
      recordOffset        = extentSize - extentUseableSpace ;
      curUseableSpace     = extentUseableSpace ;

      /// extentUseableSpace > 16MB
      while ( curUseableSpace - deleteRecordSize >=
              (INT32)DMS_MIN_DELETEDRECORD_SZ )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, deleteRecordSize,
                             extAddr, rRW.writePtr<dmsDeletedRecord>(),
                             FALSE ) ;
         curUseableSpace -= deleteRecordSize ;
         recordOffset += deleteRecordSize ;
      }
      /// 16MB < curUseableSpace < 16MB + DMS_MIN_DELETEDRECORD_SZ
      if ( curUseableSpace > deleteRecordSize )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, DMS_PAGE_SIZE4K,
                             extAddr, rRW.writePtr<dmsDeletedRecord>(),
                             FALSE ) ;
         curUseableSpace -= DMS_PAGE_SIZE4K ;
         recordOffset += DMS_PAGE_SIZE4K ;
      }
      /// 0 < curUseableSpace < 16MB
      if ( curUseableSpace > 0 )
      {
         dmsRecordID rid( extentID, recordOffset ) ;
         dmsRecordRW rRW = record2RW( rid, mb->_blockID ) ;
         _saveDeletedRecord( mb, rid, curUseableSpace,
                             extAddr, rRW.writePtr<dmsDeletedRecord>(),
                             FALSE ) ;
      }

      // correct check
      SDB_ASSERT( extentUseableSpace == extAddr->_freeSpace,
                  "Extent[%d] free space invalid" ) ;
   done :
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATA__MAPEXTENT2DELLIST ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD, "_dmsStorageData::_extentInsertRecord" )
   INT32 _dmsStorageData::_extentInsertRecord( dmsMBContext *context,
                                               dmsExtRW &extRW,
                                               dmsRecordRW &recordRW,
                                               const dmsRecordData &recordData,
                                               UINT32 needRecordSize,
                                               _pmdEDUCB *cb,
                                               BOOLEAN isInsert )
   {
      INT32 rc                         = SDB_OK ;
      monAppCB * pMonAppCB             = cb ? cb->getMonAppCB() : NULL ;
      dmsRecord* pRecord               = NULL ;
      dmsOffset  myOffset              = DMS_INVALID_OFFSET ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      pRecord = recordRW.writePtr( needRecordSize ) ;
      myOffset = pRecord->getMyOffset() ;
      // first we need to check if the delete record is large enough
      if ( pRecord->getSize() < needRecordSize )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !recordData.isCompressed()
                && recordData.len() < DMS_MIN_RECORD_DATA_SZ )
      {
         PD_LOG( PDERROR, "Bson obj size[%d] is invalid",
                 recordData.len() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // set to normal status
      pRecord->setNormal() ;
      pRecord->resetAttr() ;

      // then for the original location we set new record header and copy data
      pRecord->setData( recordData ) ;

      pRecord->setNextOffset( DMS_INVALID_OFFSET ) ;
      pRecord->setPrevOffset( DMS_INVALID_OFFSET ) ;

      // increase write counter
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      // no need to change offset
      if ( isInsert )
      {
         dmsExtent *extent       = extRW.writePtr<dmsExtent>() ;
         dmsOffset   offset      = extent->_lastRecordOffset ;
         // finally add the record into list
         extent->_recCount++ ;
         _increaseMBStat( context->mbStat()->_clUniqueID, context->mbStat(), cb ) ;
         // if there is last record in the extent
         if ( DMS_INVALID_OFFSET != offset )
         {
            // if there is already record in the extent
            dmsRecordRW preRW = record2RW( dmsRecordID( extRW.getExtentID(),
                                                        offset ),
                                           context->mbID() ) ;
            dmsRecord *preRecord = preRW.writePtr() ;
            // set the next of previous point to the new record
            preRecord->setNextOffset( myOffset ) ;
            // set the previous of current points to the original last
            pRecord->setPrevOffset( offset ) ;
         }
         extent->_lastRecordOffset = myOffset ;
         // then check for first record in extent
         if ( DMS_INVALID_OFFSET == extent->_firstRecordOffset )
         {
            // we only change it when it points to nothing
            extent->_firstRecordOffset = myOffset ;
         }
      }

      context->mbStat()->_lastCompressRatio =
         (UINT8)( recordData.getCompressRatio() * 100 ) ;
      context->mbStat()->_totalOrgDataLen += recordData.orgLen() ;
      context->mbStat()->_totalDataLen += recordData.len() ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTINSERTRECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD, "_dmsStorageData::_extentRemoveRecord" )
   INT32 _dmsStorageData::_extentRemoveRecord( dmsMBContext *context,
                                               dmsExtRW &extRW,
                                               dmsRecordRW &recordRW,
                                               _pmdEDUCB *cb,
                                               BOOLEAN decCount )
   {
      INT32 rc              = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD ) ;
      monAppCB * pMonAppCB  = cb ? cb->getMonAppCB() : NULL ;

      dmsExtent *pExtent = NULL ;
      const dmsRecord *pRecord = NULL ;
      dmsRecordID rid = recordRW.getRecordID() ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      pExtent = extRW.writePtr<dmsExtent>() ;
      pRecord = recordRW.readPtr() ;

      // not over-flow to record
      if ( !pRecord->isOvt() )
      {
         dmsOffset prevRecordOffset = pRecord->getPrevOffset() ;
         dmsOffset nextRecordOffset = pRecord->getNextOffset() ;

         if ( DMS_INVALID_OFFSET != prevRecordOffset )
         {
            dmsRecordID prevRID = rid ;
            prevRID._offset = prevRecordOffset ;
            dmsRecordRW prevRW = record2RW( prevRID, context->mbID() ) ;
            dmsRecord *prevRecord = prevRW.writePtr() ;
            prevRecord->setNextOffset( nextRecordOffset ) ;
         }
         if ( DMS_INVALID_OFFSET != nextRecordOffset )
         {
            dmsRecordID nextRID = rid ;
            nextRID._offset = nextRecordOffset ;
            dmsRecordRW nextRW = record2RW( nextRID, context->mbID() ) ;
            dmsRecord *nextRecord = nextRW.writePtr() ;
            nextRecord->setPrevOffset( prevRecordOffset ) ;
         }
         if ( pExtent->_firstRecordOffset == rid._offset )
         {
            pExtent->_firstRecordOffset = nextRecordOffset ;
         }
         if ( pExtent->_lastRecordOffset == rid._offset )
         {
            pExtent->_lastRecordOffset = prevRecordOffset ;
         }

         if ( decCount )
         {
            --(pExtent->_recCount) ;
            _decreaseMBStat( context->mbStat()->_clUniqueID, context->mbStat(), cb ) ;
         }
      }
      //increase data write counter
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

      rc = _saveDeletedRecord( context->mb(), rid, 0 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to save deleted record, rc = %d", rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATA__EXTENTREMOVERECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__FINALRECORDSIZE, "_dmsStorageData::_finalRecordSize" )
   void _dmsStorageData::_finalRecordSize( UINT32 &size,
                                           const dmsRecordData &recordData,
                                           BOOLEAN markInsert )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__FINALRECORDSIZE ) ;

      if ( !markInsert )
      {
         _overflowSize( size ) ;
      }

      size += DMS_RECORD_METADATA_SZ ;
      // record is ALWAYS 4 bytes aligned
      size = OSS_MIN( DMS_RECORD_MAX_SZ, ossAlignX ( size, 4 ) ) ;
      PD_TRACE2 ( SDB__DMSSTORAGEDATA__FINALRECORDSIZE,
                  PD_PACK_STRING ( "size after align" ),
                  PD_PACK_UINT ( size ) ) ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATA__FINALRECORDSIZE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA__ONINSERTFAIL, "_dmsStorageData::_onInsertFail" )
   INT32 _dmsStorageData::_onInsertFail( dmsMBContext *context,
                                         BOOLEAN hasInsert,
                                         dmsRecordID rid,
                                         SDB_DPSCB *dpscb,
                                         ossValuePtr dataPtr,
                                         _pmdEDUCB *cb,
                                         const dmsTransRecordInfo *pInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA__ONINSERTFAIL ) ;
      if ( hasInsert )
      {
         // we won't touch old version if it's insert failure.
         // No callback needed
         // we need transaction record info to decide how to remove the
         // record ( e.g. mark deleted or remove from extent )
         rc = deleteRecord( context, rid, dataPtr, cb, dpscb, NULL, pInfo,
                            TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to rollback, rc: %d", rc ) ;
      }
      else if ( rid.isValid() )
      {
         _saveDeletedRecord( context->mb(), rid, 0 ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA__ONINSERTFAIL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_EXTRACTDATA, "_dmsStorageData::extractData" )
   INT32 _dmsStorageData::extractData( const dmsMBContext *mbContext,
                                       const dmsRecordRW &recordRW,
                                       _pmdEDUCB *cb,
                                       dmsRecordData &recordData,
                                       BOOLEAN needIncDataRead )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATA_EXTRACTDATA ) ;
      monAppCB * pMonAppCB    = cb ? cb->getMonAppCB() : NULL ;
      const dmsRecord *pRecord= recordRW.readPtr( 0 ) ;

      recordData.reset() ;

      if ( !mbContext->isMBLock() )
      {
         PD_LOG( PDERROR, "MB Context must be locked" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// if ovf, need to get the ovt's data
      if ( pRecord->isOvf() )
      {
         dmsRecordID ovfRID = pRecord->getOvfRID() ;
         dmsRecordRW ovfRW = record2RW( ovfRID, mbContext->mbID() ) ;
         // Inherit no-throw property
         ovfRW.setNothrow( recordRW.isNothrow() ) ;
         pRecord = ovfRW.readPtr( 0 ) ;
         if ( NULL == pRecord )
         {
            rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
            PD_LOG( PDERROR, "Failed to get record from address[%d.%d]",
                    ovfRID._extent, ovfRID._offset ) ;
            goto error ;
         }
         SDB_ASSERT( pRecord->isOvt(), "Record must be ovt" ) ;
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      }

      recordData.setData( pRecord->getData(), pRecord->getDataLength(),
                          UTIL_COMPRESSOR_INVALID, TRUE ) ;

      if ( pRecord->isCompressed() )
      {
         const CHAR *pUncompressData = NULL ;
         INT32 unCompressDataLen = 0 ;
         rc = dmsUncompress( cb, &_compressorEntry[ mbContext->mbID() ],
                             pRecord->getCompressType(), pRecord->getData(),
                             pRecord->getDataLength(),
                             &pUncompressData, &unCompressDataLen ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to uncompress data, rc: %d", rc ) ;
            goto error ;
         }
         /// check the length
         if ( unCompressDataLen != *(INT32*)pUncompressData )
         {
            PD_LOG( PDERROR, "Uncompress data length[%d] does not match "
                    "real length[%d]", unCompressDataLen,
                    *(INT32*)pUncompressData ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
         recordData.setData( pUncompressData, unCompressDataLen,
                             UTIL_COMPRESSOR_INVALID, FALSE ) ;
      }
      if( needIncDataRead )
      {
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      }

      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_READ, 1 ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATA_EXTRACTDATA, rc ) ;
      return rc ;
   error:
      goto done ;

   }

   INT32 _dmsStorageData::_operationPermChk( DMS_ACCESS_TYPE accessType )
   {
      INT32 rc = SDB_OK ;
      if ( DMS_ACCESS_TYPE_POP == accessType )
      {
         PD_LOG( PDERROR, "Operation type[ %d ] is incompatible with "
                 "collection", accessType ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _dmsStorageData::_getEyeCatcher() const
   {
      return DMS_DATASU_EYECATCHER ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATA_POSTEXTLOAD, "_dmsStorageData::postLoadExt" )
   void _dmsStorageData::postLoadExt( dmsMBContext *context,
                                      dmsExtent *extAddr,
                                      SINT32 extentID )
   {
      _mapExtent2DelList( context, extAddr, extentID ) ;
   }

   INT32 _dmsStorageData::dumpExtOptions( dmsMBContext *context,
                                          BSONObj &extOptions )
   {
      return SDB_OK ;
   }

   INT32 _dmsStorageData::setExtOptions ( dmsMBContext * context,
                                          const BSONObj & extOptions )
   {
      return SDB_OK ;
   }

}
