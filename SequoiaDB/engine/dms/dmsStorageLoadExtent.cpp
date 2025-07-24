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

   Source File Name = dmsStorageLoadExtent.cpp

   Descriptive Name =

   When/how to use: load extent to database

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2013  JW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "dmsSMEMgr.hpp"
#include "dmsStorageLoadExtent.hpp"
#include "dmsCompress.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "migLoad.hpp"

namespace engine
{
   void dmsStorageLoadOp::_initExtentHeader ( dmsExtent *extAddr,
                                              UINT16 numPages )
   {
      SDB_ASSERT ( _pageSize * numPages == _currentExtentSize,
                   "extent size doesn't match" ) ;
      extAddr->_eyeCatcher[0]          = DMS_EXTENT_EYECATCHER0 ;
      extAddr->_eyeCatcher[1]          = DMS_EXTENT_EYECATCHER1 ;
      extAddr->_blockSize              = numPages ;
      extAddr->_mbID                   = 0 ;
      extAddr->_flag                   = DMS_EXTENT_FLAG_INUSE ;
      extAddr->_version                = DMS_EXTENT_CURRENT_V ;
      extAddr->_logicID                = DMS_INVALID_EXTENT ;
      extAddr->_prevExtent             = DMS_INVALID_EXTENT ;
      extAddr->_nextExtent             = DMS_INVALID_EXTENT ;
      extAddr->_recCount               = 0 ;
      extAddr->_firstRecordOffset      = DMS_INVALID_EXTENT ;
      extAddr->_lastRecordOffset       = DMS_INVALID_EXTENT ;
      extAddr->_freeSpace              = _pageSize * numPages -
                                         sizeof(dmsExtent) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT, "dmsStorageLoadOp::_allocateExtent" )
   INT32 dmsStorageLoadOp::_allocateExtent ( INT32 requestSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT );
      if ( requestSize < DMS_MIN_EXTENT_SZ(_pageSize) )
      {
         requestSize = DMS_MIN_EXTENT_SZ(_pageSize) ;
      }
      else if ( requestSize > DMS_MAX_EXTENT_SZ )
      {
         requestSize = DMS_MAX_EXTENT_SZ ;
      }
      else
      {
         requestSize = ossRoundUpToMultipleX ( requestSize, _pageSize ) ;
      }

