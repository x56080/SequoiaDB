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

   Source File Name = dmsWTCollection.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTCollection.hpp"
#include "wiredtiger/dmsWTDataCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCollection implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_ALLOCRECID, "_dmsWTCollection::allocRecordID" )
   INT32 _dmsWTCollection::allocRecordID( UINT32 length, dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_ALLOCRECID ) ;

      // TODO: no need to be aligned in WT
      UINT64 tmpRID = _metadata.getMBStat()->_ridGen.add( ossAlignX( length, 4 ) ) ;
      rid._extent = (dmsExtentID)( tmpRID >> 32 ) ;
      rid._offset = (dmsOffset)( tmpRID & 0xFFFFFFFF ) ;

      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_ALLOCRECID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_INSERTREC, "_dmsWTCollection::insertRecord" )
   INT32 _dmsWTCollection::insertRecord( const dmsRecordID &rid,
                                         const dmsRecordData &recordData,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_INSERTREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value( recordData.data(), recordData.len() ) ;

      rc = _engine->insertToStore( _dataStore, key, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert record to engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_INSERTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_UPDATEREC, "_dmsWTCollection::updateRecord" )
   INT32 _dmsWTCollection::updateRecord( const dmsRecordID &rid,
                                         const dmsRecordData &recordData,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_UPDATEREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value( recordData.data(), recordData.len() ) ;

      rc = _engine->updateToStore( _dataStore, key, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update record to engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_UPDATEREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_RMREC, "_dmsWTCollection::removeRecord" )
   INT32 _dmsWTCollection::removeRecord( const dmsRecordID &rid,
                                         IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_RMREC ) ;

      UINT64 key = rid.toUINT64() ;

      rc = _engine->removeFromStore( _dataStore, key ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove record from engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_RMREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_EXTRACTREC, "_dmsWTCollection::extractRecord" )
   INT32 _dmsWTCollection::extractRecord( const dmsRecordID &rid,
                                          dmsRecordData &recordData,
                                          IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_EXTRACTREC ) ;

      UINT64 key = rid.toUINT64() ;
      dmsWTItem value ;
      rc = _engine->extractFromStore( _dataStore, key, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record from engine, rc: %d", rc ) ;

      recordData.setData( (const CHAR *)( value.get()->data ),
                          value.get()->size,
                          UTIL_COMPRESSOR_INVALID,
                          TRUE ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_EXTRACTREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCOLLECTION_CREATEDATACURSOR, "_dmsWTCollection::createDataCursor" )
   INT32 _dmsWTCollection::createDataCursor( std::unique_ptr< IDataCursor > &cursor,
                                             const dmsRecordID &startRID,
                                             BOOLEAN afterStartRID,
                                             BOOLEAN isForward,
                                             IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCOLLECTION_CREATEDATACURSOR ) ;

      cursor = std::unique_ptr< dmsWTDataCursor >( new dmsWTDataCursor() ) ;
      PD_CHECK( cursor, SDB_OOM, error, PDERROR, "Failed to create data cursor, rc: %d", rc ) ;

      rc = cursor->open( this, startRID, afterStartRID, isForward, executor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open data cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCOLLECTION_CREATEDATACURSOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
