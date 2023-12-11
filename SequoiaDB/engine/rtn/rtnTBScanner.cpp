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

   Source File Name = rtnTBScanner.cpp

   Descriptive Name = RunTime Table Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnTBScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _rtnTBScanner define
    */
   _rtnTBScanner::_rtnTBScanner( dmsStorageUnit *su,
                                 dmsMBContext *mbContext,
                                 const dmsRecordID &startRID,
                                 BOOLEAN isAfterStartRID,
                                 INT32 direction,
                                 pmdEDUCB *cb )
   : _rtnScanner( su, mbContext, direction, cb ),
     _startRID( startRID ),
     _isAfterStartRID( isAfterStartRID )
   {
   }

   _rtnTBScanner::~_rtnTBScanner()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTBSCAN_ADVANCE, "_rtnTBScanner::advance" )
   INT32 _rtnTBScanner::advance( dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNTBSCAN_ADVANCE ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      if ( !_cursorPtr )
      {
         rc = _mbContext->getCollPtr()->
                     createDataCursor( _cursorPtr, _startRID, _isAfterStartRID, _direction > 0 ? TRUE : FALSE, _cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to create data cursor on "
                     "collection [%s.%s], rc: %d", _su->getSUName(),
                     _mbContext->clName(), rc ) ;
      }
      else
      {
         rc = _cursorPtr->advance( _cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
      }

      rc = getCurrentRID( rid ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get currnent record ID, rc: %d", rc ) ;

   done:
      if ( SDB_DMS_EOC == rc && !_isEOF )
      {
         _isEOF = TRUE ;
         goto done ;
      }
      PD_TRACE_EXITRC( SDB__RTNTBSCAN_ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTBSCAN_GETCURRID, "_rtnTBScanner::getCurrentRID" )
   INT32 _rtnTBScanner::getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNTBSCAN_GETCURRID ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      PD_CHECK( _cursorPtr, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get record ID, cursor is clsoed" ) ;

      rc = _cursorPtr->getCurrentRecordID( nextRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record ID, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNTBSCAN_GETCURRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNTBSCAN_GETCURREC, "_rtnTBScanner::getCurrentRecord" )
   INT32 _rtnTBScanner::getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNTBSCAN_GETCURREC ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      PD_CHECK( _cursorPtr, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get record, cursor is clsoed" ) ;

      rc = _cursorPtr->getCurrentRecord( recordData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNTBSCAN_GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
