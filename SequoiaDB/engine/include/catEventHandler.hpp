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

   Source File Name = catEventHandler.hpp

   Descriptive Name = Interfaces for Catalog event handlers

   When/how to use: N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/13/2016  hgm Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_EVENT_HANDLER_HPP_
#define CAT_EVENT_HANDLER_HPP_

#include "msg.hpp"
#include "netMsgHandler.hpp"

namespace engine
{

   /*
      _catEventHandler define
   */
   class _catEventHandler
   {
   public :
      _catEventHandler () {}

      virtual ~_catEventHandler () {}

      virtual const CHAR *getHandlerName () = 0 ;

      virtual INT32 onBeginCommand ( MsgHeader *pReqMsg ) = 0 ;

      virtual INT32 onEndCommand ( MsgHeader *pReqMsg, INT32 result ) = 0 ;

      virtual INT32 onSendReply ( MsgOpReply *pReply, INT32 result ) = 0 ;
   } ;

}

#endif // CAT_EVENT_HANDLER_HPP_
