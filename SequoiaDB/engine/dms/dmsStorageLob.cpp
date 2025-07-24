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

   Source File Name = dmsStorageLob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          17/07/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsStorageLob.hpp"
#include "dpsOp2Record.hpp"
#include "pmd.hpp"
#include "dpsTransCB.hpp"
#include "dmsTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{
   #define DMS_LOB_META( extent )\
      ( ( _dmsLobDataMapBlk * )extent )

   #define DMS_LOBM_EYECATCHER               "SDBLOBM"
   #define DMS_LOBM_EYECATCHER_LEN           8

   #define DMS_LOB_EXTEND_THRESHOLD_SIZE     ( 65536 )   // 64K

   #define DMS_LOB_PAGE_IN_USED( page )\
           ( DMS_SME_ALLOCATED == getSME()->getBitMask( page ) )

   #define DMS_LOB_GET_HASH_FROM_BLK( blk, hash )\
           do\
           {\
              const BYTE *d1 = (blk)->_oid ;\
              const BYTE *d2 = ( const BYTE * )( &( (blk)->_sequence ) ) ;\
              (hash) = ossHash( d1, sizeof( (blk)->_oid ),\
                              d2, sizeof( (blk)->_sequence ) ) ;\
           } while( FALSE )

   /*
      _dmsStorageLob implement
   */
   _dmsStorageLob::_dmsStorageLob( const CHAR *lobmFileName,
                                   const CHAR *lobdFileName,
                                   dmsStorageInfo *info,
                                   dmsStorageData *pDataSu,
                                   utilCacheUnit *pCacheUnit )
   :_dmsStorageBase( lobmFileName, info ),
    _dmsBME( NULL ),
    _segmentSize( 0 ),
    _dmsData( pDataSu ),
    _data( lobdFileName ),
    _pCacheUnit( pCacheUnit )
   {
      ossMemset( _path, 0, sizeof( _path ) ) ;
      _needDelayOpen = FALSE ;

      _dmsData->_attachLob( this ) ;
   }

   _dmsStorageLob::~_dmsStorageLob()
   {
      _dmsData->_detachLob() ;
      _dmsData = NULL ;
      _dmsBME = NULL ;

      for ( UINT32 i = 0 ; i < _vecBucketLacth.size() ; ++i )
      {
         SDB_OSS_DEL _vecBucketLacth[ i ] ;
      }
      _vecBucketLacth.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_OPEN, "_dmsStorageLob::open" )
   INT32 _dmsStorageLob::open( const CHAR *path,
                               BOOLEAN createNew,
                               BOOLEAN rmWhenExist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_OPEN ) ;

      // copy path
      ossStrncpy( _path, path, OSS_MAX_PATHSIZE ) ;

      // if not create lobs
      if ( 0 == _dmsData->getHeader()->_createLobs )
      {
         _needDelayOpen = TRUE ;
      }
      else
      {
         rc = _openLob( path, createNew, rmWhenExist ) ;
         /// when open exist lob files, need to analysis the lob count
         if ( !createNew && SDB_OK == rc &&
              getHeader()->_version <= DMS_LOB_VERSION_1 )
         {
            rc = _calcCount() ;
         }
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_OPEN, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__DELAYOPEN, "_dmsStorageLob::_delayOpen" )
   INT32 _dmsStorageLob::_delayOpen()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__DELAYOPEN ) ;

      _delayOpenLatch.get() ;

      if ( !_needDelayOpen )
      {
         goto done ;
      }

      if ( isOpened() )
      {
         goto done ;
      }

      rc = _openLob( _path, TRUE, TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Delay open[%s] failed, rc: %d",
                 getSuName(), rc ) ;
         goto error ;
      }

      _needDelayOpen = FALSE ;

      // set data header
      _dmsData->updateCreateLobs( 1 ) ;

   done:
      _delayOpenLatch.release() ;
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__DELAYOPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__OPENLOB, "_dmsStorageLob::_openLob" )
   INT32 _dmsStorageLob::_openLob( const CHAR *path,
                                   BOOLEAN createNew,
                                   BOOLEAN rmWhenExist )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__OPENLOB ) ;
      ossSpinSLatch *pLatch = NULL ;
      /// create bucket latch
      for ( UINT32 i = 0 ; i < DMS_BUCKETS_LATCH_SIZE ; ++i )
      {
         pLatch = SDB_OSS_NEW ossSpinSLatch ;
         if ( !pLatch )
         {
            PD_LOG( PDERROR, "Create bucket latch[%d] failed", i ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _vecBucketLacth.push_back( pLatch ) ;
      }

      /// Init cache unit
      rc = _pCacheUnit->init( getLobData(), _pStorageInfo->_lobdPageSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init cache unit failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = openStorage( path, createNew, rmWhenExist ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lobm file:%s, rc:%d",
                 _suFileName, rc ) ;
         if ( createNew && SDB_FE != rc )
         {
            goto rmlobm ;
         }
         goto error ;
      }

      rc = _data.open( path, createNew, rmWhenExist,
                       *_pStorageInfo, pmdGetThreadEDUCB() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lobd file:%s, rc:%d",
                 _data.getFileName(), rc ) ;
         if ( createNew )
         {
            if ( SDB_FE != rc )
            {
               goto rmboth ;
            }
            goto rmlobm ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__OPENLOB, rc ) ;
      return rc ;
   error:
      _data.close() ;
      close() ;
      goto done ;
   rmlobm:
      removeStorage() ;
      goto error ;
   rmboth:
      removeStorageFiles() ;
      goto error ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_REMOVESTORAGEFILES, "_dmsStorageLob::removeStorageFiles" )
   void _dmsStorageLob::removeStorageFiles()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_REMOVESTORAGEFILES ) ;
      rc = removeStorage() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove file:%d", rc ) ;
      }

      rc = _data.remove() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to remove file:%d", rc ) ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGELOB_REMOVESTORAGEFILES ) ;
      return ;
   }

    // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_GETLOBMETA, "_dmsStorageLob::getLobMeta" )
   INT32 _dmsStorageLob::getLobMeta( const bson::OID &oid,
                                     dmsMBContext *mbContext,
                                     pmdEDUCB *cb,
                                     _dmsLobMeta &meta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_GETLOBMETA ) ;
      UINT32 readSz = 0 ;
      dmsLobRecord piece ;
      piece.set( &oid, DMS_LOB_META_SEQUENCE, 0,
                 sizeof( meta ), NULL ) ;
      rc = read( piece, mbContext, cb, 
                 ( CHAR * )( &meta ), readSz ) ;
      if ( SDB_OK == rc )
      {
         if ( sizeof( meta ) != readSz )
         {
            PD_LOG( PDERROR, "read length is %d, big error!", readSz ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         goto done ;
      }
      else if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
      {
         rc = SDB_FNE ;
         goto error ;
      }
      else
      {
         PD_LOG( PDERROR, "failed to read meta of lob, rc:%d",
                 rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_GETLOBMETA, rc ) ;
      return rc ;
   error:
      meta.clear() ;
      goto done ;
   }

    // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_WRITELOBMETA, "_dmsStorageLob::writeLobMeta" )
   INT32 _dmsStorageLob::writeLobMeta( const bson::OID &oid,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB *cb,
                                       const _dmsLobMeta &meta,
                                       BOOLEAN isNew,
                                       SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_WRITELOBMETA ) ;
      dmsLobRecord piece ;
      piece.set( &oid, DMS_LOB_META_SEQUENCE, 0,
                 sizeof( meta ), ( const CHAR * )( &meta ) ) ;
      if ( isNew )
      {
         rc = write( piece, mbContext, cb, dpsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = update( piece, mbContext, cb, dpsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_WRITELOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_WRITE, "_dmsStorageLob::write" )
   INT32 _dmsStorageLob::write( const dmsLobRecord &record,
                                dmsMBContext *mbContext,
                                pmdEDUCB *cb,
                                SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_WRITE ) ;
      SDB_ASSERT( NULL != mbContext && NULL != cb, "can not be null" ) ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      BOOLEAN locked = FALSE ;
      BOOLEAN pageFilled = FALSE ;
      utilCacheContext cContext ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in write, rc: %d", rc ) ;
      }
      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      if ( NULL != dpscb )
      {
         rc = dpsLobW2Record( fullName,
                              record._oid,
                              record._sequence,
                              record._offset,
                              record._hash,
                              record._dataLen,
                              record._data,
                              page,
                              transID,
                              preTransLsn,
                              relatedLsn,
                              logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
            goto error ;
         }

         rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u), rc: %d",
                    logRecord.head()._length, rc ) ;
            info.clear() ;
            goto error ;
         }
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      rc = _allocatePage( record, mbContext, page ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to allocate page in collection:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }

#if defined (_DEBUG)
      SDB_ASSERT( DMS_LOB_PAGE_IN_USED( page ), "must be used" ) ;
#endif

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in write", getSuName() ) ;
         goto error ;
      }
      else if ( record._offset + record._dataLen > getLobdPageSize() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Write record[%s] length more than page size[%u]",
                 record.toString().c_str(), getLobdPageSize() ) ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_INSERT ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      _registerNewWriting() ;

      _pCacheUnit->prepareWrite( page, record._offset,
                                 record._dataLen, cb,
                                 cContext ) ;
      rc = cContext.write( record._data, record._offset, record._dataLen, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write data to collection:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }

      rc = _fillPage( record, page, mbContext ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to fill page, rc:%d", rc ) ;
         goto error ;
      }
      pageFilled = TRUE ;

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(), page, cb ) ;
         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare dps log, rc:%d", rc ) ;
            goto error ;
         }

         if ( locked )
         {
            mbContext->mbUnlock() ;
            locked = FALSE ;
         }
         dpscb->writeData( info ) ;
      }

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      /// submit the data
      cContext.submit( cb ) ;
      /// when write, set the page is newest( now not support part write lob )
      cContext.makeNewest() ;

      if ( 0 != logRecord.head()._length )
      {
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
      }

      if ( SDB_OK == rc && NULL != cb )
      {
         _updateLastLSN( cb->getEndLsn() ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_WRITE, rc ) ;
      return rc ;
   error:
      /// rollback the data
      cContext.release() ;
      /// rollback the page
      if ( DMS_LOB_INVALID_PAGEID != page )
      {
         PD_LOG( PDEVENT, "Rollback lob piece[%s]",
                 record.toString().c_str(), page ) ;
         _rollback( page, mbContext, pageFilled ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_UPDATE, "_dmsStorageLob::update" )
   INT32 _dmsStorageLob::update( const dmsLobRecord &record,
                                 dmsMBContext *mbContext,
                                 pmdEDUCB *cb,
                                 SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_UPDATE ) ;
      SDB_ASSERT( NULL != mbContext && NULL != cb, "can not be null" ) ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      _dmsLobDataMapBlk *blk = NULL ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      CHAR *oldData = NULL ;
      UINT32 oldLen = 0 ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      BOOLEAN locked = FALSE ;
      utilCacheContext cContext ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in update, rc: %d", rc ) ;
      }

      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in update", getSuName() ) ;
         goto error ;
      }
      else if ( record._offset + record._dataLen > getLobdPageSize() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Update record[%s] length more than page size[%u]",
                 record.toString().c_str(), getLobdPageSize() ) ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_INSERT ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      rc = _find( record, mbContext->clLID(), page, blk ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find piece[%s], rc:%d",
                 record.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         PD_LOG( PDERROR, "Can not find piece[%s]",
                 record.toString().c_str() ) ;
         rc = SDB_LOB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

      _registerNewWriting() ;
      /// prepare write
      {
         UINT32 newDataLen = record._dataLen + record._offset ;
         if ( blk->_dataLen > newDataLen )
         {
            newDataLen = blk->_dataLen ;
         }
         _pCacheUnit->prepareWrite( page, 0, newDataLen, cb, cContext ) ;
      }

      if ( NULL != dpscb )
      {
         /// alloc memory
         rc = cb->allocBuff( blk->_dataLen, &oldData, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc read buffer[%u] failed, rc: %d",
                    blk->_dataLen, rc ) ;
            goto error ;
         }
         rc = cContext.readAndCache( oldData, 0, blk->_dataLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to read data from file, rc:%d", rc ) ;
            goto error ;
         }
         /// need to unlock the mbContext, so the sync control is not hold
         /// the mbContext
         if ( locked )
         {
            mbContext->mbUnlock() ;
            locked = FALSE ;
         }

         oldLen = cContext.submit( cb ) ;

         SDB_ASSERT( oldLen == blk->_dataLen, "impossible" ) ;

         rc = dpsLobU2Record( fullName,
                              record._oid,
                              record._sequence,
                              record._offset,
                              record._hash,
                              record._dataLen,
                              record._data,
                              oldLen,
                              oldData,
                              page,
                              transID,
                              preTransLsn,
                              relatedLsn,
                              logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
            goto error ;
         }

         rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u), rc: %d",
                    logRecord.head()._length, rc ) ;
            info.clear() ;
            goto error ;
         }
      }

      rc = cContext.write( record._data, record._offset,
                           record._dataLen, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to write data to collection:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }

      if ( record._dataLen + record._offset > blk->_dataLen )
      {
         blk->_dataLen = record._dataLen + record._offset ;
      }

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(), page, cb ) ;
         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare dps log, rc:%d", rc ) ;
            goto error ;
         }
         dpscb->writeData( info ) ;
      }

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      /// submit the data
      cContext.submit( cb ) ;
      /// when update, set the page is newest
      cContext.makeNewest() ;

      if ( 0 != logRecord.head()._length )
      {
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
      }
      if ( oldData )
      {
         cb->releaseBuff( oldData ) ;
      }
      if ( SDB_OK == rc && NULL != cb )
      {
         _updateLastLSN( cb->getEndLsn() ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_UPDATE, rc ) ;
      return rc ;
   error:
      /// rollback the data
      cContext.release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_READ, "_dmsStorageLob::read" )
   INT32 _dmsStorageLob::read( const dmsLobRecord &record,
                               dmsMBContext *mbContext,
                               pmdEDUCB *cb,
                               CHAR *buf,
                               UINT32 &readLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_READ ) ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      _dmsLobDataMapBlk *blk = NULL ;
      BOOLEAN locked = FALSE ;
      utilCacheContext cContext ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in read, rc: %d", rc ) ;
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in read", getSuName() ) ;
         goto error ;
      }
      else if ( record._offset + record._dataLen > getLobdPageSize() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Read record[%s] length more than page size[%u]",
                 record.toString().c_str(), getLobdPageSize() ) ;
         goto error ;
      }

      rc = _find( record, mbContext->clLID(), page, blk ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find page of record[%s], rc:%d",
                 record.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         rc = SDB_LOB_SEQUENCE_NOT_EXIST ;
         goto error ;
      }

