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

   Source File Name = pmdDummySession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date     Who     Description
   ====== ======== ======= ==============================================
          2020/8/8 Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdDummySession.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   MsgRouteID _pmdDummySession::identifyNID()
   {
      MsgRouteID route ;
      route.value = MSG_INVALID_ROUTEID ;
      return route ;
   }

   void _pmdDummySession::attachCB ( _pmdEDUCB *cb )
   {
      _pEDUCB = cb ;
      _eduID  = cb->getID() ;
      _pEDUCB->clearProcessInfo() ;
      _pEDUCB->attachSession( this ) ;
      _client.attachCB( _pEDUCB ) ;
   }

   void _pmdDummySession::detachCB ()
   {
      _client.detachCB() ;
      _pEDUCB->clearProcessInfo() ;
      _pEDUCB->detachSession() ;
      _pEDUCB = NULL ;
   }
}
