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
#include <iostream>
#include "common.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace sample ;

#define COLLECTION_SPACE_NAME "foo"
#define COLLECTION_NAME       "bar"

// Display Syntax Error
void displaySyntax ( CHAR *pCommand ) ;

INT32 main ( INT32 argc, CHAR **argv )
{
   // verify syntax
   if ( 5 != argc )
   {
      displaySyntax ( (CHAR*)argv[0] ) ;
      exit ( 0 ) ;
   }
   // initialize local variables
   CHAR *pHostName    = (CHAR*)argv[1] ;
   CHAR  *Port        = (CHAR*)argv[2] ;
   CHAR *pUsr         = (CHAR*)argv[3] ;
   CHAR *pPasswd      = (CHAR*)argv[4] ;

   // define a sdb object
   // use to connect to database
   sdb connection ;
   // define a sdbCollectionSpace object
   sdbCollectionSpace collectionspace ;
   // define a sdbCollection object
   sdbCollection collection ;
   // define a sdbCursor object for query
   sdbCursor cursor ;

   // define local variables
   // initialize them before use
   BSONObj obj ;
   BSONObj modifier ;
   BSONObj matcher ;
   BSONObj hint ;
   BSONObj result ;

   INT32 rc = SDB_OK ;

   // connect to database
   rc = connection.connect ( pHostName, Port, pUsr, pPasswd ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to connet to database, rc = "<<rc<<endl ;
      goto error ;
   }

   // create collection space
   rc = connection.createCollectionSpace ( COLLECTION_SPACE_NAME,
                                   SDB_PAGESIZE_4K, collectionspace ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to create collection space, rc = "<<rc<<endl ;
      goto error ;
   }

   // create collection in a specified colletion space.
   // Here,we build it up in the new collection.
   rc = collectionspace.createCollection ( COLLECTION_NAME, collection ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to create collection, rc = "<<rc<<endl ;
      goto error ;
   }

   // insert records to the collection
   // insert a English record
   createEnglishRecord ( obj  ) ;
   rc = collection.insert ( obj ) ;
   if ( rc )
   {
      cout<<"Faileded to insert record, rc = "<<rc<<endl ;
   }

   // query the records
   // the result is in the cursor object
   rc = collection.query ( cursor ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to query, rc = "<<rc<<endl ;
      goto error ;
   }

   // update the record
   modifier = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"The update rule is: " << modifier.toString() <<endl ;
   rc = collection.update( modifier, matcher, hint, 0, &result ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failede to update the record, rc = "<<rc<<endl ;
      goto error ;
   }
   cout<< "Update result: " << result.toString() <<endl ;

   // drop the specified collection
   rc = collectionspace.dropCollection( COLLECTION_NAME ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to drop the specified collection,rc = "<<rc<<endl ;
      goto error ;
   }

   // drop the specified collection space
   rc = connection.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   if( rc!=SDB_OK )
   {
      cout<<"Failed to drop the specified collection,rc = "<<rc<<endl ;
      goto error ;
   }

done:
   // disconnect from database
   connection.disconnect () ;
   return 0;
error:
   goto done ;
}

// Display Syntax Error
void displaySyntax ( CHAR *pCommand )
{
   cout<<"Syntax:"<<pCommand<<" <hostname> <servicename>\
 <username> <password>"<<endl ;
}

