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

   Source File Name = utilEnvCheck.hpp

   Descriptive Name =

   When/how to use: linux environment check util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== =========== ==============================================
          20/04/2016  Chen Chucai Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTILENVCHECK_HPP__
#define UTILENVCHECK_HPP__

#include "ossTypes.h"


namespace engine
{

   BOOLEAN utilCheckIs64BitSys() ;
   BOOLEAN utilCheckIsOpenVZ() ;
   BOOLEAN utilCheckNumaStatus() ;
   BOOLEAN utilCheckVmStatus() ;
   BOOLEAN utilCheckThpStatus() ; //thp : transparent_hugepage
  
   BOOLEAN utilCheckEnv() ;
}

#endif //UTILENVCHECK_HPP_
