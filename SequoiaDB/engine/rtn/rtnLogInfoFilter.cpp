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

   Source File Name = rtnLogInfoFilter.cpp

   Descriptive Name = Log Info Filter

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLogInfoFilter.hpp"
#include "ossUtil.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "utilUniqueID.hpp"

using namespace std ;

namespace engine
{

   /*
      _rtnLogInfoFilter implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGINFOFILTER_FILTER, "_rtnLogInfoFilter::filter" )
   INT32 _rtnLogInfoFilter::filter( utilWatchType watchLevel,
                                    const utilChangeStreamLogInfo &logInfo,
                                    BOOLEAN &isMatched )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGINFOFILTER_FILTER ) ;

      // match filter in below cases:
      // - log is for watched collection space
      //    - watch level is database
      //    - or log is for unknown collection space
      //    - or log is for watched collection space ( in bitmap )
      // - and log type is watched
      isMatched = ( ( ( UTIL_WATCH_ALL == watchLevel ) ||
                      ( UTIL_UNIQUEID_NULL == logInfo.getCSUID() ) ||
                      ( _watchedCSBitmap.isWatching( logInfo.getCSUID() ) ) ) &&
                    ( _watchedLogTypeBitmap.testBit( logInfo.getLogType() ) ) ) ;

      PD_TRACE_EXITRC( SDB__RTNLOGINFOFILTER_FILTER, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGINFOFILTER_UPDATEFILTER, "_rtnLogInfoFilter::updateFilter" )
   void _rtnLogInfoFilter::updateFilter( utilWatchType watchLevel,
                                         const utilChangeStreamWatchInfo &watchInfo )
   {
      PD_TRACE_ENTRY( SDB__RTNLOGINFOFILTER_UPDATEFILTER ) ;

      // update collection space unique ID filter
      if ( UTIL_WATCH_ALL != watchLevel )
      {
         _watchedCSBitmap.unionBitmap( watchInfo.getWatchedCSBitmap() ) ;
      }
      // update log type filter
      _watchedLogTypeBitmap.unionBitmap( watchInfo.getWatchedLogTypeBitmap() ) ;
      _watchedLogTypeBitmap.unionBitmap( watchInfo.getErrorLogTypeBitmap() ) ;

      PD_TRACE_EXIT( SDB__RTNLOGINFOFILTER_UPDATEFILTER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGINFOFILTER_CLEARFILTER, "_rtnLogInfoFilter::clearFilter" )
   void _rtnLogInfoFilter::clearFilter()
   {
      PD_TRACE_ENTRY( SDB__RTNLOGINFOFILTER_CLEARFILTER ) ;

      _watchedCSBitmap.resetBitmap() ;
      _watchedLogTypeBitmap.resetBitmap() ;

      PD_TRACE_EXIT( SDB__RTNLOGINFOFILTER_CLEARFILTER ) ;
   }

}
