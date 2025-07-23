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
