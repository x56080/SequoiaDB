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

   Source File Name = utilLocation.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2022/12/20  HYQ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_LOCATION_HPP_
#define UTIL_LOCATION_HPP_

#include "core.hpp"

namespace engine
{
   BOOLEAN utilCalAffinity( const CHAR *location1, const CHAR *location2 ) ;

   struct _utilLocationInfo
   {
      UINT8 primaryLocationNodes ;
      UINT8 locations ;
      UINT8 affinitiveLocations ;
      UINT8 affinitiveNodes ;

      _utilLocationInfo()
      {
         primaryLocationNodes = 0 ;
         locations = 0 ;
         affinitiveLocations = 0 ;
         affinitiveNodes = 0 ;
      }
   } ;

   typedef _utilLocationInfo utilLocationInfo ;
}

#endif // UTIL_LOCATION_HPP_