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

   Source File Name = rtnLogInfoFilter.hpp

   Descriptive Name = Log Info Filter

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOG_INFO_FILTER_HPP__
#define RTN_LOG_INFO_FILTER_HPP__

#include "dpsLogDef.hpp"
#include "utilChangeStreamOptions.hpp"
#include "utilChangeStreamWatchInfo.hpp"
#include "utilPooledObject.hpp"
#include "dpsLogWrapper.hpp"

namespace engine
{

   /*
      _rtnLogInfoFilter define
    */
   // filter by log information
   class _rtnLogInfoFilter : public _utilPooledObject
   {
   public:
      _rtnLogInfoFilter()= default ;
      ~_rtnLogInfoFilter() = default ;

      // filter log information
      INT32 filter( utilWatchType watchLevel,
                    const utilChangeStreamLogInfo &logInfo,
                    BOOLEAN &isMatched ) ;
      // update filter by new watch information
      void updateFilter( utilWatchType watchLevel,
                         const utilChangeStreamWatchInfo &watchInfo ) ;
      // clear filter
      void clearFilter() ;

   protected:
      // bitmap for watched collection space by unique ID
      utilWatchCSBitmap _watchedCSBitmap ;
      // bitmap for watched log types
      utilLogTypeBitmap _watchedLogTypeBitmap ;
   } ;

   typedef class _rtnLogInfoFilter rtnLogInfoFilter ;

}

#endif // RTN_LOG_INFO_FILTER_HPP__
