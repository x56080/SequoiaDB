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
   INT32 rc                          = SDB_OK ;
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

   // define local variables
   // initialize them before use
   bson modifier ;
   bson result ;
   bson_init( &modifier ) ;
   bson_init( &result ) ;

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
   if( SDB_OK != rc )
   {
      printf("Failed to connet to database, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection space
   rc = sdbCreateCollectionSpace ( connection, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &collectionspace ) ;
   if( SDB_OK != rc )
   {
      printf("Failed to create collection space, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // create collection in a specified colletion space.
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME, &collection ) ;
   if( SDB_OK != rc )
   {
      printf("Failed to create collection, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // update the record
   bson_append_start_object ( &modifier, "$set" ) ;
   bson_append_int ( &modifier, "age", 19 ) ;
   bson_append_finish_object ( &modifier ) ;
   rc = bson_finish( &modifier ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      printf( "Failed to create the modifier bson, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   printf ( "The update rule is: " ) ;
   bson_print( &modifier ) ;

   rc = sdbUpsert3( collection, &modifier, NULL, NULL, NULL, 0, &result ) ;
   if( SDB_OK != rc )
   {
      printf("Failed to upsert the record, rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   printf("Upsert result: ") ;
   bson_print( &result ) ;

   // drop the specified collection
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   if( SDB_OK != rc )
   {
      printf( "Failed to drop the specified collection, "
              "rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // drop the specified collection space
   rc = sdbDropCollectionSpace( connection, COLLECTION_SPACE_NAME ) ;
   if( SDB_OK != rc )
   {
      printf( "Failed to drop the specified collection space, "
	  	"rc = %d" OSS_NEWLINE, rc ) ;
      goto error ;
   }

done:
   bson_destroy( &modifier ) ;
   bson_destroy( &result ) ;
   // disconnect the connection
   if ( connection )
   {
      sdbDisconnect( connection ) ;
   }
   // release the local variables
   if ( collection )
   {
      sdbReleaseCollection( collection ) ;
   }
   if ( collectionspace )
   {
      sdbReleaseCS( collectionspace ) ;
   }
   if ( connection )
   {
      sdbReleaseConnection( connection ) ;
   }
   return 0;
error:
   goto done ;
}

