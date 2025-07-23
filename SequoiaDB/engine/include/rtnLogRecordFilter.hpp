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

   Source File Name = rtnLogRecordFilter.hpp

   Descriptive Name = Log Record Filter

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOG_RECORD_FILTER_HPP__
#define RTN_LOG_RECORD_FILTER_HPP__

#include "dpsLogDef.hpp"
#include "dpsLogRecord.hpp"
#include "utilChangeStreamOptions.hpp"
#include "utilChangeStreamWatchInfo.hpp"
#include "utilPooledObject.hpp"
#include "dpsLogWrapper.hpp"
#include "utilBitmap.hpp"

namespace engine
{

   /*
      _rtnLogRecordFilter define
    */
   // filter by log record
   class _rtnLogRecordFilter : public _utilPooledObject
   {
   public:
      _rtnLogRecordFilter( utilChangeStreamWatchInfo &watchInfo ) ;
      ~_rtnLogRecordFilter() = default ;

      // filter by log information
      INT32 filterInfo( const utilChangeStreamLogInfo &logInfo,
                        BOOLEAN &isMatched,
                        BOOLEAN &needFilterRecord,
                        BOOLEAN &needCheckError,
                        BOOLEAN &needFilterType ) ;
      // filter by log record
      INT32 filterRecord( const dpsLogRecord &record,
                          BOOLEAN &isMatched ) ;

      // add transaction ID for filtering
      INT32 addTransID( const DPS_TRANS_ID &transID ) ;
      // remove transaction ID from filtering
      INT32 removeTransID( const DPS_TRANS_ID &transID ) ;

   protected:
      // watch information
      utilChangeStreamWatchInfo &_watchInfo ;
      // transaction set
      // only commit or rollback records' transaction ID in this set will
      // be matched
      DPS_TRANS_ID_SET _transSet ;
   } ;

   typedef class _rtnLogRecordFilter rtnLogRecordFilter ;

}

#endif // RTN_LOG_RECORD_FILTER_HPP__
