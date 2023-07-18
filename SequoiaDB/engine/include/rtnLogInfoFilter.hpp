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
