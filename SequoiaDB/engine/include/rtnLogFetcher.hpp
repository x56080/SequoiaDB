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

   Source File Name = rtnLogFetcher.hpp

   Descriptive Name = Log Fetcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOG_FETCHER_HPP__
#define RTN_LOG_FETCHER_HPP__

#include "dpsLogDef.hpp"
#include "dpsLogRecord.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsMessageBlock.hpp"
#include "ossRWMutex.hpp"
#include "rtnContext.hpp"
#include "rtnLogRecordFilter.hpp"
#include "rtnStreamSource.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _rtnLogFetcher define
    */
   // fetcher for log record
   class _rtnLogFetcher : public _utilPooledObject
   {
   public:
      _rtnLogFetcher() ;
      ~_rtnLogFetcher() = default ;

      // fetch a log record
      INT32 fetchLog( const DPS_LSN &lsn,
                      INT64 timeout,
                      dpsMessageBlock &mb ) ;
      // fetch a dummy record
      INT32 fetchDummyLog( const DPS_LSN &lsn,
                           UINT32 length,
                           const dpsLogRecordHeader *&dummyRecord ) ;

      // fetch a batch of logs
      INT32 fetchLogs( const DPS_LSN &lsn,
                       INT32 maxFetchSize,
                       INT64 timeout,
                       dpsMessageBlock &mb ) ;

   protected:
      // pointer to log accessor
      ILogAccessor *    _logAccessor ;
      // dummy log record
      dpsLogRecordHeader _dummyHeader ;
   } ;

   typedef class _rtnLogFetcher rtnLogFetcher ;

}

#endif // RTN_LOG_FETCHER_HPP__
