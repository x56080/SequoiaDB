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

using namespace std ;

namespace engine
{

   /*
      _dmsIndexWriteGaurd imeplement
    */
   _dmsIndexWriteGuard::_dmsIndexWriteGuard( pmdEDUCB *cb, BOOLEAN isEnabled )
   : _eduCB( cb ),
     _isEnabled( isEnabled )
   {
   }

   _dmsIndexWriteGuard::~_dmsIndexWriteGuard()
   {
      releaseAll() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_LOCK, "_dmsIndexWriteGuard::lock" )
   INT32 _dmsIndexWriteGuard::lock( const dmsIdxMetadataKey &metadataKey,
                                    shared_ptr<ossRWMutex> &lockPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_LOCK ) ;

      // already locked
      if ( _locks.count( metadataKey ) > 0 )
      {
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

}