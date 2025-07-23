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
   CHECK_RC ( rc, "Failed to connet to database" ) ;

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   CHECK_RC ( rc, "Failed to create collection space" ) ;

   // create collection in a specified colletion space.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   CHECK_RC ( rc, "Failed to create collection" ) ;

   // insert records to the collection
   bson_init ( &obj ) ;
   // build a English record
   createEnglishRecord ( &obj  ) ;

   // insert
   rc = sdbInsert ( collection, &obj ) ;
   bson_destroy( &obj ) ;
   CHECK_RC ( rc, "Failed to insert record" ) ;

   // query the records
   // the result is in the cursor handle
   rc = sdbQuery(collection, NULL, NULL,  NULL, NULL, 0, -1, &cursor ) ;
   CHECK_RC ( rc, "Failed to query" ) ;

   // update the record
   // let's set the rule and query condition first
   // here,we make the condition to be NULL
   // so all the records will be update
   bson_init ( &rule ) ;
   bson_append_start_object ( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 19 ) ;
   bson_append_finish_object ( &rule ) ;
   bson_finish ( &rule ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;

   printf ( "The update rule is:" ) ;
   bson_print( &rule ) ;

   // update
   rc = sdbUpdate ( collection, &rule, NULL, NULL ) ;
   CHECK_RC ( rc, "Failed to update the record" ) ;

   // free memory after finish using bson
   bson_destroy ( &rule ) ;
   printf ( "Success to update!" OSS_NEWLINE ) ;

   // drop the specified collection
   rc = sdbDropCollection ( collectionspace,COLLECTION_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection" ) ;

   // drop the specified collection space
   rc = sdbDropCollectionSpace ( connection,COLLECTION_SPACE_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection" ) ;

done:
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

