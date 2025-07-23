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

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"
#define NUM 3

const CHAR *sql1 = "select * from foo.bar" ;
const CHAR *sql2 = "insert into foo.bar ( name, age ) values( \"Aimee\",23 )" ;


INT32 main ( INT32 argc, CHAR **argv )
{
   // initialize local variables
   CHAR *pHostName                   = NULL ;
   CHAR *pServiceName                = NULL ;
   CHAR *pUsr                        = NULL ;
   CHAR *pPasswd                     = NULL ;
   // define a connetion handle; use to connect to database
   sdbConnectionHandle connection    = 0 ;
   // define a collection space handle
   sdbCSHandle collectionspace       = 0 ;
   // define a collection handle
   sdbCollectionHandle collection    = 0 ;
   // define a cursor handle for query
   sdbCursorHandle cursor            = 0 ;

   // define local variables
   // initialize them before use
   bson obj ;
   INT32 rc = SDB_OK ;
   INT32 i = 0 ;

   // read argument
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUsr         = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // connect to database
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to connet to database, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to create collection space, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to create collection, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   
   // load some data
   for( i = 0; i < NUM; i++ )
   {
      bson_init( &obj ) ;
      bson_append_string( &obj, "name", "tom" ) ;
      bson_append_int( &obj, "age", 24 ) ;
      rc = bson_finish( &obj ) ;
      CHECK_RC ( rc, "Failed to build bson" ) ;

      rc = sdbInsert ( collection, &obj ) ;
      if ( rc )
      {
         printf ( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
         bson_destroy( &obj ) ;
         goto error ;
      }
      bson_destroy( &obj ) ;
   }

   // execute sql query, the return result is in cursor
   printf("The operation is :\n") ;
   printf("%s\n",sql1) ;
   rc = sdbExec ( connection, sql1, &cursor ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to execute, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // display all the record
   printf("The result are as below:\n") ;
   bson_init( &obj ) ;
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj );
   }
   bson_destroy( &obj ) ;
   sdbReleaseCursor ( cursor ) ;

   // execute sql insert
   printf("The operation is :\n") ;
   printf("%s\n",sql2) ;
   rc = sdbExecUpdate ( connection, sql2 ) ;
   if ( rc )
   {
      printf ( "Failed to execute, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   // display all the record
   rc = sdbQuery ( collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   if( rc != SDB_OK )
   {
      printf("Failed to query all the records,\
              rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   printf("The result are as below:\n") ;
   bson_init( &obj ) ;
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy( &obj ) ;
      bson_init( &obj );
   }
   bson_destroy( &obj ) ;
   sdbReleaseCursor ( cursor ) ;

done:
   // drop the specified collection space
   sdbDropCollectionSpace ( connection,COLLECTION_SPACE_NAME ) ;
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
   return 0;
error:
   goto done ;
}


