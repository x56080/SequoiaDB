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

   Source File Name = rtnCoordAuth.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNCOORDAUTH_HPP_
#define RTNCOORDAUTH_HPP_

#include "rtnCoordAuthBase.hpp"

namespace engine
{
   class rtnCoordAuth : public rtnCoordAuthBase
   {
   public:
      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   private:

      virtual void   _onSucReply( const MsgOpReply *pReply ) ;

   } ;
}

#endif

