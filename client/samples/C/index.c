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
#define INDEX_NAME            "index"

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

   // read argument
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUsr         = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 1 ) ;
   }

   // connect to database
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &connection ) ;
   CHECK_RC ( rc, "Failed to connet to database" ) ;

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   CHECK_RC ( rc, "Failed to create collection space" ) ;

   // create collection in a specified colletion space.
   // Here,we build it up in the new collection.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   CHECK_RC ( rc, "Failed to create collection" ) ;

   // build a bson for index definition
   bson_init( &obj ) ;
   rc = bson_append_int( &obj, "name", 1 ) ;
   CHECK_RC ( rc, "Failed to append data" ) ;
   rc = bson_append_int( &obj, "age", -1 ) ;
   CHECK_RC ( rc, "Failed to append data" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Failed to build bson" ) ;
   printf("The index to build is: ") ;
   bson_print ( &obj ) ;

   // create index
   rc = sdbCreateIndex ( collection, &obj, INDEX_NAME, FALSE, FALSE ) ;
   CHECK_RC ( rc, "Failed to create index" ) ;

   bson_destroy ( &obj ) ;
   printf("Suceess to build index!" OSS_NEWLINE ) ;

done:
   // drop collection space
   rc = sdbDropCollectionSpace( connection, COLLECTION_SPACE_NAME ) ;
   if ( rc != SDB_OK )
   {
      printf ( "Failed to drop collection space, rc = %d"OSS_NEWLINE, rc ) ;
   }
   // disconnect the connection
   sdbDisconnect ( connection ) ;
   // release the local variables
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
   return rc ;
error:
   goto done ;
}

