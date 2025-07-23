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

   Source File Name = clsLocalValidation.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clsLocalValidation.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"


using namespace bson ;

namespace engine
{
   /*static void func()
   {
      return ;
   }*/

   INT32 _clsLocalValidation::run()
   {
      /// 4. update validation tick
      pmdUpdateValidationTick() ;

      return SDB_OK ;
   }

}

