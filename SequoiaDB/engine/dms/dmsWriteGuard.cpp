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

   Source File Name = dmsWriteGuard.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsWriteGuard.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageDataCommon.hpp"

using namespace std ;

namespace engine
{

   /*
      _dmsDataWriteGuard imeplement
    */
   _dmsDataWriteGuard::_dmsDataWriteGuard()
   : _su( nullptr ),
     _mbID( DMS_INVALID_MBID ),
     _eduCB( nullptr ),
     _isEnabled( FALSE ),
     _isInWrite( FALSE )
   {
   }

   _dmsDataWriteGuard::_dmsDataWriteGuard( dmsStorageDataCommon *su,
                                           dmsMBContext *mbContext,
                                           pmdEDUCB *cb,
                                           BOOLEAN isEnabled )
   : _su( su ),
     _mbStat( mbContext->mbStat() ),
     _mbID( mbContext->mbID() ),
     _eduCB( cb ),
     _isEnabled( isEnabled ),
     _isInWrite( FALSE )
   {
   }

   _dmsDataWriteGuard::~_dmsDataWriteGuard()
   {
      abort() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_BEFOREWRITE, "_dmsDataWriteGuard::beforeWrite" )
   void _dmsDataWriteGuard::beforeWrite()
   {
      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_BEFOREWRITE ) ;

      if ( _isEnabled && !_isInWrite )
      {
         _su->markDirty( _mbID, DMS_CHG_BEFORE ) ;
         _su->incWritePtrCount( _mbID ) ;
         _mbStat->_snapshotID.inc() ;
         _isInWrite = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSDATAWRITEGUARD_BEFOREWRITE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_AFTERWRITE, "_dmsDataWriteGuard::afterWrite" )
   void _dmsDataWriteGuard::afterWrite()
   {
      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_AFTERWRITE ) ;

      if ( _isEnabled && _isInWrite )
      {
         _su->markDirty( _mbID, DMS_CHG_AFTER ) ;
         _su->decWritePtrCount( _mbID ) ;
         _isInWrite = FALSE ;
      }