#if defined (_DEBUG)
      SDB_ASSERT( DMS_LOB_PAGE_IN_USED( page ), "must be used" ) ;
#endif
      _pCacheUnit->prepareRead( page, record._offset, record._dataLen,
                                cb, cContext ) ;
      rc = cContext.read( buf, record._offset, record._dataLen, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to read data from file, rc:%d", rc ) ;
         goto error ;
      }

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      /// submit the read data
      readLen = cContext.submit( cb ) ;
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_READ, rc ) ;
      return rc ;
   error:
      /// rollback the read data
      cContext.release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ALLOCATEPAGE, "_dmsStorageLob::_allocatePage" )
   INT32 _dmsStorageLob::_allocatePage( const dmsLobRecord &record,
                                        dmsMBContext *context,
                                        DMS_LOB_PAGEID &page )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ALLOCATEPAGE ) ;
      SDB_ASSERT( NULL != record._oid && 0 <= record._sequence &&
                  record._dataLen + record._offset <= getLobdPageSize() &&
                  0 == record._offset, "invalid lob record" ) ;

      rc = _findFreeSpace( 1, page, context ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find free space, rc:%d", rc ) ;
         goto error ;
      }

      /// add lob page
      context->mbStat()->_totalLobPages += 1 ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ALLOCATEPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__FILLPAGE, "_dmsStorageLob::_fillPage" )
   INT32 _dmsStorageLob::_fillPage( const dmsLobRecord &record,
                                    DMS_LOB_PAGEID page,
                                    dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__FILLPAGE ) ;
      _dmsLobDataMapBlk *blk = NULL ;

      ossValuePtr extent = extentAddr( page ) ;
      if ( !extent )
      {
         PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                 page ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      blk = DMS_LOB_META( extent ) ;
      /// must first set clLogiclID
      blk->_clLogicalID = context->clLID() ;
      blk->_mbID = context->mbID() ;

      ossMemset( blk->_pad1, 0, sizeof( blk->_pad1 ) ) ;
      ossMemset( blk->_pad2, 0, sizeof( blk->_pad2 ) ) ;
      ossMemcpy( blk->_oid, record._oid, DMS_LOB_OID_LEN ) ;
      blk->_sequence = record._sequence ;
      blk->_dataLen = record._dataLen + record._offset ;
      blk->_prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
      blk->_nextPageInBucket = DMS_LOB_INVALID_PAGEID ;

#if defined (_DEBUG)
      {
         UINT32 __hash = 0 ;
         DMS_LOB_GET_HASH_FROM_BLK( blk, __hash ) ;
         if ( __hash != record._hash )
         {
            dmsLobDataMapBlk memBlk ;
            ossMemcpy( &memBlk, blk, sizeof( memBlk ) ) ;
            SDB_ASSERT( __hash == record._hash, "must be same" ) ;
         }
      }
#endif

      rc = _push2Bucket( _getBucket( record._hash ),
                         page, *blk, &record ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to push page[%d] to bucket[%d], rc: %d",
                 page, _getBucket( record._hash ), rc ) ;
         goto error ;
      }

      /// add stat
      if ( DMS_LOB_META_SEQUENCE == record._sequence )
      {
         context->mbStat()->_totalLobs++ ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__FILLPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_REMOVE, "_dmsStorageLob::remove" )
   INT32 _dmsStorageLob::remove( const dmsLobRecord &record,
                                 dmsMBContext *mbContext,
                                 pmdEDUCB *cb,
                                 SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_REMOVE ) ;
      UINT32 bucketNumber = 0 ;
      _dmsLobDataMapBlk *blk = NULL ;
      DMS_LOB_PAGEID page = DMS_LOB_INVALID_PAGEID ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      UINT32 resevedLength = 0 ;
      DPS_TRANS_ID transID = DPS_INVALID_TRANS_ID ;
      DPS_LSN_OFFSET preTransLsn = DPS_INVALID_LSN_OFFSET ;
      DPS_LSN_OFFSET relatedLsn = DPS_INVALID_LSN_OFFSET ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      CHAR *oldData = NULL ;
      UINT32 oldLen = 0 ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      BOOLEAN locked = FALSE ;
      BOOLEAN hasRemoved = FALSE ;
      utilCacheContext cContext ;
      UINT64 beginLSN = 0 ;
      UINT64 endLSN = 0 ;
      ossSpinSLatch *pLatch = NULL ;

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in remove, rc: %d", rc ) ;
      }

      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      /// First to checkSyncControl and reserveLogSpace by a pageSize
      /// And this is outside of collection latch
      if ( dpscb )
      {
         oldLen = getLobdPageSize() ;
         rc = dpsLobRm2Record( fullName,
                               record._oid,
                               record._sequence,
                               record._offset,
                               record._hash,
                               oldLen,
                               oldData,
                               page,
                               transID,
                               preTransLsn,
                               relatedLsn,
                               logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
            goto error ;
         }

         rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u), rc: %d",
                    logRecord.head()._length, rc ) ;
            goto error ;
         }
         resevedLength = logRecord.head()._length ;
         /// clear log info
         oldLen = 0 ;
         info.clear() ;
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in remove", getSuName() ) ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_DELETE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      rc = _find( record, mbContext->clLID(), page, blk, &bucketNumber ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to find record[%s], rc:%d",
                 record.toString().c_str(), rc ) ;
         goto error ;
      }

      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         goto done ;
      }

      _registerNewWriting() ;

      /// When dpscb is NULL, not to alloc the page when page is
      /// not in cache( use len = 0 )
      _pCacheUnit->prepareWrite( page, 0, dpscb ? blk->_dataLen : 0,
                                 cb, cContext ) ;
      if ( dpscb )
      {
         /// alloc memory
         rc = cb->allocBuff( blk->_dataLen, &oldData, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Alloc read buffer[%u] failed, rc: %d",
                    blk->_dataLen, rc ) ;
            goto error ;
         }
         rc = cContext.read( oldData, 0, blk->_dataLen, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to read data from file, rc:%d", rc ) ;
            goto error ;
         }
      }

      /// lock bucket
      if ( dpscb )
      {
         pLatch = _getBucketLatch( bucketNumber ) ;
         pLatch->get() ;
      }

      /// remove and release the page
      rc = _removePage( page, blk, &bucketNumber, mbContext,
                        pLatch ? TRUE : FALSE, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to remove page:%d, rc:%d", page, rc ) ;
         goto error ;
      }
      hasRemoved = TRUE ;

      /// release the mbContext
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }

      if ( dpscb )
      {
         /// submit the read data
         oldLen = cContext.submit( cb ) ;

         rc = dpsLobRm2Record( fullName,
                               record._oid,
                               record._sequence,
                               record._offset,
                               record._hash,
                               oldLen,
                               oldData,
                               page,
                               transID,
                               preTransLsn,
                               relatedLsn,
                               logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to build dps log, rc:%d", rc ) ;
            goto error ;
         }

         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(), mbContext->clLID(), page, cb ) ;
         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to prepare dps log, rc:%d", rc ) ;
            goto error ;
         }

         /// release bucket lock
         pLatch->release() ;
         pLatch = NULL ;
         /// write
         dpscb->writeData( info ) ;
      }

      /// discard the page
      cContext.discardPage( beginLSN, endLSN ) ;
      /// release the context and then lock mbContext again
      cContext.release() ;

   done:
      if ( pLatch )
      {
         pLatch->release() ;
         pLatch = NULL ;
      }
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      if ( 0 != resevedLength )
      {
         transCB->releaseLogSpace( resevedLength, cb ) ;
      }
      if ( oldData )
      {
         cb->releaseBuff( oldData ) ;
      }
      if ( SDB_OK == rc && NULL != cb )
      {
         _updateLastLSN( cb->getEndLsn() ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_REMOVE, rc ) ;
      return rc ;
   error:
      if ( hasRemoved )
      {
         ossPanic() ;
      }
      goto done ;
   }

   INT32 _dmsStorageLob::_releasePage( DMS_LOB_PAGEID page,
                                       dmsMBContext *context )
   {
      context->mbStat()->_totalLobPages -= 1 ;
      return _releaseSpace( page, 1 ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__FIND, "_dmsStorageLob::_find" )
   INT32 _dmsStorageLob::_find( const _dmsLobRecord &record,
                                UINT32 clID,
                                DMS_LOB_PAGEID &page,
                                _dmsLobDataMapBlk *&lobBlk,
                                UINT32 *bucket )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__FIND ) ;
      UINT32 bucketNumber = _getBucket( record._hash ) ;
      ossScopedLock lock( _getBucketLatch( bucketNumber ), SHARED ) ;

      DMS_LOB_PAGEID pageInBucket = _dmsBME->_buckets[bucketNumber] ;
      while ( DMS_LOB_INVALID_PAGEID != pageInBucket )
      {
         _dmsLobDataMapBlk *blk = NULL ;
         ossValuePtr extent = extentAddr( pageInBucket ) ;
         if ( !extent )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                    "pageid:%d", pageInBucket ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         blk = DMS_LOB_META( extent ) ;
#if defined (_DEBUG)
         {
            UINT32 __hash = 0 ;
            DMS_LOB_GET_HASH_FROM_BLK( blk, __hash ) ;
            UINT32 testBucketNo = _getBucket( __hash ) ;
            if ( testBucketNo != bucketNumber )
            {
               dmsLobDataMapBlk memBlk ;
               ossMemcpy( &memBlk, blk, sizeof( memBlk ) ) ;
               SDB_ASSERT( testBucketNo == bucketNumber, "must be same" ) ;
            }
         }
#endif
         if ( clID == blk->_clLogicalID &&
              blk->equals( record._oid->getData(), record._sequence ) )
         {
            page = pageInBucket ;
            lobBlk = blk ;
            break ;
         }
         else
         {
            pageInBucket = blk->_nextPageInBucket ;
            continue ; 
         }
      }

      if ( DMS_LOB_INVALID_PAGEID != pageInBucket && NULL != bucket )
      {
         *bucket = bucketNumber ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__FIND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__PUSH2BUCKET, "_dmsStorageLob::_push2Bucket" )
   INT32 _dmsStorageLob::_push2Bucket( UINT32 bucket,
                                       DMS_LOB_PAGEID pageId,
                                       _dmsLobDataMapBlk &blk,
                                       const dmsLobRecord *pRecord )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__PUSH2BUCKET ) ;

      blk.setNormal() ;

      ossScopedLock lock( _getBucketLatch( bucket ), EXCLUSIVE ) ;

      DMS_LOB_PAGEID &pageInBucket = _dmsBME->_buckets[bucket] ;
      /// empty bucket
      if ( DMS_LOB_INVALID_PAGEID == pageInBucket )
      {
         pageInBucket = pageId ;
         blk._prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
         blk._nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
      }
      /// neet to find the last one
      else
      {
         DMS_LOB_PAGEID tmpPage = pageInBucket ;
         _dmsLobDataMapBlk *lastBlk = NULL ;
         ossValuePtr extent = 0 ;
         do
         {
            extent = extentAddr( tmpPage ) ;
            if ( !extent )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                       tmpPage ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            lastBlk = DMS_LOB_META( extent ) ;
            /// check exist
            if ( pRecord &&
                 blk._clLogicalID == lastBlk->_clLogicalID &&
                 lastBlk->equals( pRecord->_oid->getData(),
                                  pRecord->_sequence ) )
            {
               PD_LOG( PDERROR, "Lob piece found, piece[%s], page:%d",
                       pRecord->toString().c_str(), tmpPage ) ;
               rc = SDB_LOB_SEQUENCE_EXISTS ;
               goto error ;
            }

            if ( DMS_LOB_INVALID_PAGEID == lastBlk->_nextPageInBucket )
            {
               lastBlk->_nextPageInBucket = pageId ;
               blk._prevPageInBucket = tmpPage ;
               blk._nextPageInBucket = DMS_LOB_INVALID_PAGEID ;
               break ;
            }
            else
            {
               tmpPage = lastBlk->_nextPageInBucket ;
               continue ;
            }
         } while ( TRUE ) ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__PUSH2BUCKET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ONCREATE, "_dmsStorageLob::_onCreate" )
   INT32 _dmsStorageLob::_onCreate( OSSFILE *file, UINT64 curOffSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ONCREATE ) ;
      SDB_ASSERT( DMS_BME_OFFSET == curOffSet, "invalid offset" ) ;
      _dmsBucketsManagementExtent *bme = NULL ;

      bme = SDB_OSS_NEW _dmsBucketsManagementExtent() ;
      if ( NULL == bme )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _writeFile ( file, (const CHAR *)(bme),
                        sizeof( _dmsBucketsManagementExtent ) ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "failed to write new bme to file rc: %d",
                  rc ) ;
         goto error ;
      }
   done:
      if ( NULL != bme )
      {
         SDB_OSS_DEL bme ;
         bme = NULL ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ONCREATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ONMAPMETA, "_dmsStorageLob::_onMapMeta" )
   INT32 _dmsStorageLob::_onMapMeta( UINT64 curOffSet )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ONMAPMETA ) ;
      rc = map ( DMS_BME_OFFSET, DMS_BME_SZ, (void**)&_dmsBME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map BME: %s", getSuFileName() ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ONMAPMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   UINT32 _dmsStorageLob::_getSegmentSize() const
   {
      SDB_ASSERT( 0 != _segmentSize, "not initialized" ) ;
      return _segmentSize ;
   }

   UINT32 _dmsStorageLob::_extendThreshold() const
   {
      if ( _pStorageInfo )
      {
         return _pStorageInfo->_extentThreshold >> _data.pageSizeSquareRoot() ;
      }
      return (UINT32)( DMS_LOB_EXTEND_THRESHOLD_SIZE >> pageSizeSquareRoot() ) ;
   }

   UINT64 _dmsStorageLob::_dataOffset()
   {
      return DMS_BME_OFFSET + DMS_BME_SZ ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__EXTENDSEGMENTS, "_dmsStorageLob::_extendSegments" )
   INT32 _dmsStorageLob::_extendSegments( UINT32 numSeg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__EXTENDSEGMENTS ) ;

      /// When in restore from crash, the _data is not open,
      /// so can't extend
      if ( _data.isOpened() )
      {
         INT64 extentLen = _data.getSegmentSize() ;
         rc = _data.extend( extentLen * numSeg ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extend lobd file:%d", rc ) ;
            goto error ;
         }
      }

      rc = this->_dmsStorageBase::_extendSegments( numSeg ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extend lobm file:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__EXTENDSEGMENTS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _dmsStorageLob::_getEyeCatcher() const
   {
      return DMS_LOBM_EYECATCHER ;
   }

   UINT32 _dmsStorageLob::_curVersion() const
   {
      return DMS_LOB_CUR_VERSION ;
   }

   INT32 _dmsStorageLob::_checkVersion( dmsStorageUnitHeader *pHeader )
   {
      INT32 rc = SDB_OK ;
      if ( pHeader->_version > _curVersion() )
      {
         PD_LOG( PDERROR, "Incompatible version: %u", pHeader->_version ) ;
         rc = SDB_DMS_INCOMPATIBLE_VERSION ;
      }
      else if ( pHeader->_secretValue != _pStorageInfo->_secretValue )
      {
         PD_LOG( PDERROR, "Secret value[%llu] not the same with data su[%llu]",
                 pHeader->_secretValue, _pStorageInfo->_secretValue ) ;
         rc = SDB_DMS_SECRETVALUE_NOT_SAME ;
      }
      return rc ;
   }

   void _dmsStorageLob::_onClosed()
   {
      _data.close() ;
      _dmsBME = NULL ;
      return ;
   }

   void _dmsStorageLob::_initHeaderPageSize( dmsStorageUnitHeader * pHeader,
                                             dmsStorageInfo * pInfo )
   {
      pHeader->_pageSize = DMS_PAGE_SIZE64B ;
      pHeader->_lobdPageSize = pInfo->_lobdPageSize ;
   }

   INT32 _dmsStorageLob::_checkPageSize( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      if ( pHeader->_pageSize != DMS_PAGE_SIZE64B &&
           pHeader->_pageSize != DMS_PAGE_SIZE256B )
      {
         PD_LOG( PDERROR, "Lob meta page size[%d] must be %d or %d in file[%s]",
                 pHeader->_pageSize,
                 DMS_PAGE_SIZE64B, DMS_PAGE_SIZE256B,
                 getSuFileName() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else if ( DMS_PAGE_SIZE4K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE8K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE16K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE32K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE64K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE128K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE256K != pHeader->_lobdPageSize &&
                DMS_PAGE_SIZE512K != pHeader->_lobdPageSize )
      {
         PD_LOG ( PDERROR, "Invalid lob page size: %d in file[%s], lob page "
                  "size must be one of 4K/8K/16K/32K/64K/128K/256K/512K",
                 pHeader->_lobdPageSize, getSuFileName() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _segmentSize = _data.getSegmentSize() / pHeader->_lobdPageSize *
                     pHeader->_pageSize ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_CALCCOUNT, "_dmsStorageLob::_calcCount" )
   INT32 _dmsStorageLob::_calcCount()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_CALCCOUNT ) ;
      DMS_LOB_PAGEID current = 0 ;

      /// clear all lob count
      for( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _dmsData->_mbStatInfo[i]._totalLobs = 0 ;
      }

      /// re-count
      while ( current < (INT32)pageNum() )
      {
         if ( DMS_LOB_PAGE_IN_USED( current ) )
         {
            _dmsLobDataMapBlk *blk = NULL ;
            dmsMB *mb = NULL ;
            ossValuePtr extent = extentAddr( current ) ;
            if ( !extent )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                       current ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            blk = DMS_LOB_META( extent ) ;
            if ( blk->_mbID >= DMS_MME_SLOTS || blk->_mbID < 0 ||
                 blk->isUndefined() )
            {
               ++current ;
               continue ;
            }
            mb = &(_dmsData->_dmsMME->_mbList[ blk->_mbID ] ) ;
            if ( mb->_logicalID != blk->_clLogicalID ||
                 DMS_LOB_META_SEQUENCE != blk->_sequence )
            {
               ++current ;
               continue ;
            }

            /// stat
            _dmsData->_mbStatInfo[blk->_mbID]._totalLobs += 1 ;
         }
         ++current ;
      }
      /// flush MME
      _dmsData->flushMME( TRUE ) ;

      /// update the header
      _dmsHeader->_version = DMS_LOB_CUR_VERSION ;
      flushHeader( TRUE ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_CALCCOUNT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_READPAGE, "_dmsStorageLob::_readPage" )
   INT32 _dmsStorageLob::readPage( DMS_LOB_PAGEID &pos,
                                   BOOLEAN onlyMetaPage,
                                   _pmdEDUCB *cb,
                                   dmsMBContext *mbContext,
                                   dmsLobInfoOnPage &page )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_READPAGE ) ;
      DMS_LOB_PAGEID current = pos ;
      BOOLEAN locked = FALSE ;

      if ( DMS_LOB_INVALID_PAGEID == current )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _needDelayOpen )
      {
         rc = _delayOpen() ;
         PD_RC_CHECK( rc, PDERROR, "Delay open failed in read, rc: %d", rc ) ;
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( SHARED ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get lock:%d", rc ) ;
            goto error ;
         }
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in read", getSuName() ) ;
         goto error ;
      }

      do
      {
         if ( pageNum() <= ( UINT32 )current )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
         else if ( DMS_LOB_PAGE_IN_USED( current ) )
         {
            _dmsLobDataMapBlk *blk = NULL ;
            ossValuePtr extent = extentAddr( current ) ;
            if ( !extent )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                       "pageid:%d", current ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            blk = DMS_LOB_META( extent ) ;
            /// first check undefined
            if ( blk->isUndefined() )
            {
               ++current ;
               continue ;
            }
            /// then check clLID
            else if ( mbContext->clLID() != blk->_clLogicalID ||
                      ( onlyMetaPage &&
                        DMS_LOB_META_SEQUENCE != blk->_sequence ) )
            {
               ++current ;
               continue ;
            }

            ossMemcpy( &( page._oid ), blk->_oid, sizeof( page._oid ) ) ;
            page._sequence = blk->_sequence ;
            page._len = blk->_dataLen ;
            ++current ;
            break ;
         }
         else
         {
            /// not allocated.
            ++current ;
         }
      } while ( TRUE ) ;

      /// point to the next.
      pos = current ;
   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_READPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__REMOVEPAGE, "_dmsStorageLob::_removePage" )
   INT32 _dmsStorageLob::_removePage( DMS_LOB_PAGEID page,
                                      _dmsLobDataMapBlk *blk,
                                      const UINT32 *bucket,
                                      dmsMBContext *mbContext,
                                      BOOLEAN hasLockBucket,
                                      BOOLEAN needRelease )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__REMOVEPAGE ) ;
      UINT32 bucketNumber = 0 ;     

      if ( NULL != bucket )
      {
         bucketNumber = *bucket ;
      }
      else
      {
         UINT32 __hash1 = 0 ;
         DMS_LOB_GET_HASH_FROM_BLK( blk, __hash1 ) ;
         bucketNumber = _getBucket( __hash1 ) ;
      }

      /// lock
      if ( !hasLockBucket )
      {
         _getBucketLatch( bucketNumber )->get() ;
      }

      if ( DMS_LOB_INVALID_PAGEID == blk->_prevPageInBucket )
      {
         SDB_ASSERT( _dmsBME->_buckets[bucketNumber] == page,
                     "must be this page" ) ;
         _dmsBME->_buckets[bucketNumber] = blk->_nextPageInBucket ;
         if ( DMS_LOB_INVALID_PAGEID != blk->_nextPageInBucket )
         {
            _dmsLobDataMapBlk *nextBlk = NULL ;
            ossValuePtr nextExtent = extentAddr( blk->_nextPageInBucket ) ;
            if ( !nextExtent )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                       "pageid:%d", blk->_nextPageInBucket ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            nextBlk = DMS_LOB_META( nextExtent ) ;
            nextBlk->_prevPageInBucket = DMS_LOB_INVALID_PAGEID ;
         }
      }
      else
      {
         _dmsLobDataMapBlk *lastBlk = NULL ;
         ossValuePtr lastExtent = extentAddr( blk->_prevPageInBucket ) ;
         if ( !lastExtent )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                    blk->_prevPageInBucket ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         lastBlk = DMS_LOB_META( lastExtent ) ;
         lastBlk->_nextPageInBucket = blk->_nextPageInBucket ;

         if ( DMS_LOB_INVALID_PAGEID != blk->_nextPageInBucket )
         {
            _dmsLobDataMapBlk *nextBlk = NULL ;
            ossValuePtr nextExtent = extentAddr( blk->_nextPageInBucket ) ;
            if ( !nextExtent )
            {
               PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                       blk->_nextPageInBucket ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            nextBlk = DMS_LOB_META( nextExtent ) ;
            nextBlk->_prevPageInBucket = blk->_prevPageInBucket ;
         }
      }

      if ( DMS_LOB_META_SEQUENCE == blk->_sequence )
      {
         mbContext->mbStat()->_totalLobs -= 1 ;
      }

      blk->reset() ;
      blk->setRemoved() ;

      /// release the page
      if ( needRelease )
      {
         _releasePage( page, mbContext ) ;
      }
   done:
      if ( !hasLockBucket )
      {
         _getBucketLatch( bucketNumber )->release() ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__REMOVEPAGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_TRUNCATE, "_dmsStorageLob::truncate" )
   INT32 _dmsStorageLob::truncate( dmsMBContext *mbContext,
                                   _pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_TRUNCATE ) ;

      DMS_LOB_PAGEID current = -1 ;
      BOOLEAN locked = FALSE ;
      BOOLEAN needPanic = FALSE ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord = info.getMergeBlock().record() ;
      dpsTransCB *transCB = pmdGetKRCB()->getTransCB() ;
      utilCacheContext cContext ;
      UINT64 beginLSN = 0 ;
      UINT64 endLSN = 0 ;

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in remove", getSuName() ) ;
         goto error ;
      }

      /// make full name
      _clFullName( mbContext->mb()->_collectionName, fullName,
                   sizeof( fullName ) ) ;

      if ( NULL != dpscb )
      {
         rc = dpsLobTruncate2Record( fullName, logRecord ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build dps log:%d", rc ) ;
            goto error ;
         }

         rc = dpscb->checkSyncControl( logRecord.head()._length, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "check sync control failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = transCB->reservedLogSpace( logRecord.head()._length, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "failed to reserved log space(length=%u)",
                    logRecord.head()._length ) ;
            info.clear() ;
            goto error ;
         }
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      _registerNewWriting() ;
      needPanic = TRUE ;
      while ( ( UINT32 )++current < pageNum() )
      {
         if ( !DMS_LOB_PAGE_IN_USED( current ) )
         {
            continue ;
         }

         _dmsLobDataMapBlk *blk = NULL ;
         ossValuePtr extent = extentAddr( current ) ;
         if ( !extent )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), pageid:%d",
                    current ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         blk = DMS_LOB_META( extent ) ;
         /// need first check undefined
         if ( blk->isUndefined() )
         {
            continue ;
         }
         /// then check clLID
         else if ( mbContext->clLID() != blk->_clLogicalID )
         {
            continue ; 
         }

         rc = _removePage( current, blk, NULL, mbContext, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove page:%d, rc:%d", rc ) ;
            goto error ;
         }
         /// when the page is dirty, dicard the page, size is 0, will not
         /// alloc the page when page is not in memory
         _pCacheUnit->prepareWrite( current, 0, 0, cb, cContext ) ;
         cContext.discardPage( beginLSN, endLSN ) ;
         cContext.release() ;
      }

      // clear the stat info
      mbContext->mbStat()->_totalLobPages = 0 ;
      mbContext->mbStat()->_totalLobs = 0 ;

      if ( NULL != dpscb )
      {
         SDB_ASSERT( NULL != _dmsData, "can not be null" ) ;
         info.setInfoEx( _dmsData->logicalID(),
                         mbContext->clLID(),
                         DMS_INVALID_EXTENT, cb ) ;

         rc = dpscb->prepare( info ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to prepare dps log:%d", rc ) ;
            goto error ;
         }

         if ( locked )
         {
            mbContext->mbUnlock() ;
            locked = FALSE ;
         }
         dpscb->writeData( info ) ;
      }

   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      if ( 0 != logRecord.head()._length )
      {
         transCB->releaseLogSpace( logRecord.head()._length, cb ) ;
      }

      if ( SDB_OK == rc && NULL != cb )
      {
         _updateLastLSN( cb->getEndLsn() ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_TRUNCATE, rc ) ;
      return rc ;
   error:
      if ( needPanic )
      {
         PD_LOG( PDSEVERE, "we must panic db now, we got a lrreparable "
                 "error" ) ;
         ossPanic() ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB__ROLLBACK, "_dmsStorageLob::_rollback" )
   INT32 _dmsStorageLob::_rollback( DMS_LOB_PAGEID page,
                                    dmsMBContext *mbContext,
                                    BOOLEAN pageFilled )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB__ROLLBACK ) ;
      BOOLEAN locked = FALSE ;
      if ( DMS_LOB_INVALID_PAGEID == page )
      {
         goto done ;
      }

      if ( !mbContext->isMBLock() )
      {
         rc = mbContext->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         locked = TRUE ;
      }

      if ( !isOpened() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "File[%s] is not open in write", getSuName() ) ;
         goto error ;
      }

      if ( pageFilled )
      {
         _dmsLobDataMapBlk *blk = NULL ;
         ossValuePtr extent = extentAddr( page ) ;
         if ( !extent )
         {
            PD_LOG( PDERROR, "we got a NULL extent from extendAddr(), "
                    "pageid:%d", page ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         blk = DMS_LOB_META( extent ) ;
         rc = _removePage( page, blk, NULL, mbContext, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove page:%d, rc:%d", page, rc ) ;
            goto error ;
         }
      }
      else
      {
         _releasePage( page, mbContext ) ;
      }
   done:
      if ( locked )
      {
         mbContext->mbUnlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB__ROLLBACK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_ONFLUSHDIRTY, "_dmsStorageLob::_onFlushDirty" )
   INT32 _dmsStorageLob::_onFlushDirty( BOOLEAN sync )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_ONFLUSHDIRTY ) ;
      if ( !isOpened() )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         _data.flush() ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_ONFLUSHDIRTY, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGELOB_TRYTOFLUSH, "_dmsStorageLob::tryToFlush" )
   INT32 _dmsStorageLob::tryToFlush( BOOLEAN ignoreTick, BOOLEAN &failed )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGELOB_TRYTOFLUSH ) ;
      BOOLEAN locked = FALSE ;

      if ( !isOpened() )
      {
         failed = FALSE ;
         goto done ;
      }

      if ( !ignoreTick && !_noWriteForAWhile() )
      {
         failed = TRUE ;
         goto done ;
      }

      _validFlag = 1 ;

      rc = _data.flush() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to flush lob data:%d", rc ) ;
         goto error ;
      }

      rc = flushAll( TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to flush dirty segments:%d", rc ) ;
         goto error ;
      }

      if ( 0 ==_validFlag )
      {
         failed = TRUE ;
      }
      else
      {
         ossLatch ( &_pagecleanerLatch ) ;
         locked = TRUE ;
         if ( 0 ==_validFlag )
         {
            failed = TRUE ;
            goto done ;
         }
         _markHeaderValid() ;
         failed = FALSE ;
      }
   done:
      if ( locked )
      {
         ossUnlatch( &_pagecleanerLatch ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGELOB_TRYTOFLUSH, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

