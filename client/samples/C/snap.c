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
   /* initialize local variables */
   INT32 count                       = 0 ;
   CHAR *pHostName                   = NULL ;
   CHAR *pServiceName                = NULL ;
   CHAR *pUser                       = NULL ;
   CHAR *pPasswd                     = NULL ;
   sdbConnectionHandle connection    = 0 ;
   sdbCollectionHandle collection    = 0 ;
   sdbCursorHandle cursor            = 0 ;
   bson obj ;
   INT32 rc                          = 0 ;
   /* verify syntax */
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }

   /* read argument */
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUser        = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

   /* connect to database */
   rc = connectTo ( pHostName, pServiceName, pUser, pPasswd, &connection ) ;
   if ( rc )
   {
      printf ( "Failed to connect to database at %s:%s, rc = %d"
               OSS_NEWLINE,
               pHostName, pServiceName, rc ) ;
      exit ( 0 ) ;
   }

   rc = sdbGetSnapshot ( connection, SDB_SNAP_DATABASE, NULL, NULL, NULL,
                         &cursor ) ;
   if ( rc )
   {
      printf ( "Failed to get snapshot, rc = %d\n", rc ) ;
      exit ( 0 ) ;
   }
   while ( TRUE )
   {
      bson_init ( &obj ) ;
      rc = sdbNext ( cursor, &obj ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            printf ( "Failed to fetch next record from collection, rc = %d"
                     OSS_NEWLINE, rc ) ;
         }
         break ;
      }
      printf ( "Record Read [ %d ]: " OSS_NEWLINE, count ) ;
      bson_print ( &obj ) ;
      bson_destroy ( &obj ) ;
      ++ count ;
   }

   /* disconnect from server */
   sdbDisconnect ( connection ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseConnection ( connection ) ;
   return 0 ;
}
