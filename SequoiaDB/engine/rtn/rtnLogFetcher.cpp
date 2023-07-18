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

   Source File Name = rtnLogFetcher.cpp

   Descriptive Name = Log Fetcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLogFetcher.hpp"
#include "dpsDef.hpp"
#include "ossErr.h"
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdEDU.hpp"
#include "rtnTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _rtnLogFetcher implement
    */
   _rtnLogFetcher::_rtnLogFetcher()
   : _logAccessor( pmdGetKRCB()->getDPSCB() )
   {
      SDB_ASSERT( NULL != _logAccessor, "should have log accessor" ) ;
      _dummyHeader.clear() ;
      _dummyHeader._type = LOG_TYPE_DUMMY ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGFETCHER_FETCHLOG, "_rtnLogFetcher::fetchLog" )
   INT32 _rtnLogFetcher::fetchLog( const DPS_LSN &lsn,
                                   INT64 timeout,
                                   dpsMessageBlock &mb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGFETCHER_FETCHLOG ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      INT64 waitTime = 0 ;
      while ( TRUE )
      {
         PD_CHECK( !cb->isInterrupted(), cb->getInterruptRC(), error, PDWARNING,
                  "Failed to fetch log, session is interrupted" ) ;
         PD_CHECK( !PMD_IS_DB_DOWN(), SDB_APP_FORCED, error, PDWARNING,
                  "Failed to fetch log, database is down" ) ;

         mb.clear() ;
         rc = _logAccessor->search( lsn, &mb ) ;
         if ( SDB_DPS_LSN_OUTOFRANGE == rc )
         {
            if ( timeout < 0 || waitTime < timeout )
            {
               ossSleep( OSS_ONE_SEC ) ;
               waitTime += OSS_ONE_SEC ;
               rc = SDB_OK ;
               continue ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to search log [offset %llu], rc: %d",
                      lsn.offset, rc ) ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGFETCHER_FETCHLOG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGFETCHER_FETCHDUMMYLOG, "_rtnLogFetcher::fetchDummyLog" )
   INT32 _rtnLogFetcher::fetchDummyLog( const DPS_LSN &lsn,
                                        UINT32 length,
                                        const dpsLogRecordHeader *&dummyRecord )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGFETCHER_FETCHDUMMYLOG ) ;

      _dummyHeader._lsn = lsn.offset ;
      _dummyHeader._version = lsn.version ;
      _dummyHeader._length = length ;

      dummyRecord = &_dummyHeader ;

      PD_TRACE_EXITRC( SDB__RTNLOGFETCHER_FETCHDUMMYLOG, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNLOGFETCHER_FETCHLOGS, "_rtnLogFetcher::fetchLogs" )
   INT32 _rtnLogFetcher::fetchLogs( const DPS_LSN &lsn,
                                    INT32 maxFetchSize,
                                    INT64 timeout,
                                    dpsMessageBlock &mb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNLOGFETCHER_FETCHLOGS ) ;

      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      INT64 waitTime = 0 ;
      while ( TRUE )
      {
         PD_CHECK( !cb->isInterrupted(), cb->getInterruptRC(), error, PDWARNING,
                   "Failed to fetch log, session is interrupted" ) ;
         PD_CHECK( !PMD_IS_DB_DOWN(), SDB_APP_FORCED, error, PDWARNING,
                   "Failed to fetch log, database is down" ) ;

         mb.clear() ;
         rc = _logAccessor->search( lsn, &mb, DPS_SEARCH_ALL, -1, -1, maxFetchSize ) ;
         if ( SDB_DPS_LSN_OUTOFRANGE == rc )
         {
            if ( timeout < 0 || waitTime < timeout )
            {
               ossSleep( OSS_ONE_SEC ) ;
               waitTime += OSS_ONE_SEC ;
               rc = SDB_OK ;
               continue ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to search log [offset %llu], rc: %d",
                     lsn.offset, rc ) ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNLOGFETCHER_FETCHLOGS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
