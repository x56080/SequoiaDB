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

   
*******************************************************************************/
#include <stdio.h>
#include "common.h"


INT32 main ( INT32 argc, CHAR **argv )
{
   // define a connecion handle; use to connect to database
   sdbConnectionHandle connection    = 0 ;
   INT32 rc = SDB_OK ;

   // read argument
   CHAR *pHostName    = (CHAR*)argv[1] ;
   CHAR *pServiceName = (CHAR*)argv[2] ;
   CHAR *pUsr         = (CHAR*)argv[3] ;
   CHAR *pPasswd      = (CHAR*)argv[4] ;

   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }

   // connect to database
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to connet to database, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   else
      printf( "connect success!\n") ;

   // disconnect from database
   sdbDisconnect ( connection ) ;
   // release connection
   sdbReleaseConnection ( connection ) ;
done:
   return 0 ;
error:
   goto done ;
}


