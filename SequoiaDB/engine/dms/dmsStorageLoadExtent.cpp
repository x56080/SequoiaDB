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
#include "dmsTransLockCallback.hpp"

using namespace bson ;

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
      else if ( requestSize > (INT32)DMS_MAX_EXTENT_SZ(_su->data()) )
      {
         requestSize = DMS_MAX_EXTENT_SZ(_su->data()) ;
      }
      else
      {
         requestSize = ossRoundUpToMultipleX ( requestSize, _pageSize ) ;
      }

      if ( (UINT32)requestSize > _buffSize && _pCurrentExtent )
      {
         SDB_OSS_FREE( _pCurrentExtent ) ;
         _pCurrentExtent = NULL ;
         _buffSize = 0 ;
      }

      if ( (UINT32)requestSize > _buffSize )
      {
         _pCurrentExtent = ( CHAR* )SDB_OSS_MALLOC( requestSize << 1 ) ;
         if ( !_pCurrentExtent )
         {
            PD_LOG( PDERROR, "Alloc memroy[%d] failed",
                    requestSize << 1 ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _buffSize = ( requestSize << 1 ) ;
      }

      _currentExtentSize = requestSize ;
      _initExtentHeader ( (dmsExtent*)_pCurrentExtent,
                          _currentExtentSize/_pageSize ) ;

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

      SDB_ASSERT( mbContext, "mb context can't be NULL" ) ;

<<<<<<< HEAD
      rc = _su->data()->prepareCollectionLoads( mbContext, record, isLast, isAsynchr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare collection loads, rc: %d", rc ) ;
=======
      compressorEntry = _su->data()->getCompressorEntry( mbContext->mbID() ) ;
      /* For concurrency protection with drop CL and set compresor. */
      dmsCompressorGuard compGuard( compressorEntry, SHARED ) ;

      try
      {
         recordData.setData( record.objdata(), record.objsize(),
                             UTIL_COMPRESSOR_INVALID, TRUE ) ;
         /* (0) */
         // verify whether the record got "_id" inside
         BSONElement ele = record.getField ( DMS_ID_KEY_NAME ) ;
         const CHAR *pCheckErr = "" ;
         if ( !dmsIsRecordIDValid( ele, TRUE, &pCheckErr ) )
         {
            PD_LOG_MSG( PDERROR, "_id is error: %s", pCheckErr ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

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
            recordData.setData( pNewRecordData,
                                oidEle.size() + record.objsize(),
                                UTIL_COMPRESSOR_INVALID, TRUE ) ;
            record = BSONObj( pNewRecordData ) ;
         }
         dmsrecordSize = recordData.len() ;

         // check
         if ( recordData.len() + DMS_RECORD_METADATA_SZ >
              DMS_RECORD_USER_MAX_SZ )
         {
            rc = SDB_DMS_RECORD_TOO_BIG ;
            goto error ;
         }

         if ( compressorEntry->ready() )
         {
            const CHAR *compressedData    = NULL ;
            INT32 compressedDataSize      = 0 ;
            UINT8 compressRatio           = 0 ;
            rc = dmsCompress( cb, compressorEntry,
                              recordData.data(), recordData.len(),
                              &compressedData, &compressedDataSize,
                              compressRatio ) ;
            // Compression is valid and ratio is less the threshold
            if ( SDB_OK == rc &&
                 compressedDataSize + sizeof(UINT32) < recordData.orgLen() &&
                 compressRatio < UTIL_COMPRESSOR_DFT_MIN_RATIO )
            {
               // 4 bytes len + compressed record
               dmsrecordSize = compressedDataSize + sizeof(UINT32) ;
               // set the compression data
               recordData.setData( compressedData, compressedDataSize,
                                   compressorEntry->getCompressorType(),
                                   FALSE ) ;
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

         // add record metadata and oid
         dmsrecordSize *= DMS_RECORD_OVERFLOW_RATIO ;
         dmsrecordSize += DMS_RECORD_METADATA_SZ ;
         // record is ALWAYS 4 bytes aligned
         dmsrecordSize = OSS_MIN( DMS_RECORD_MAX_SZ,
                                  ossAlignX ( dmsrecordSize, 4 ) ) ;

         INT32 expandSize = dmsrecordSize << DMS_RECORDS_PER_EXTENT_SQUARE ;
         if ( expandSize > DMS_BEST_UP_EXTENT_SZ )
         {
            expandSize = expandSize < DMS_BEST_UP_EXTENT_SZ ?
                         DMS_BEST_UP_EXTENT_SZ : expandSize ;
         }

         if ( !_pCurrentExtent )
         {
            rc = _allocateExtent ( expandSize ) ;
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

            rc = _allocateExtent ( expandSize ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to allocate new extent in reorg file, "
                        "rc = %d", rc ) ;
               goto error ;
            }
         }

         recordOffset = _currentExtentSize - _currentExtent->_freeSpace ;
         pRecord = ( dmsRecord* )( (const CHAR*)_currentExtent + recordOffset ) ;

         if ( _currentExtent->_freeSpace - (INT32)dmsrecordSize <
              (INT32)DMS_MIN_RECORD_SZ &&
              _currentExtent->_freeSpace <= (INT32)DMS_RECORD_MAX_SZ )
         {
            dmsrecordSize = _currentExtent->_freeSpace ;
         }

         // set record header
         pRecord->setNormal() ;
         pRecord->setMyOffset( recordOffset ) ;
         pRecord->setSize( dmsrecordSize ) ;
         pRecord->setData( recordData ) ;
         pRecord->setNextOffset( DMS_INVALID_OFFSET ) ;
         pRecord->setPrevOffset( DMS_INVALID_OFFSET ) ;

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
            pPreRecord = (dmsRecord*)( (const CHAR*)_currentExtent + offset ) ;
            pPreRecord->setNextOffset( recordOffset ) ;
            pRecord->setPrevOffset( offset ) ;
         }
         _currentExtent->_lastRecordOffset = recordOffset ;

         // then check extent header for first record
         offset = _currentExtent->_firstRecordOffset ;
         if ( DMS_INVALID_OFFSET == offset )
         {
            _currentExtent->_firstRecordOffset = recordOffset ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

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

      rc = _su->data()->buildCollectionLoads( mbContext, isAsynchr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build collection loads, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__LDDATA, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, "dmsStorageLoadOp::loadRollbackPhase" )
   INT32 dmsStorageLoadOp::loadRollbackPhase( dmsMBContext *mbContext, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT ) ;

      SDB_ASSERT ( _su, "_su can't be NULL" ) ;
      SDB_ASSERT ( mbContext, "mb context can't be NULL" ) ;

      PD_LOG ( PDEVENT, "Start loadRollbackPhase" ) ;

      rc = _su->data()->truncateCollectionLoads( mbContext, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection loads, rc: %d", rc ) ;

   done:
      PD_LOG ( PDEVENT, "End loadRollbackPhase" ) ;
      PD_TRACE_EXITRC ( SDB__DMSSTORAGELOADEXT__ROLLEXTENT, rc );
      return rc ;
   error:
      goto done ;
   }

}

