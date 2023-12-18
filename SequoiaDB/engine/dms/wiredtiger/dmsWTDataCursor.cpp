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
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

namespace
{
   static dmsRecordID s_minRID = dmsRecordID::minRID() ;
   static dmsRecordID s_maxRID = dmsRecordID::maxRID() ;
}

   /*
      _dmsWTDataCursor implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_OPEN, "_dmsWTDataCursor::open" )
   INT32 _dmsWTDataCursor::open( shared_ptr<ICollection> collPtr,
                                 const dmsRecordID &startRID,
                                 BOOLEAN isAfterStartRID,
                                 BOOLEAN isForward,
                                 IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_OPEN ) ;

      dmsWTCollection *wtCollection = dynamic_cast<dmsWTCollection *>( collPtr.get() ) ;
      UINT64 key = startRID.toUINT64() ;

      PD_CHECK( wtCollection, SDB_SYS, error, PDERROR,
                "Failed to open cursor, collection is not WiredTiger collection" ) ;

      if ( startRID.isValid() )
      {
         // fix record ID key here, avoid move call inside WiredTiger
         if ( isAfterStartRID )
         {
            if ( isForward )
            {
               if ( s_maxRID == startRID )
               {
                  rc = SDB_DMS_EOC ;
                  _isEOF = TRUE ;
                  goto error ;
               }
               ++ key ;
            }
            else if ( !isForward )
            {
               if ( s_minRID == startRID )
               {
                  rc = SDB_DMS_EOC ;
                  _isEOF = TRUE ;
                  goto error ;
               }
               -- key ;
            }
         }

         rc = _open( wtCollection->getEngine(), wtCollection->getStore().getURI(),
                     "", dmsWTSessIsolation::READ_COMMITTED, key, FALSE,
                     isForward, executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      }
      else
      {
         rc = _open( wtCollection->getEngine(), wtCollection->getStore().getURI(),
                     "", dmsWTSessIsolation::READ_COMMITTED, isForward, executor ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;
      }

      _collPtr = std::move( collPtr ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_OPEN, rc ) ;
      return rc ;

   error:
      close() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_GETCURRECID, "_dmsWTDataCursor::getCurrentRecordID" )
   INT32 _dmsWTDataCursor::getCurrentRecordID( dmsRecordID &recordID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_GETCURRECID ) ;

      UINT64 key = 0 ;

      PD_CHECK( !isEOF(), SDB_DMS_EOC, error, PDERROR,
                "Failed to get current record ID, cursor is hit end" ) ;
      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;

      rc = _cursor.getKey( key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

      recordID.fromUINT64( key ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_GETCURRECID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTDATACURSOR_GETCURREC, "_dmsWTDataCursor::getCurrentRecord" )
   INT32 _dmsWTDataCursor::getCurrentRecord( dmsRecordData &data )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTDATACURSOR_GETCURREC ) ;

      dmsWTItem value ;

      PD_CHECK( !isEOF(), SDB_DMS_EOC, error, PDERROR,
                "Failed to get current record ID, cursor is hit end" ) ;
      PD_CHECK( isOpened() && !isClosed(), SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get current record ID, cursor is not opened" ) ;

      rc = _cursor.getValue( value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

      data.setData( (const CHAR *)( value.get()->data ), (UINT32)( value.get()->size ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTDATACURSOR_GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
