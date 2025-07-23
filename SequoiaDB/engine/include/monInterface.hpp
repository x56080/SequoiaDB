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

   Source File Name = monInterface.hpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains structure for
   application and snapshot.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/10/2022  TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MON_INTERFACE_HPP__
#define MON_INTERFACE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "monCB.hpp"

namespace engine
{

   /*
      _IMonSubmitEvent define
   */
   class _IMonSubmitEvent
   {
      public:
         _IMonSubmitEvent() {}
         virtual ~_IMonSubmitEvent() {}

      public:
         virtual void onSubmit( const monAppCB &delta ) = 0 ;
   } ;
   typedef _IMonSubmitEvent IMonSubmitEvent ;


}

#endif // MON_INTERFACE_HPP__

