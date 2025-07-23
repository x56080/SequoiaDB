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

   Source File Name = pdSecure.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/10/2022  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PDSECURE_HPP_
#define PDSECURE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "utilSecure.hpp"

namespace engine
{
   // input: BSONObj
   #define PD_SECURE_OBJ( obj ) PD_SECURE_STR( (obj).toPoolString() )

   // input: ossPoolString or string
   #define PD_SECURE_STR( str ) \
      ( ( pdIsDiaglogSecureEnabled() && pdLocalIsDiaglogSecureEnabled() ) ? \
        utilSecureStr( str ).c_str() : str.c_str() )

   // input: const CHAR*
   #define PD_SECURE_STR1( data ) \
      ( ( pdIsDiaglogSecureEnabled() && pdLocalIsDiaglogSecureEnabled() ) ? \
        utilSecureStr( data, ossStrlen( data ) ).c_str() : data )
}

#endif
