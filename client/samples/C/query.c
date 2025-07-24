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

#define NUM_RECORD 5

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"
#define COLLECTION_FULL_NAME  "foo.bar"

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
   INT32 rc    = SDB_OK ;
   INT32 count = 0 ;
   bson obj ;
   bson rule ;
   bson objList [ NUM_RECORD ] ;

   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }
   // read argument
   pHostName    = (CHAR*)argv[1] ;
   pServiceName = (CHAR*)argv[2] ;
   pUsr         = (CHAR*)argv[3] ;
   pPasswd      = (CHAR*)argv[4] ;

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

   // create record list using objList
   createRecordList ( &objList[0], NUM_RECORD ) ;

   // insert obj and free memory that allocated by createRecordList
   for ( count = 0; count < NUM_RECORD; count++ )
   {
      // all the contents of inserted record are the same except _id
      rc = sdbInsert ( collection, &objList[count] ) ;
      if ( rc )
      {
         printf ( "Failed to insert record, rc = %d" OSS_NEWLINE, rc ) ;
      }
      bson_destroy ( &objList[count] ) ;
   }

   // query all the record in this collection
   // and return the result by the cursor handle
   rc = sdbQuery(collection, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   CHECK_RC ( rc, "Failed to query" ) ;

   // get all the qureied records
   bson_init(&obj);
   while( !( rc=sdbNext( cursor, &obj ) ) )
   {
      bson_print( &obj ) ;
      bson_destroy(&obj) ;
      bson_init(&obj);
   }
   bson_destroy(&obj) ;
   if( rc==SDB_DMS_EOC )
   {
      printf("All the record had been list." OSS_NEWLINE ) ;
   }
   else if( rc!=SDB_OK )
   {
      CHECK_RC ( rc, "Failed to get the record" ) ;
   }

   // drop the specified collection
   rc = sdbDropCollection( collectionspace,COLLECTION_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection" ) ;

   // drop the specified collection space
   rc = sdbDropCollectionSpace( connection,COLLECTION_SPACE_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection space" ) ;

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
