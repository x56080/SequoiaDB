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

   Source File Name = dpsTransArbit.cpp

   Descriptive Name = DPS Global Transaction Arbitration

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains implement for arbitration of
   global transactions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/20/2020  HGM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsTransArbit.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "dpsUtil.hpp"

using namespace std ;

namespace engine
{

   /*
      _dpsTransArbit implement
    */
   _dpsTransArbit::_dpsTransArbit()
   {
   }

   _dpsTransArbit::~_dpsTransArbit()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSTRANSARBIT_ARBIT, "_dpsTransArbit::arbit" )
   INT32 _dpsTransArbit::arbit( const DPS_TRANS_ID &writeTransID,
                                DPS_TRANS_STATUS writeTransStatus,
                                BOOLEAN &visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSTRANSARBIT_ARBIT ) ;

      DPS_TRANS_STATUS curStatus = DPS_TRANS_UNKNOWN ;
      BOOLEAN curVisible = FALSE ;

      visible = FALSE ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;

      // find if we have done arbitration for this transaction
      if ( _findRecord( writeTransID, curStatus, curVisible ) )
      {
         // if found, use arbitration history
         visible = curVisible ;
         goto done ;
      }

      // not found, do the arbitration, only committed transaction could be
      // visible to current transaction
      curVisible = ( DPS_TRANS_COMMIT == writeTransStatus ) ;

      // save record for arbitration with the same write transaction ID in
      // next time or from other groups
      rc = _saveRecord( writeTransID, writeTransStatus, curVisible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save arbitrate record for "
                   "transaction [%s], rc: %d",
                   dpsTransIDToString( writeTransID ).c_str(), rc ) ;

      visible = curVisible ;

   done:
      PD_TRACE_EXITRC( SDB__DPSTRANSARBIT_ARBIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSTRANSARBIT_FINDARBIT, "_dpsTransArbit::findArbit" )
   BOOLEAN _dpsTransArbit::findArbit( const DPS_TRANS_ID &transID,
                                      BOOLEAN &visible )
   {
      BOOLEAN found = FALSE ;

      PD_TRACE_ENTRY( SDB__DPSTRANSARBIT_FINDARBIT ) ;

      DPS_TRANS_STATUS status = DPS_TRANS_UNKNOWN ;

      ossScopedRWLock lock( &_mutex, SHARED ) ;
      found = _findRecord( transID, status, visible ) ;

      PD_TRACE_EXIT( SDB__DPSTRANSARBIT_FINDARBIT ) ;

      return found ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSTRANSARBIT_SAVEARBIT, "_dpsTransArbit::saveArbit" )
   INT32 _dpsTransArbit::saveArbit( const DPS_TRANS_ID &transID,
                                    DPS_TRANS_STATUS status,
                                    BOOLEAN visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSTRANSARBIT_SAVEARBIT ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
      rc = _saveRecord( transID, status, visible ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save arbitrate record, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DPSTRANSARBIT_SAVEARBIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSTRANSARBIT_CLEAR, "_dpsTransArbit::clear" )
   void _dpsTransArbit::clear()
   {
      PD_TRACE_ENTRY( SDB__DPSTRANSARBIT_CLEAR ) ;

      ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
      _records.clear() ;

      PD_TRACE_EXIT( SDB__DPSTRANSARBIT_CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSTRANSARBIT__FINDREC, "_dpsTransArbit::_findRecord" )
   BOOLEAN _dpsTransArbit::_findRecord( const DPS_TRANS_ID &transID,
                                        DPS_TRANS_STATUS &status,
                                        BOOLEAN &visible )
   {
      BOOLEAN found = FALSE ;

      PD_TRACE_ENTRY( SDB__DPSTRANSARBIT__FINDREC ) ;

      DPS_ARBIT_RECORD_MAP_IT iter = _records.find( transID ) ;
      if ( iter != _records.end() )
      {
         status = iter->second.getStatus() ;
         visible = iter->second.isVisible() ;
         found = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DPSTRANSARBIT__FINDREC ) ;

      return found ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSTRANSARBIT__SAVEREC, "_dpsTransArbit::_saveRecord" )
   INT32 _dpsTransArbit::_saveRecord( const DPS_TRANS_ID &transID,
                                      DPS_TRANS_STATUS status,
                                      BOOLEAN visible )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DPSTRANSARBIT__FINDREC ) ;

      try
      {
         dpsTransArbitRecord record( status, visible ) ;
         _records[ transID ] = record ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save arbitrate record, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSTRANSARBIT__FINDREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
