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

   Source File Name = rtnMsg.cpp

   Descriptive Name = Runtime Message

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   message processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "rtn.hpp"
#include "pd.hpp"


namespace engine
{

   INT32 rtnMsg ( MsgOpMsg *pMsg )
   {
      INT32 rc = SDB_OK ;

      if ( (UINT32)pMsg->header.messageLength < sizeof( MsgHeader ) )
      {
         PD_LOG( PDERROR, "Recieve invalid msg[length: %d]",
                 pMsg->header.messageLength ) ;
         rc = SDB_INVALIDARG ;
      }
      else
      {
         CHAR *message = &pMsg->msg[0] ;
         message[ pMsg->header.messageLength - sizeof(MsgHeader) - 1 ] = 0 ;
         PD_LOG ( getPDLevel(), "%s", message ) ;
      }
      return rc ;
   }

}

