/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rtnSortDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSORTDEF_HPP_
#define RTNSORTDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{
   enum RTN_SORT_STEP
   {
      RTN_SORT_STEP_BEGIN = 0,
      RTN_SORT_STEP_FETCH_FROM_INTER,
      RTN_SORT_STEP_FETCH_FROM_MERGE,
   } ;

  const UINT32 RTN_SORT_MIN_BUFSIZE = 128 ;
  const UINT32 RTN_SORT_MIN_MERGESIZE = 3 ;
  const UINT32 RTN_SORT_MAX_MERGESIZE = 10 ;

}

#endif

