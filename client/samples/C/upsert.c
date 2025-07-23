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

#define COLLECTION_SPACE_NAME "cs"
#define COLLECTION_NAME       "cl"

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
   bson rule ;
   bson condition ;
   INT32 rc = SDB_OK ;

   // read argument
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUsr         = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

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

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to create collection space, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection in a specified colletion space.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to create collection, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // insert records to the collection
   bson_init( &obj ) ;
   // insert a English record
   createEnglishRecord ( &obj  ) ;
   rc = sdbInsert ( collection, &obj ) ;
   if ( rc )
   {
      printf ( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
   }
   bson_destroy ( &obj ) ;

   // query the records
   // the result is in the cursor handle
   rc = sdbQuery(collection, NULL, NULL,  NULL, NULL, 0, -1, &cursor ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to query, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // update the record
   // let's set the rule and query condition first
   // here,we make the condition to be NULL
   // so all the records will be update
   bson_init( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_start_object ( &rule, "dev_id" ) ;
   bson_append_binary ( &rule, "id_rand", BSON_BIN_BINARY, "a", 1 ) ;
   bson_append_start_object ( &rule, "id_s" ) ;
   bson_append_int ( &rule, "year", 2005 ) ;
   bson_append_int ( &rule, "mon", 11 ) ;
   bson_append_int ( &rule, "day", 10 ) ;
   bson_append_int ( &rule, "num", 1024 ) ;
   bson_append_finish_object ( &rule ) ;
   //bson_append_binary ( &rule, "id_rand", BSON_BIN_BINARY, "a", 1 ) ;
   bson_append_finish_object ( &rule ) ;
   bson_append_finish_object ( &rule ) ;
   rc = bson_finish ( &rule ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

   printf("The update rule is:") ;
   bson_print( &rule ) ;

   bson_init ( &condition ) ;
   bson_append_start_object ( &condition, "dev_id.id_s" ) ;
   bson_append_int ( &condition, "year", 2007 ) ;
   bson_append_int ( &condition, "mon", 11 ) ;
   bson_append_int ( &condition, "day", 10 ) ;
   bson_append_int ( &condition, "num", 1024 ) ;
   bson_append_finish_object ( &condition ) ;
   rc = bson_finish ( &condition ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

   rc = sdbUpsert( collection, &rule, &condition, NULL ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &condition ) ;
   if( rc!=SDB_OK )
   {
      printf("Failed to update the record, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }
   printf("Success to update!" OSS_NEWLINE ) ;

done:
   // drop collection space
   rc = sdbDropCollectionSpace( connection, COLLECTION_SPACE_NAME ) ;
   if ( rc != SDB_OK )
   {
      printf("Failed to drop the specified collection,\
              rc = %d" OSS_NEWLINE, rc ) ;
   }
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
   return 0;
error:
   goto done ;
}

