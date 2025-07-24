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

   Source File Name = sptApi.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_API_HPP_
#define SPT_API_HPP_

#include "sptInjection.hpp"
#include "sptObjDesc.hpp"
#include "sptInvoker.hpp"
#include "sptContainer.hpp"
#include "sptScope.hpp"
#include "sptArguments.hpp"
#include "sptReturnVal.hpp"
#include "sptObject.hpp"

#define SPT_CLASS_DEF( c )\
        ( (c)->__desc.getClassDef() )

#endif

