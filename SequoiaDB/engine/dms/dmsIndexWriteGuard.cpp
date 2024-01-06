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

   Source File Name = dmsIndexWriteGuard.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsIndexWriteGuard.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageDataCommon.hpp"

using namespace std ;

namespace engine
{

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
                                    dmsIndexBuildGuardPtr &guardPtr,
                                    BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_LOCK ) ;

      BOOLEAN isLockedByThisRound = FALSE ;
      BOOLEAN hasRisk = FALSE ;

      dmsRIDIdxBuildGuardMapIter iter = _guards.find( metadataKey ) ;
      // already locked
      if ( iter != _guards.end() )
      {
         SDB_ASSERT( guardPtr == iter->second.first, "should be same guard" ) ;
         if ( iter->second.second.count( rid ) )
         {
            needProcess = TRUE ;
            goto done ;
         }
      }
      else
      {
         // wait lock
         while ( TRUE )
         {
            if ( _eduCB->isInterrupted() )
            {
               PD_RC_CHECK( rc, PDERROR, "Failed to lock index write guard, "
                           "interrupted" ) ;
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
            rc = guardPtr->getProcessMutex().lock_r( OSS_ONE_SEC ) ;
            if ( SDB_OK == rc )
            {
               isLockedByThisRound = TRUE ;
               break ;
            }
            else if ( SDB_TIMEOUT == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to lock index write guard, rc: %d", rc ) ;
         }
      }

      needProcess = TRUE ;

      rc = guardPtr->writeCheck( rid, needProcess, hasRisk ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check write, rc: %d", rc ) ;

      if ( hasRisk )
      {
         try
         {
            if ( iter != _guards.end() )
            {
               iter->second.second.insert( rid ) ;
            }
            else
            {
               auto res = _guards.insert(
                           make_pair( metadataKey,
                                      make_pair( guardPtr,
                                                 ossPoolSet<dmsRecordID>() ) ) ) ;
               res.first->second.second.insert( rid ) ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to save index build guard, "
                    "occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
         isLockedByThisRound = FALSE ;
      }

   done:
      if ( isLockedByThisRound )
      {
         guardPtr->getProcessMutex().release_r() ;
      }
      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_LOCK, rc ) ;
      return rc ;

   error:
      goto done ;
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

      for ( dmsRIDIdxBuildGuardMapIter iter = _guards.begin() ;
            iter != _guards.end() ;
            ++ iter )
      {
         dmsIndexBuildGuardPtr &guardPtr = iter->second.first ;
         ossPoolSet<dmsRecordID> &ridSet = iter->second.second ;
         for ( auto rid : ridSet )
         {
            guardPtr->writeCommit( rid, TRUE ) ;
         }
         iter->second.first->getProcessMutex().release_r() ;
      }
      _guards.clear() ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_COMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_ABORT, "_dmsIndexWriteGuard::abort" )
   INT32 _dmsIndexWriteGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_ABORT ) ;

      for ( dmsRIDIdxBuildGuardMapIter iter = _guards.begin() ;
            iter != _guards.end() ;
            ++ iter )
      {
         dmsIndexBuildGuardPtr &guardPtr = iter->second.first ;
         ossPoolSet<dmsRecordID> &ridSet = iter->second.second ;
         for ( auto rid : ridSet )
         {
            guardPtr->writeAbort( rid, TRUE ) ;
         }
         iter->second.first->getProcessMutex().release_r() ;
      }
      _guards.clear() ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_ABORT, rc ) ;

      return rc ;
   }

}
