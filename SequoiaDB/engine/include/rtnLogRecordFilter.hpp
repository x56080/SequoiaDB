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
