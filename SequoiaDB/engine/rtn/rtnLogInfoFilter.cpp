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
