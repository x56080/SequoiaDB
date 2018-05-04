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

   Source File Name = pmdSignalHandler.hpp

   Descriptive Name = Process MoDel Signal Handler Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains function used for signal
   handler.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/04/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDSIGNALHANDLER_HPP_
#define PMDSIGNALHANDLER_HPP_

#include "core.hpp"

namespace engine
{

   /*
      _signalInfo define
   */
   struct _pmdSignalInfo
   {
      const CHAR        *_name ;
      INT32             _handle ;
   } ;
   typedef _pmdSignalInfo pmdSignalInfo ;

   /*
      Tool functions
   */
   pmdSignalInfo& pmdGetSignalInfo( INT32 signum ) ;

}

#endif // PMDSIGNALHANDLER_HPP_
