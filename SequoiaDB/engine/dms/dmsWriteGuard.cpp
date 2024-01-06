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
