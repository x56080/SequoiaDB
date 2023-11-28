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

   Source File Name = dmsWTDataCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTDataCursor.hpp"
#include "wiredtiger/dmsWTCollection.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCursor implement
    */
   _dmsWTDataCursor::_dmsWTDataCursor()
   : _session(),
     _cursor( _session )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_OPEN, "_dmsWTDataCursor::open" )
   INT32 _dmsWTDataCursor::open( ICollection *collection,
                                 const dmsRecordID &startRID,
                                 BOOLEAN afterStartRID,
                                 BOOLEAN isForward,
                                 IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_OPEN ) ;

      dmsWTCollection *wtCollection = dynamic_cast< dmsWTCollection * >( collection ) ;
      UINT64 key = startRID.toUINT64() ;

      PD_CHECK( wtCollection, SDB_SYS, error, PDERROR,
                "Failed to open cursor, collection is not WiredTiger collection" ) ;

      rc = wtCollection->getEngine()->openSession( _session ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      rc = _cursor.open( wtCollection->getDataStore().getURI(), "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

      if ( startRID.isValid() )
      {
         rc = _cursor.searchNext( key ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               _setEOF() ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to search next, rc: %d", rc ) ;
            }
            goto error ;
         }
      }
      else
      {
         rc = _cursor.next() ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               _setEOF() ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to move next, rc: %d", rc ) ;
            }
            goto error ;
         }
      }

      rc = _extractRecordID() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record ID, rc: %d", rc ) ;

      rc = _extractRecordData() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record data, rc: %d", rc ) ;

      _isOpened = TRUE ;
      _isForward = isForward ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_CLOSE, "_dmsWTDataCursor::close" )
   INT32 _dmsWTDataCursor::close()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_CLOSE ) ;

      rc = _cursor.close() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to close cursor, rc: %d", rc ) ;
      }
      _isClosed = TRUE ;

      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_CLOSE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_MOVENEXT, "_dmsWTDataCursor::moveNext" )
   INT32 _dmsWTDataCursor::moveNext( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_MOVENEXT ) ;

      if ( isEOF() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      PD_CHECK( isOpened(), SDB_SYS, error, PDERROR,
                "Failed to move next, cursor is not opened" ) ;
      PD_CHECK( !isClosed(), SDB_SYS, error, PDERROR,
                "Failed to move next, cursor is not opened" ) ;

      rc = _cursor.next() ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _setEOF() ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to move next, rc: %d", rc ) ;
         }
         goto error ;
      }

      rc = _extractRecordID() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record ID, rc: %d", rc ) ;

      rc = _extractRecordData() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record data, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_MOVENEXT, rc ) ;
      return rc ;

   error:
      close() ;
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_MOVEPREV, "_dmsWTDataCursor::movePrev" )
   INT32 _dmsWTDataCursor::movePrev( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_MOVEPREV ) ;

      if ( isEOF() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      PD_CHECK( isOpened(), SDB_SYS, error, PDERROR,
                "Failed to move prev, cursor is not opened" ) ;
      PD_CHECK( !isClosed(), SDB_SYS, error, PDERROR,
                "Failed to move prev, cursor is not opened" ) ;

      rc = _cursor.prev() ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            _setEOF() ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to move prev, rc: %d", rc ) ;
         }
         goto error ;
      }

      rc = _extractRecordID() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record ID, rc: %d", rc ) ;

      rc = _extractRecordData() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record data, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_MOVEPREV, rc ) ;
      return rc ;

   error:
      close() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR__EXTRACTRECID, "_dmsWTDataCursor::_extractRecordID" )
   INT32 _dmsWTDataCursor::_extractRecordID()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR__EXTRACTRECID) ;

      UINT64 key = 0 ;

      rc = _cursor.getKey( key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

      _setCurrentRecordID( _dmsRecordID( key ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR__EXTRACTRECID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR__EXTRACTRECDATA, "_dmsWTDataCursor::_extractRecordData" )
   INT32 _dmsWTDataCursor::_extractRecordData()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR__EXTRACTRECDATA ) ;

      dmsWTItem value ;

      rc = _cursor.getValue( value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

      _setCurrentRecord(
         dmsRecordData( (const CHAR *)( value.get()->data ),
                        (UINT32)( value.get()->size ) ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR__EXTRACTRECDATA, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
