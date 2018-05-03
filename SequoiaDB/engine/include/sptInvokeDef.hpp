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

   Source File Name = sptInvokeDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_INVOKEDEF_HPP_
#define SPT_INVOKEDEF_HPP_

#include "jsapi.h"

namespace engine
{
   namespace JS_INVOKER
   {
      typedef JSBool (*MEMBER_FUNC)(JSContext *cx , uintN argc , jsval *vp) ;

      typedef void (*DESTRUCT_FUNC)(JSContext *cx , JSObject *obj) ;

      typedef JSBool (*RESLOVE_FUNC)( JSContext *cx , JSObject *obj , jsid id ,
                                      uintN flags , JSObject ** objp) ;
   }
}

#endif // SPT_INVOKEDEF_HPP_

