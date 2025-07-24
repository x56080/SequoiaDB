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

#define BUFSIZE 1000

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"
#define COLLECTION_FULL_NAME  "foo.bar"



INT32 main ( INT32 argc, CHAR **argv )
{
   // initialize local variables
   CHAR *pHostName             = NULL ;
   CHAR *pServiceName          = NULL ;
   CHAR *pUsr                  = NULL ;
   CHAR *pPasswd               = NULL ;
   // define handles
   sdbConnectionHandle db      = SDB_INVALID_HANDLE ;
   sdbCSHandle cs              = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl      = SDB_INVALID_HANDLE ;
   sdbLobHandle lob            = SDB_INVALID_HANDLE ;

   INT32 rc                    = SDB_OK ;
   CHAR buf[BUFSIZE]           = { 'a' } ;
   bson_oid_t oid ;

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
   rc = sdbConnect ( pHostName, pServiceName, pUsr, pPasswd, &db ) ;
   CHECK_RC ( rc, "Failed to connet to database" ) ;

   // create collection space
   rc = sdbCreateCollectionSpace ( db, COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, &cs ) ;
   CHECK_RC ( rc, "Failed to create collection space" ) ;

   // create collection
   rc = sdbCreateCollection ( cs, COLLECTION_NAME, &cl ) ;
   CHECK_RC ( rc, "Failed to create collection" ) ;

   // create a large object and than write something to it
   // we need to gen an oid for the creating large object
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   CHECK_RC ( rc, "Failed to create large object" ) ;

   // write something to the newly created large object
   rc = sdbWriteLob( lob, buf, BUFSIZE ) ;
   CHECK_RC ( rc, "Failed to write to large object" ) ;
   
   // close the newly created large object
   rc = sdbCloseLob( &lob ) ;
   CHECK_RC ( rc, "Failed to close large object" ) ;

   printf( "Success to create a large object and write something to it!\n" ) ;
	  
   // drop the collection
   rc = sdbDropCollection( cs,COLLECTION_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection" ) ;

   // drop the collection space
   rc = sdbDropCollectionSpace( db,COLLECTION_SPACE_NAME ) ;
   CHECK_RC ( rc, "Failed to drop the specified collection space" ) ;

done:
   // disconnect the connection
   sdbDisconnect ( db ) ;
   // release the handles
//   sdbReleaseLob ( lob ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
   return 0;
error:
   goto done ;
}

