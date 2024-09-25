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
#ifndef DMS_INDEX_CHANGE_WATCHER_HPP__
#define DMS_INDEX_CHANGE_WATCHER_HPP__

#include "dms.hpp"
#include "ixm.hpp"
#include "ossMemPool.hpp"

namespace engine
{
   /*
      Building index concurrently change watcher. If index is being built while
      update / delete change the record, use this class to record and
      communicate the changed.

      Extents have 3 status:

      |-----built------|-----building-----|-----not built-----|

      1. built: extents were scan and keys were inserted
      2. building: extents were scan into buffer, and part of keys may be
                   inserted. This window is what we are watching.
      3. not built: extents have not been scan yet.

      If the records at building extents was changed, they may be dirty read by
      building session. The update / delete session should notify the building
      session to handle the conflict.
   */
   class _dmsIndexChangeWatcher: public SDBObject
   {
      public:
         INT32 reset( OID indexOID ) ;

         INT32 setWatchWindow( OID indexOID,
                               dmsExtentID startExtLID,
                               dmsExtentID endExtID ) ;

         INT32 notifyRecordChanged( ixmIndexCB *indexCB,
                                    dmsExtentID extLID,
                                    dmsRecordID rid ) ;

         BOOLEAN isRecordChanged( OID indexOID, dmsRecordID rid ) ;

      private:
         struct _WatchWindow
         {
            dmsExtentID _start ;
            dmsExtentID _end ;
         } ;
         typedef ossPoolMap< OID, _WatchWindow > WINDOW_MAP ;

         typedef std::pair< OID, dmsRecordID > OID_RID_PAIR ;
         class _OidRidPairCmp
         {
         public:
            bool operator()( const OID_RID_PAIR l, const OID_RID_PAIR r )
            {
               return ( l.first < r.first ||
                        ( l.first == r.first && l.second < r.second ) ) ;
            }
         } ;
         typedef ossPoolSet< OID_RID_PAIR, _OidRidPairCmp > OID_RID_SET ;

      private:
         ossSpinXLatch  _latch ;
         WINDOW_MAP     _windowMap ;
         OID_RID_SET    _idSet ;
   } ;
   typedef _dmsIndexChangeWatcher dmsIndexChangeWatcher ;
}

#endif /* DMS_INDEX_CHANGE_WATCHER_HPP__ */