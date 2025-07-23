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

#define SAMPLE_DATA_FILE_NAME "sampledb.dat"
#define MAX_BUF_SZ 16384

INT32 loadFromFile ( const CHAR *pFileName,
                     sdbConnectionHandle connection )
{
   CHAR *p = NULL ;
   FILE *pFile = NULL ;
   CHAR buffer [ MAX_BUF_SZ + 1 ] ;
   CHAR *pCollectionName = NULL ;
   bson obj ;
   bson_init ( &obj ) ;
   pFile = fopen ( pFileName, "r" ) ;
   if ( !pFile )
   {
      printf ( "Not able to open file %s\n", pFileName ) ;
      return SDB_IO ;
   }
   while ( TRUE )
   {
      memset ( buffer, 0, sizeof(buffer) ) ;
      p = fgets ( buffer, MAX_BUF_SZ, pFile ) ;
      if ( !p )
      {
         break ;
      }
      if ( isComment ( p ) )
         continue ;
      if ( NULL != ( pCollectionName = loadTag ( p ) ) )
      {
         printf ( "Tag: %s\n", pCollectionName ) ;
      }
      else if ( loadJSON ( p, &obj ) )
      {
         printf ( "JSON: %s\n", p ) ;
         bson_destroy ( &obj ) ;
         bson_init ( &obj ) ;
      }
   }
   bson_destroy ( &obj ) ;
   fclose( pFile ) ;
   return SDB_OK ;
}

/* main function */
INT32 main ( INT32 argc, CHAR **argv )
{
   /* initialize local variables */
   CHAR *pHostName                   = NULL ;
   CHAR *pServiceName                = NULL ;
   CHAR *pUser                       = NULL ;
   CHAR *pPasswd                     = NULL ;
   sdbConnectionHandle connection    = 0;
   INT32 rc                          = 0 ;
   /* verify syntax */
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
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
      printf ( "Failed to connect to database at %s:%s, rc = %d\n",
               pHostName, pServiceName, rc ) ;
      goto error ;
   }

   rc = loadFromFile ( SAMPLE_DATA_FILE_NAME, connection ) ;
   if ( rc )
   {
      printf ( "Failed to load from file %s, rc = %d\n",
               SAMPLE_DATA_FILE_NAME, rc ) ;
      goto error ;
   }

   sdbDisconnect( connection ) ;

done:
   /* dispose connection */
   sdbReleaseConnection ( connection ) ;
   return rc ;
error:
   goto done ;
}
