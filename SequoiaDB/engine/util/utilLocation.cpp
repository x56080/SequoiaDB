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

   Source File Name = utilLocation.cpp

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
#include "utilLocation.hpp"
#include "ossUtil.hpp"

namespace engine
{
   BOOLEAN utilCalAffinity( const CHAR *location1, const CHAR *location2 )
   {
      BOOLEAN ret = FALSE ;
      const CHAR *pos1 = NULL, *pos2 = NULL ;
      UINT32 prefixLen1 = 0, prefixLen2 = 0 ;
      UINT32 locationLen1 = 0, locationLen2 = 0 ;

      if ( NULL == location1 || 0 == location1[0] ||
           NULL == location2 || 0 == location2[0] )
      {
         goto done ;
      }

      locationLen1 = ossStrlen( location1 ) ;
      locationLen2 = ossStrlen( location2 ) ;
      pos1 = ossStrchr( location1, '.' ) ;
      pos2 = ossStrchr( location2, '.' ) ;
      prefixLen1 = NULL != pos1 ? pos1 - location1 : 0 ;
      prefixLen2 = NULL != pos2 ? pos2 - location2 : 0 ;

      // e.g. "A" and "A"
      if ( NULL == pos1 && NULL == pos2 &&
           locationLen1 == locationLen2 &&
           0 == ossStrncasecmp( location1, location2, locationLen1 ) )
      {
         ret = TRUE ;
      }
      // e.g. "A" and "A.a"
      else if ( ( NULL == pos1 && prefixLen2 == locationLen1 &&
                  0 == ossStrncasecmp( location1, location2, prefixLen2 ) ) ||
                ( NULL == pos2 && prefixLen1 == locationLen2 &&
                  0 == ossStrncasecmp( location2, location1, prefixLen1 ) ) )
      {
         ret = TRUE ;
      }
      // e.g. "A.a" and "A.b"
      else if ( NULL != pos1 && NULL != pos2 &&
                prefixLen1 == prefixLen2 &&
                0 != prefixLen1 &&
                0 == ossStrncasecmp( location1, location2, prefixLen1 ) )
      {
         ret = TRUE ;
      }

   done:
      return ret ;
   }
}
