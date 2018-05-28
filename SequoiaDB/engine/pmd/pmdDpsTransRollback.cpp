/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = pmdDpsTransRollback.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "ossUtil.h"

namespace engine
{
   INT32 pmdDpsTransRollbackEntryPoint( pmdEDUCB *cb, void *pData )
   {
      INT32 rc = SDB_OK ;

      while( !cb->isDisconnected() )
      {
         pmdEDUEvent event;
         if ( cb->waitEvent( event, OSS_ONE_SEC, TRUE ) )
         {
            if ( PMD_EDU_EVENT_TERM == event._eventType )
            {
               PD_LOG ( PDEVENT, "EDU[%lld] is terminated",
                        cb->getID() );
               rc = SDB_APP_DISCONNECT;
               break;
            }
            else if ( PMD_EDU_EVENT_ACTIVE == event._eventType )
            {
               rc = rtnTransRollbackAll( cb );
            }
            pmdEduEventRelase( event, cb ) ;
         }
      }
      rc = SDB_OK ;
      return rc ;
   }
}

