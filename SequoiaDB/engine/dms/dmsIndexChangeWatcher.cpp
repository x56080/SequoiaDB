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