      PD_TRACE_EXIT( SDB__DMSDATAWRITEGUARD_AFTERWRITE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_BEGIN, "_dmsDataWriteGuard::begin" )
   INT32 _dmsDataWriteGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_BEGIN ) ;

      beforeWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_BEGIN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_COMMIT, "_dmsDataWriteGuard::commit" )
   INT32 _dmsDataWriteGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_COMMIT ) ;

      afterWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_COMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_ABORT, "_dmsDataWriteGuard::abort" )
   INT32 _dmsDataWriteGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_ABORT ) ;

      afterWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_ABORT, rc ) ;

      return rc ;
   }

   /*
      _dmsIndexWriteGaurd imeplement
    */
   _dmsIndexWriteGuard::_dmsIndexWriteGuard()
   : _eduCB( nullptr ),
     _isEnabled( FALSE )
   {
   }

   _dmsIndexWriteGuard::_dmsIndexWriteGuard( pmdEDUCB *cb, BOOLEAN isEnabled )
   : _eduCB( cb ),
     _isEnabled( isEnabled )
   {
   }

   _dmsIndexWriteGuard::~_dmsIndexWriteGuard()
   {
      abort() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_LOCK, "_dmsIndexWriteGuard::lock" )
   INT32 _dmsIndexWriteGuard::lock( const dmsIdxMetadataKey &metadataKey,
                                    const ixmIndexCB &indexCB,
                                    const dmsRecordID &rid,
                                    shared_ptr<ossRWMutex> &lockPtr,
                                    BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_LOCK ) ;

      needProcess = TRUE ;

      // already locked
      if ( _locks.count( metadataKey ) > 0 )
      {
         needProcess = ( rid <= indexCB.getScanRID() ? TRUE : FALSE ) ;
         goto done ;
      }

      while ( TRUE )
      {
         if ( _eduCB->isInterrupted() )
         {
            PD_RC_CHECK( rc, PDERROR, "Failed to lock index write guard, "
                         "interrupted" ) ;
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }
         rc = lockPtr->lock_r( OSS_ONE_SEC ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }
         else if ( SDB_TIMEOUT == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to lock index write guard, rc: %d", rc ) ;
      }

      if ( rid <= indexCB.getScanRID() )
      {
         needProcess = TRUE ;
         lockPtr->release_r() ;
         goto done ;
      }
      else
      {
         needProcess = FALSE ;
      }

      try
      {
         _locks.insert( make_pair( metadataKey, lockPtr ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save lock to index write guard, "
                 "occur exception: %s", e.what() ) ;
         lockPtr->release_r() ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_LOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_RELEASEALL, "_dmsIndexWriteGuard::releaseAll" )
   void _dmsIndexWriteGuard::releaseAll()
   {
      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_RELEASEALL ) ;

      for ( dmsIdxBuildLockMapIter iter = _locks.begin() ;
            iter != _locks.end() ;
            ++ iter )
      {
         iter->second->release_r() ;
      }
      _locks.clear() ;

      PD_TRACE_EXIT( SDB__DMSINDEXWRITEGUARD_RELEASEALL ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_BEGIN, "_dmsIndexWriteGuard::begin" )
   INT32 _dmsIndexWriteGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_BEGIN ) ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_BEGIN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_COMMIT, "_dmsIndexWriteGuard::commit" )
   INT32 _dmsIndexWriteGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_COMMIT ) ;

      releaseAll() ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_COMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_ABORT, "_dmsIndexWriteGuard::abort" )
   INT32 _dmsIndexWriteGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_ABORT ) ;

      releaseAll() ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_ABORT, rc ) ;

      return rc ;
   }

   /*
      _dmsPersistGuard implement
    */
   _dmsPersistGuard::_dmsPersistGuard()
   : _service( nullptr ),
     _persistUnit( nullptr ),
     _su( nullptr ),
     _mbStat( nullptr ),
     _eduCB( nullptr ),
     _isEnabled( FALSE ),
     _hasBegin( FALSE )
   {
   }

   _dmsPersistGuard::_dmsPersistGuard( IStorageService *service,
                                       dmsStorageDataCommon *su,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB *cb,
                                       BOOLEAN isEnabled )
   : _service( service ),
     _persistUnit( nullptr ),
     _su( su ),
     _mbStat( mbContext ? mbContext->mbStat() : nullptr ),
     _clUniqueID( mbContext ? mbContext->getCLUniqueID() : UTIL_UNIQUEID_NULL ),
     _eduCB( cb ),
     _isEnabled( isEnabled ),
     _hasBegin( FALSE )
   {
   }

   _dmsPersistGuard::~_dmsPersistGuard()
   {
      if ( _hasBegin )
      {
         abort() ;
      }
      else
      {
         fini() ;
      }
      if ( _dummySession.eduCB() )
      {
         _dummySession.detachCB() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_INIT, "_dmsPersistGuard::init" )
   INT32 _dmsPersistGuard::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_INIT ) ;

      PD_CHECK( !_persistUnit, SDB_SYS, error, PDERROR,
                "Failed to begin persist guard, already in guard" ) ;

      if ( !_isEnabled || !_service )
      {
         goto done ;
      }

      if ( !_eduCB->getSession() )
      {
         _dummySession.attachCB( _eduCB ) ;
      }
      rc = _service->getPersistUnit( _eduCB, _persistUnit ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_FINI, "_dmsPersistGuard::fini" )
   INT32 _dmsPersistGuard::fini()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_FINI ) ;

      _persistUnit = nullptr ;

      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_FINI, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_BEGIN, "_dmsPersistGuard::begin" )
   INT32 _dmsPersistGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_BEGIN ) ;

      if ( !_isEnabled || !_service )
      {
         goto done ;
      }

      if ( !_persistUnit )
      {
         if ( !_eduCB->getSession() )
         {
            _dummySession.attachCB( _eduCB ) ;
         }
         rc = _service->getPersistUnit( _eduCB, _persistUnit ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc ) ;
      }

      if ( _persistUnit )
      {
         rc = _persistUnit->beginUnit( _eduCB, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to begin persist unit, rc: %d", rc ) ;

         if ( NULL != _su && NULL != _mbStat )
         {
            utilThreadLocalPtr<IStatPersistUnit> statUnitPtr ;
            _statUnitPtr = dmsStatPersistUnit::makeThreadLocalPtr( _clUniqueID, _su, _mbStat ) ;
            PD_CHECK( _statUnitPtr, SDB_OOM, error, PDERROR,
                      "Failed to make statistics persist unit" ) ;
            statUnitPtr = _statUnitPtr ;
            PD_CHECK( _statUnitPtr, SDB_SYS, error, PDERROR,
                      "Failed to convert statistics persist unit" ) ;
            rc = _persistUnit->registerStatUnit( statUnitPtr ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to register stat persist unit, rc: %d", rc ) ;
         }

         _hasBegin = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_BEGIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_COMMIT, "_dmsPersistGuard::commit" )
   INT32 _dmsPersistGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_COMMIT ) ;

      if ( _isEnabled && _persistUnit && _hasBegin )
      {
         rc = _persistUnit->commitUnit( _eduCB, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to commit persist unit, rc: %d", rc ) ;

         _hasBegin = FALSE ;
         _persistUnit = nullptr ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_COMMIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_ABORT, "_dmsPersistGuard::abort" )
   INT32 _dmsPersistGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_ABORT ) ;

      if ( _isEnabled && _persistUnit && _hasBegin )
      {
         rc = _persistUnit->abortUnit( _eduCB, FALSE, isForced ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to abort persist unit, rc: %d", rc ) ;

         _hasBegin = FALSE ;
         _persistUnit = nullptr ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_ABORT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _dmsPersistGuard::incRecordCount( UINT64 count )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incRecordCount( count ) ;
      }
      else if ( UTIL_UNIQUEID_NULL != _clUniqueID && _su && _mbStat )
      {
         _su->increaseMBStat( _clUniqueID, _mbStat, count, _eduCB ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalRecords.add( count ) ;
         _mbStat->_rcTotalRecords.add( count ) ;
      }
   }

   void _dmsPersistGuard::decRecordCount( UINT64 count )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->decRecordCount( count ) ;
      }
      else if ( UTIL_UNIQUEID_NULL != _clUniqueID && _su && _mbStat )
      {
         _su->decreaseMBStat( _clUniqueID, _mbStat, count, _eduCB ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalRecords.sub( count ) ;
         _mbStat->_rcTotalRecords.sub( count ) ;
      }
   }

   void _dmsPersistGuard::incDataLen( UINT64 dataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incDataLen( dataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalDataLen.add( dataLen ) ;
      }
   }

   void _dmsPersistGuard::decDataLen( UINT64 dataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incDataLen( dataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalDataLen.sub( dataLen ) ;
      }
   }

   void _dmsPersistGuard::incOrgDataLen( UINT64 orgDataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incOrgDataLen( orgDataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalOrgDataLen.add( orgDataLen ) ;
      }
   }

   void _dmsPersistGuard::decOrgDataLen( UINT64 orgDataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incOrgDataLen( orgDataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalOrgDataLen.sub( orgDataLen ) ;
      }
   }

   /*
      _dmsWriteGuard implement
    */
   _dmsWriteGuard::_dmsWriteGuard( IStorageService *service,
                                   dmsStorageDataCommon *su,
                                   dmsMBContext *mbContext,
                                   pmdEDUCB *cb,
                                   BOOLEAN isDataWriteGuardEnabled,
                                   BOOLEAN isIndexWriteGuardEnabled,
                                   BOOLEAN isPersistGuardEnabled )
   : _dataGuard( su, mbContext, cb, isDataWriteGuardEnabled ),
     _indexGuard( cb, isIndexWriteGuardEnabled ),
     _persistGuard( service, su, mbContext, cb, isPersistGuardEnabled)
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWRITEGUARD_BEGIN, "_dmsWriteGuard::begin" )
   INT32 _dmsWriteGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWRITEGUARD_BEGIN ) ;

      rc = _dataGuard.begin() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin data write guard, rc: %d", rc ) ;

      rc = _indexGuard.begin() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin index write guard, rc: %d", rc ) ;

      rc = _persistGuard.begin() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin persist guard, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWRITEGUARD_BEGIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWRITEGUARD_COMMIT, "_dmsWriteGuard::commit" )
   INT32 _dmsWriteGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWRITEGUARD_COMMIT ) ;

      rc = _persistGuard.commit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit persist guard, rc: %d", rc ) ;

      rc = _indexGuard.commit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit index write guard, rc: %d", rc ) ;

      rc = _dataGuard.commit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit data write guard, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWRITEGUARD_COMMIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWRITEGUARD_ABORT, "_dmsWriteGuard::abort" )
   INT32 _dmsWriteGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWRITEGUARD_COMMIT ) ;

      INT32 tmpRC = SDB_OK ;

      tmpRC = _persistGuard.abort( isForced ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDWARNING, "Failed to abort persist guard, rc: %d", tmpRC ) ;
         if ( SDB_OK == rc )
         {
            rc = tmpRC ;
         }
      }

      tmpRC = _indexGuard.abort( isForced ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDWARNING, "Failed to abort index write guard, rc: %d", tmpRC ) ;
         if ( SDB_OK == rc )
         {
            rc = tmpRC ;
         }
      }

      tmpRC = _dataGuard.abort( isForced ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDWARNING, "Failed to abort data write guard, rc: %d", tmpRC ) ;
         if ( SDB_OK == rc )
         {
            rc = tmpRC ;
         }
      }

      PD_TRACE_EXITRC( SDB__DMSWRITEGUARD_COMMIT, rc ) ;

      return rc ;
   }

}
