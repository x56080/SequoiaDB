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

   Source File Name = clsLocalValidation.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_LOCALVALIDATION_HPP_
#define CLS_LOCALVALIDATION_HPP_

#include "oss.hpp"
#include "core.hpp"

namespace engine
{
   class _clsLocalValidation : public SDBObject
   {
   public:
      INT32 run() ;
   } ;
}

#endif

