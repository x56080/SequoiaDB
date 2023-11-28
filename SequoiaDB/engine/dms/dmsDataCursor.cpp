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

   Source File Name = dmsDataCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsDataCursor.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      _dmsDataCursor implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATACURSOR_GETCURRECID, "_dmsDataCursor::getCurrentRecordID" )
   INT32 _dmsDataCursor::getCurrentRecordID( dmsRecordID &recordID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATACURSOR_GETCURRECID ) ;

      PD_CHECK( isOpened(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;
      PD_CHECK( !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;
      PD_CHECK( !isEOF(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;

      recordID = _curentRecordID ;

   done:
      PD_TRACE_EXITRC( SDB__DMSDATACURSOR_GETCURRECID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATACURSOR_GETCURREC, "_dmsDataCursor::getCurrentRecord" )
   INT32 _dmsDataCursor::getCurrentRecord( dmsRecordData &data )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATACURSOR_GETCURREC ) ;

      PD_CHECK( isOpened(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;
      PD_CHECK( !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;
      PD_CHECK( !isEOF(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;

      data = _currentRecordData ;

   done:
      PD_TRACE_EXITRC( SDB__DMSDATACURSOR_GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}