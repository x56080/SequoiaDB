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

   Source File Name = dmsPersistUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsPersistUnit.hpp"
#include "dmsDef.hpp"
#include "ossErr.h"
#include "ossMem.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
       _dmsPersistUnit implement
    */
   _dmsPersistUnit::_dmsPersistUnit()
   : _state( dmsPersistUnitState::INACTIVE ),
     _activeLevel( 0 )
   {
   }

   _dmsPersistUnit::~_dmsPersistUnit()
   {
      // should not call virtual function here, just assert
      SDB_ASSERT( dmsPersistUnitState::INACTIVE == _state, "should be inactive" ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTUNIT_BEGINUNIT, "_dmsPersistUnit::beginUnit" )
   INT32 _dmsPersistUnit::beginUnit( IExecutor *executor, BOOLEAN isTrans )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTUNIT_BEGINUNIT ) ;

      if ( isTrans && !_isTransSupported() )
      {
         goto done ;
      }
      if ( dmsPersistUnitState::ACTIVE == _state ||
           dmsPersistUnitState::ACTIVE_IN_TRANS == _state )
      {
         ++ _activeLevel ;
      }
      else if ( dmsPersistUnitState::PREPARED == _state )
      {
         PD_LOG( PDERROR, "Persist unit has been prepared" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         rc = _beginUnit( executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to begin persist unit, rc: %d", rc ) ;

         _state = isTrans ?
                  dmsPersistUnitState::ACTIVE_IN_TRANS :
                  dmsPersistUnitState::ACTIVE ;
         _activeLevel = 1 ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTUNIT_BEGINUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTUNIT_PREPAREUNIT, "_dmsPersistUnit::prepareUnit" )
   INT32 _dmsPersistUnit::prepareUnit( IExecutor *executor, BOOLEAN isTrans )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTUNIT_PREPAREUNIT ) ;

      if ( dmsPersistUnitState::INACTIVE == _state ||
           dmsPersistUnitState::PREPARED == _state )
      {
         goto done ;
      }
      else if ( _activeLevel > 1 )
      {
         goto done ;
      }

      rc = _prepareUnit( executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare persist unit, rc: %d", rc ) ;

      _state = dmsPersistUnitState::PREPARED ;

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTUNIT_PREPAREUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTUNIT_COMMITUNIT, "_dmsPersistUnit::commitUnit" )
   INT32 _dmsPersistUnit::commitUnit( IExecutor *executor, BOOLEAN isTrans )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTUNIT_COMMITUNIT ) ;

      if ( dmsPersistUnitState::INACTIVE == _state )
      {
         goto done ;
      }
      if ( !isTrans && _activeLevel > 1 )
      {
         -- _activeLevel ;
         goto done ;
      }

      rc = _commitUnit( executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit persist unit, rc: %d", rc ) ;

      _state = dmsPersistUnitState::INACTIVE ;
      _activeLevel = 0 ;

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTUNIT_COMMITUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTUNIT_ABORTUNIT, "_dmsPersistUnit::abortUnit" )
   INT32 _dmsPersistUnit::abortUnit( IExecutor *executor,
                                     BOOLEAN isTrans,
                                     BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTUNIT_ABORTUNIT ) ;

      if ( dmsPersistUnitState::INACTIVE == _state )
      {
         goto done ;
      }
      else if ( !isTrans && !isForced && _activeLevel > 1 )
      {
         -- _activeLevel ;
         goto done ;
      }

      rc = _abortUnit( executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to abort persist unit, rc: %d", rc ) ;

      _state = dmsPersistUnitState::INACTIVE ;
      _activeLevel = 0 ;

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTUNIT_ABORTUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