      if ( !_pCurrentExtent )
      {
         _pCurrentExtent = (CHAR*)SDB_OSS_MALLOC ( requestSize ) ;
         if ( !_pCurrentExtent )
         {
            PD_LOG ( PDERROR, "Unable to allocate %d bytes memory",
                     requestSize ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _currentExtentSize = requestSize ;
         _initExtentHeader ( (dmsExtent*)_pCurrentExtent,
                             _currentExtentSize/_pageSize ) ;
      }
      else
      {
         if ( requestSize > _currentExtentSize )
         {
            CHAR *pOldPtr = _pCurrentExtent ;
            _pCurrentExtent = (CHAR*)SDB_OSS_REALLOC ( _pCurrentExtent,
                                                       requestSize ) ;
            if ( !_pCurrentExtent )
            {
               PD_LOG ( PDERROR, "Unable to realloc %d bytes memory",
                        requestSize ) ;
               _pCurrentExtent = pOldPtr ;
               rc = SDB_OOM ;
               goto error ;
            }
            _currentExtentSize = requestSize ;
         }
         _initExtentHeader ( (dmsExtent*)_pCurrentExtent,
                             _currentExtentSize/_pageSize ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__ALLOCEXTENT, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK, "dmsStorageLoadOp::pushToTempDataBlock" )
   INT32 dmsStorageLoadOp::pushToTempDataBlock ( dmsMBContext *mbContext,
                                                 pmdEDUCB *cb,
                                                 BSONObj &record,
                                                 BOOLEAN isLast,
                                                 BOOLEAN isAsynchr )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK );
      UINT32      dmsrecordSize   = 0 ;
      ossValuePtr recordPtr       = 0 ;
      ossValuePtr prevPtr         = 0 ;
      dmsOffset   offset          = DMS_INVALID_OFFSET ;
      dmsOffset   recordOffset    = DMS_INVALID_OFFSET ;
      BSONElement ele ;
      _IDToInsert oid ;
      idToInsertEle oidEle((CHAR*)(&oid)) ;
      CHAR *pNewRecordData = NULL ;
      const CHAR *recordData = NULL ;
      UINT32 recordDataSize = 0 ;
      BOOLEAN compressed = FALSE ;
      dmsCompressorEntry *compressorEntry = NULL ;

      SDB_ASSERT( mbContext, "mb context can't be NULL" ) ;

      compressorEntry = _su->data()->getCompressorEntry( mbContext->mbID() ) ;
      /* For concurrency protection with drop CL and set compresor. */
      dmsCompressorGuard compGuard( compressorEntry, SHARED ) ;
      /* (0) */
      // verify whether the record got "_id" inside
      ele = record.getField ( DMS_ID_KEY_NAME ) ;
      const CHAR *pCheckErr = "" ;
      if ( !dmsIsRecordIDValid( ele, TRUE, &pCheckErr ) )
      {
         PD_LOG( PDERROR, "Record[%s] _id is error: %s",
                 record.toString().c_str(), pCheckErr ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      recordData = record.objdata() ;
      recordDataSize = record.objsize() ;
      dmsrecordSize = recordDataSize ;
      // if the record is not for temp, and
      // "_id" doesn't exist, let's create the object
      if ( ele.eoo() )
      {
         oid._oid.init() ;
         rc = cb->allocBuff( oidEle.size() + record.objsize(),
                             &pNewRecordData ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc memory[size:%u] failed, rc: %d",
                    oidEle.size() + record.objsize(), rc ) ;
            goto error ;
         }
         /// copy to new data
         *(UINT32*)pNewRecordData = oidEle.size() + record.objsize() ;
         ossMemcpy( pNewRecordData + sizeof(UINT32), oidEle.rawdata(),
                    oidEle.size() ) ;
         ossMemcpy( pNewRecordData + sizeof(UINT32) + oidEle.size(),
                    record.objdata() + sizeof(UINT32),
                    record.objsize() - sizeof(UINT32) ) ;

         record = BSONObj( pNewRecordData ) ;
         recordData = pNewRecordData ;
         recordDataSize = oidEle.size() + record.objsize() ;
         dmsrecordSize = recordDataSize ;
      }

      if ( ( dmsrecordSize + DMS_RECORD_METADATA_SZ ) > DMS_RECORD_USER_MAX_SZ )
      {
         rc = SDB_DMS_RECORD_TOO_BIG ;
         goto error ;
      }

      if ( compressorEntry->ready() )
      {
         const CHAR *compressedData = NULL ;
         INT32 compressedDataSize = 0 ;

         rc = dmsCompress( cb, compressorEntry, recordData, recordDataSize,
                           &compressedData, &compressedDataSize ) ;
         if ( SDB_OK == rc &&
              compressedDataSize + sizeof(UINT32) < recordDataSize )
         {
            recordData = compressedData ;
            recordDataSize = compressedDataSize ;
            // 4 bytes len + compressed record
            dmsrecordSize = compressedDataSize + sizeof(UINT32) ;
            compressed = TRUE ;
         }
         else if ( rc )
         {
            // In any case of error, leave it, and use the original data.
            if ( SDB_UTIL_COMPRESS_ABORT == rc )
            {
               PD_LOG( PDINFO, "Record compression aborted. "
                       "Insert the original data. rc: %d", rc ) ;
            }
            else
            {
               PD_LOG( PDWARNING, "Record compression failed. "
                       "Insert the original data. rc: %d", rc ) ;
            }
            rc = SDB_OK ;
         }
      }

      /*
       * Release the guard to avoid deadlock with truncate/drop collection.
       */
      compGuard.release() ;
      dmsrecordSize *= DMS_RECORD_OVERFLOW_RATIO ;
      dmsrecordSize += DMS_RECORD_METADATA_SZ ;
      dmsrecordSize = OSS_MIN(DMS_RECORD_MAX_SZ, ossAlignX(dmsrecordSize,4)) ;
      if ( !_pCurrentExtent )
      {
         rc = _allocateExtent ( dmsrecordSize <<
                                DMS_RECORDS_PER_EXTENT_SQUARE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to allocate new extent in reorg file, "
                     "rc = %d", rc ) ;
            goto error ;
         }
         _currentExtent = (dmsExtent*)_pCurrentExtent ;
      }

      if ( dmsrecordSize > (UINT32)_currentExtent->_freeSpace || isLast )
      {
         // lock
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to lock collection, rc=%d", rc ) ;
            goto error ;
         }
         if ( !isAsynchr )
         {
            _currentExtent->_firstRecordOffset = DMS_INVALID_OFFSET ;
            _currentExtent->_lastRecordOffset = DMS_INVALID_OFFSET ;
         }

         rc = _su->loadExtentA( mbContext, _pCurrentExtent,
                                _currentExtentSize / _pageSize,
                                TRUE ) ;
         // unlock
         mbContext->mbUnlock() ;

         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to load extent, rc = %d", rc ) ;
            goto error ;
         }

         if ( isLast )
         {
            goto done ;
         }

         rc = _allocateExtent ( dmsrecordSize <<
                                DMS_RECORDS_PER_EXTENT_SQUARE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to allocate new extent in reorg file, "
                     "rc = %d", rc ) ;
            goto error ;
         }
      }
      recordOffset = _currentExtentSize - _currentExtent->_freeSpace ;
      recordPtr = ((ossValuePtr)_currentExtent) + recordOffset ;
      if ( _currentExtent->_freeSpace - (INT32)dmsrecordSize <
           (INT32)DMS_MIN_RECORD_SZ &&
           _currentExtent->_freeSpace <= (INT32)DMS_RECORD_MAX_SZ )
      {
         dmsrecordSize = _currentExtent->_freeSpace ;
      }

      // set record header
      DMS_RECORD_SETFLAG ( recordPtr, DMS_RECORD_FLAG_NORMAL ) ;
      DMS_RECORD_SETMYOFFSET ( recordPtr, recordOffset ) ;
      DMS_RECORD_SETSIZE ( recordPtr, dmsrecordSize ) ;
      if ( compressed )
      {
         DMS_RECORD_SETATTR ( recordPtr, DMS_RECORD_FLAG_COMPRESSED ) ;
      }
      else
      {
         DMS_RECORD_UNSETATTR ( recordPtr, DMS_RECORD_FLAG_COMPRESSED ) ;
      }

      DMS_RECORD_SETDATA ( recordPtr, recordData, recordDataSize ) ;
      DMS_RECORD_SETNEXTOFFSET ( recordPtr, DMS_INVALID_OFFSET ) ;
      DMS_RECORD_SETPREVOFFSET ( recordPtr, DMS_INVALID_OFFSET ) ;
      // set extent header
      if ( isAsynchr )
      {
         _currentExtent->_recCount++ ;
      }
      _currentExtent->_freeSpace -= dmsrecordSize ;
      // set previous record next pointer
      offset = _currentExtent->_lastRecordOffset ;
      if ( DMS_INVALID_OFFSET != offset )
      {
         prevPtr = ((ossValuePtr)_currentExtent) + offset ;
         DMS_RECORD_SETNEXTOFFSET ( prevPtr, recordOffset ) ;
         DMS_RECORD_SETPREVOFFSET ( recordPtr, offset ) ;
      }

      _currentExtent->_lastRecordOffset = recordOffset ;

      // then check extent header for first record
      offset = _currentExtent->_firstRecordOffset ;
      if ( DMS_INVALID_OFFSET == offset )
      {
         _currentExtent->_firstRecordOffset = recordOffset ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__IMPRTBLOCK, rc );
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__LDDATA, "dmsStorageLoadOp::loadBuildPhase" )
   INT32 dmsStorageLoadOp::loadBuildPhase ( dmsMBContext *mbContext,
                                            pmdEDUCB *cb,
                                            BOOLEAN isAsynchr,
                                            migMaster *pMaster,
                                            UINT32 *success,
                                            UINT32 *failure )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__LDDATA ) ;

      dmsExtent     *extAddr        = NULL ;
      //dmsExtent     *prevExt        = NULL ;
      dmsRecordID    recordID ;
      ossValuePtr    recordPtr      = 0 ;
      ossValuePtr    recordDataPtr  = 0 ;
      ossValuePtr    extentPtr      = 0 ;
      dmsOffset      recordOffset   = DMS_INVALID_OFFSET ;
      dmsExtentID    tempExtentID   = 0 ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      dmsCompressorEntry *compressorEntry = NULL ;

      SDB_ASSERT ( _su, "_su can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "dms mb context can't be NULL" ) ;
      SDB_ASSERT ( cb, "cb is NULL" ) ;

      rc = mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to lock collection, rc: %d", rc ) ;

      if ( DMS_INVALID_EXTENT == mbContext->mb()->_loadFirstExtentID &&
           DMS_INVALID_EXTENT == mbContext->mb()->_loadLastExtentID )
      {
         PD_LOG ( PDEVENT, "has not load extent" ) ;
         goto done ;
      }

      if ( ( DMS_INVALID_EXTENT == mbContext->mb()->_lastExtentID &&
             DMS_INVALID_EXTENT != mbContext->mb()->_firstExtentID ) ||
           ( DMS_INVALID_EXTENT != mbContext->mb()->_lastExtentID &&
             DMS_INVALID_EXTENT == mbContext->mb()->_firstExtentID ) )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Invalid mb context[%s], first extent: %d, last "
                  "extent: %d", mbContext->toString().c_str(),
                  mbContext->mb()->_firstExtentID,
                  mbContext->mb()->_lastExtentID ) ;
         goto error ;
      }

      clearFlagLoadLoad ( mbContext->mb() ) ;
      setFlagLoadBuild ( mbContext->mb() ) ;

      compressorEntry = _su->data()->getCompressorEntry( mbContext->mbID() ) ;
      while ( !cb->isForced() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "dms mb context lock failed, rc", rc ) ;
            goto error ;
         }

         tempExtentID = mbContext->mb()->_loadFirstExtentID ;
         if ( DMS_INVALID_EXTENT == tempExtentID )
         {
            mbContext->mb()->_loadFirstExtentID = DMS_INVALID_EXTENT ;
            mbContext->mb()->_loadLastExtentID = DMS_INVALID_EXTENT ;
            goto done ;
         }

         extentPtr = _su->data()->extentAddr( tempExtentID ) ;
         extAddr = (dmsExtent *)extentPtr ;
         PD_CHECK ( extAddr, SDB_SYS, error, PDERROR, "Invalid extent: %d",
                    tempExtentID ) ;
         SDB_ASSERT( extAddr->validate( mbContext->mbID() ),
                     "Invalid extent" ) ;

         rc = _su->data()->addExtent2Meta( tempExtentID, extAddr, mbContext ) ;
         PD_RC_CHECK( rc, PDERROR, "Add extent to meta failed, rc: %d", rc ) ;

         mbContext->mb()->_loadFirstExtentID = extAddr->_nextExtent ;

         extAddr->_firstRecordOffset = DMS_INVALID_OFFSET ;
         extAddr->_lastRecordOffset  = DMS_INVALID_OFFSET ;
         _su->addExtentRecordCount( mbContext->mb(), extAddr->_recCount ) ;
         extAddr->_recCount          = 0 ;
         // add freespace to del list
         _su->mapExtent2DelList( mbContext->mb(), extAddr, tempExtentID ) ;

         recordOffset = DMS_EXTENT_METADATA_SZ ;
         recordID._extent = tempExtentID ;

         while ( DMS_INVALID_OFFSET != recordOffset )
         {
            recordPtr = extentPtr + recordOffset ;
            recordID._offset = recordOffset ;
            DMS_RECORD_EXTRACTDATA( recordPtr, recordDataPtr,
                                    compressorEntry ) ;
            recordOffset = DMS_RECORD_GETNEXTOFFSET(recordPtr) ;
            ++( extAddr->_recCount ) ;

            try
            {
               // get the BSON object
               BSONObj obj ( (const CHAR*)recordDataPtr ) ;
               // when we get here, that means we have a new record
               // to add to index
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

               // attempt to insert into the index
               rc = _su->index()->indexesInsert( mbContext, tempExtentID, obj,
                                                 recordID, cb ) ;
               // if any error happen
               if ( rc )
               {
                  if ( SDB_IXM_DUP_KEY != rc )
                  {
                     PD_LOG ( PDERROR, "Failed to insert into index, rc=%d",
                              rc ) ;
                     goto rollback ;
                  }
                  if ( success )
                  {
                     --(*success) ;
                  }
                  if ( failure )
                  {
                     ++(*failure) ;
                  }
                  if ( pMaster )
                  {
                     rc = pMaster->sendMsgToClient( "Error: index insert, error"
                                                    "code %d, %s", rc,
                                                    obj.toString().c_str() ) ;
                     if ( rc )
                     {
                        PD_LOG ( PDERROR, "Failed to send msg, rc=%d", rc ) ;
                     }
                  }
                  rc = _su->data()->deleteRecord ( mbContext, recordID,
                                                   recordDataPtr,
                                                   cb, NULL ) ;
                  if ( rc )
                  {
                     PD_LOG ( PDERROR, "Failed to rollback, rc = %d", rc ) ;
                     goto rollback ;
                  }
               }
            }
            catch ( std::exception &e )
            {
               PD_LOG ( PDERROR, "Failed to create BSON object: %s",
                        e.what() ) ;
               rc = SDB_SYS ;
               goto rollback ;
            }

            // extent point to cur record
            if ( DMS_INVALID_OFFSET == extAddr->_firstRecordOffset )
            {
               extAddr->_firstRecordOffset = recordID._offset ;
            }
            extAddr->_lastRecordOffset = recordID._offset ;
         } //while ( DMS_INVALID_OFFSET != recordOffset )

         // unlock
         mbContext->mbUnlock() ;
      } // while

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__LDDATA, rc );
      return rc ;
   error:
      goto done ;
   rollback:
      // save the extent other record to del list
      recordOffset = recordID._offset ;
      while ( DMS_INVALID_OFFSET != recordOffset )
      {
         recordPtr = extentPtr + recordOffset ;
         recordID._offset = recordOffset ;
         recordOffset = DMS_RECORD_GETNEXTOFFSET(recordPtr) ;

         _su->extentRemoveRecord( mbContext->mb(), recordID, 0, cb ) ;

         if ( DMS_INVALID_OFFSET != recordOffset )
         {
            ++( extAddr->_recCount ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, "dmsStorageLoadOp::loadRollbackPhase" )
   INT32 dmsStorageLoadOp::loadRollbackPhase( dmsMBContext *mbContext )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT ) ;

      SDB_ASSERT ( _su, "_su can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "mb context can't be NULL" ) ;

      PD_LOG ( PDEVENT, "Start loadRollbackPhase" ) ;

      rc = _su->data()->truncateCollectionLoads( NULL, mbContext ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection loads, rc: %d",
                   rc ) ;

   done:
      PD_LOG ( PDEVENT, "End loadRollbackPhase" ) ;
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, rc );
      return rc ;
   error:
      goto done ;
   }

}

