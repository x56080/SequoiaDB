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
