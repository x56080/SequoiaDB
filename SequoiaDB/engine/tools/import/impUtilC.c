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

   Source File Name = impUtilC.c

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2023/02/10  TZB Initial Draft

   Last Changed =

*******************************************************************************/
#include "impUtilC.h"
#include "../client/client.h"

INT32 checkConnInfo( const CHAR *pHostname, const CHAR *pSvcname,
                     const CHAR *pUser, const CHAR *pPassword, BOOLEAN useSSL )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;

   if (useSSL)
   {
      rc = sdbSecureConnect( pHostname, pSvcname, pUser, pPassword, &conn ) ;
   }
   else
   {
      rc = sdbConnect( pHostname, pSvcname, pUser, pPassword, &conn ) ;
   }
   if ( SDB_INVALID_HANDLE != conn )
   {
      sdbDisconnect( conn ) ;
      sdbReleaseConnection( conn ) ;
   }
   return rc ;
}
