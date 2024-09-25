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

   Source File Name = dmsIndexChangeWatcher.hpp

   Descriptive Name = Building index concurrently change watcher

   When/how to use: If index is being built while update / delete change
   the record, use this class to record and communicate the changed.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/12/2024  LSQ Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsIndexChangeWatcher.hpp"

namespace engine
{

   /*
      _dmsIndexChangeWatcher implement
   */
   INT32 _dmsIndexChangeWatcher::setWatchWindow( OID indexOID,
                                                 dmsExtentID startExtLID,
                                                 dmsExtentID endExtID )
   {
      INT32 rc = SDB_OK ;
      try
      {
         ossScopedLock lock( &_latch ) ;
         _WatchWindow window = { startExtLID, endExtID };
         _windowMap[ indexOID ] = window ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
      }
      return rc ;
   }

   INT32 _dmsIndexChangeWatcher::reset( OID indexOID )
   {
      INT32 rc = SDB_OK ;
      try
      {
         ossScopedLock lock( &_latch ) ;

         dmsRecordID minRid ;
         minRid.resetMin() ;
         OID_RID_PAIR lowBound( indexOID, minRid ) ;
         OID_RID_SET::iterator minIt = _idSet.lower_bound( lowBound ) ;

         dmsRecordID maxRid ;
         maxRid.resetMax() ;
         OID_RID_PAIR upBound( indexOID, maxRid ) ;
         OID_RID_SET::iterator maxIt = _idSet.upper_bound( upBound ) ;

         _idSet.erase( minIt, maxIt ) ;
         _windowMap.erase( indexOID ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
      }
      return rc ;
   }

   INT32 _dmsIndexChangeWatcher::notifyRecordChanged( ixmIndexCB *indexCB,
                                                      dmsExtentID extLID,
                                                      dmsRecordID rid )
   {
      INT32 rc = SDB_OK ;
      try
      {
         ossScopedLock lock( &_latch ) ;

         OID oid ;
         WINDOW_MAP::iterator it ;

         rc = indexCB->getIndexID( oid ) ;
         if ( rc != SDB_OK )
         {
            goto error ;
         }

         it = _windowMap.find( oid ) ;
         if ( it != _windowMap.end() )
         {
            _WatchWindow &window = it->second ;
            if ( window._start <= extLID && extLID <= window._end )
            {
               OID_RID_PAIR value( oid, rid ) ;
               _idSet.insert( value ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _dmsIndexChangeWatcher::isRecordChanged( OID indexOID,
                                                    dmsRecordID rid )
   {
      ossScopedLock lock( &_latch ) ;
      OID_RID_PAIR value( indexOID, rid ) ;
      OID_RID_SET::iterator it = _idSet.find( value ) ;
      return ( it != _idSet.end() ) ;
   }
}
