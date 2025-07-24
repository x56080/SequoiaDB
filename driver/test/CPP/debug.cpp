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

   Source File Name = debug.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

#define USERNAME               "SequoiaDB"
#define PASSWORD               "SequoiaDB"

#define NUM_RECORD             5

TEST ( debug, test )
{
   ASSERT_TRUE( 1 == 1 ) ;
}

/*
// date
TEST ( debug, date )
{
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;

   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   INT32 rc = SDB_OK ;
   BSONObj obj ;
   BSONObjBuilder ob ;
//   time_t s = time(NULL) ;
   time_t s = 1379404680 ;
   unsigned long long millis = s*1000 ;
//   Date_t date ( time(NULL)*1000 ) ;
   Date_t date ( millis ) ;
   cout<<"date is: "<<date.toString ()<<endl ;
   ob.appendDate ( "date", date ) ;
   obj = ob.obj () ;
   cout<<obj.toString ()<<endl ;
   ASSERT_TRUE ( obj.toString()=="{ \"date\": {\"$date\": \"2013-09-17\"} }" ) ;
   // create sdb
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cs
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   rc = cl.insert( obj, NULL ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   db.disconnect() ;

}


TEST ( debug, time_stamp )
{
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;

   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   INT32 rc = SDB_OK ;

   BSONObj obj ;
   BSONObjBuilder ob ;
   BSONObjBuilder ob1 ;
   ob.appendTimestamp ( "timestamp", 1379323441000, 0 ) ;
   obj = ob.obj () ;
   cout<<"timestamp1 is: "<<obj.toString()<<endl ;
   ASSERT_TRUE ( obj.toString() == "{ \"timestamp\": {\"$timestamp\": \
\"2013-09-16-17.24.01.000000\"} }" ) ;

   ob1.appendTimestamp ( "timestamp", 1379323441000, 1 ) ;
   obj = ob1.obj () ;
   cout<<"timestamp2 is: "<<obj.toString()<<endl ;
   ASSERT_TRUE ( obj.toString() == "{ \"timestamp\": {\"$timestamp\": \
\"2013-09-16-17.24.01.000001\"} }" ) ;

   // create sdb
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cs
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   rc = cl.insert( obj, NULL ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   db.disconnect() ;

}

*/

//TEST(debug, getNodeStatus)
//{
//   sdb db ;
//   sdbCollectionSpace cs ;
//   sdbReplicaGroup rg ;
//
//   // initialize local variables
//   const CHAR *pHostName                    = HOST ;
//   const CHAR *pPort                        = SERVER ;
//   const CHAR *pUsr                         = USER ;
//   const CHAR *pPasswd                      = PASSWD ;
//   INT32 rc                                 = SDB_OK ;
//   INT32 replcaGroupId                      = 1000 ;
//   sdbNodeStatus status                     = SDB_NODE_ALL ;
//   INT32 nodeNum                            = 0 ;
///*
//      SDB_NODE_ACTIVE,
//      SDB_NODE_INACTIVE,
//      SDB_NODE_DOWN,
//      SDB_NODE_ABNORMAL,
//      SDB_NODE_UNKNOWN
//*/
//   // connect to database
//   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//   rc = db.getReplicaGroup( replcaGroupId, rg ) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//   rc = rg.getNodeNum( status, &nodeNum ) ;
//   ASSERT_TRUE( rc==SDB_OK ) ;
//
//   printf( "nodeNum is: %d\n", nodeNum );
//   // disconnect the connection
//   db.disconnect() ;
//}


/*
TEST( debug, SdbIsValid )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   sdbCursor cursor2 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BOOLEAN result = FALSE ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   // TODO:
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   std::cout<< "before close connection, result is " << result << endl ;
   ASSERT_TRUE( result == TRUE ) ;
   // disconnect manually
   result = TRUE ;
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   std::cout<< "after close connection manually, result is " << result << endl ;
   ASSERT_TRUE( result == TRUE ) ;
   // close
   connection.disconnect() ;
   result = FALSE ;
   rc = connection.isValid( &result ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   std::cout<< "after close connection, result is " << result << endl ;
   ASSERT_TRUE( result == FALSE ) ;
}


TEST( debug, connect )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR* connArr[10] = {"192.168.20.35:12340",
                              "192.168.20.36:12340",
                              "123:123",
                              "",
                              ":12340",
                              "192.168.20.40",
                              "localhost:50000",
                              "192.168.20.40:12340",
                              "localhost:12340",
                              "localhost:11810"} ;
   // connect to database
   rc = connection.connect( connArr, 10, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   rc = connection.getList( cursor, 5);
   ASSERT_TRUE( rc==SDB_OK ) ;
   // disconnect the connection
   connection.disconnect() ;
}
*/
/*
TEST( debug, cl_alter )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   const CHAR* pCS = "cs_cl_alter" ;
   const CHAR* pCL = "cl_cl_alter" ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   // connect
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // create cs
   BSONObj cs_opt ;
   rc = db.createCollectionSpace( pCS, cs_opt, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // create cl
   BSONObj cl_opt ;
//   BSONObj cl_opt = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" <<
//                          "range" << "ReplSize" << 2 ) ;
   rc = cs.createCollection( pCL, cl_opt, cl ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // alter cl
   BSONObj cl_opt2 = BSON( "ShardingKey" << BSON( "b" << 1 ) << "ShardingType" <<
                           "hash" << "ReplSize" << 1 << "Partition" << 1024 ) ;
   rc = cl.alterCollection( cl_opt2 ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;

   // drop cs
   rc = db.dropCollectionSpace( pCS ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   db.disconnect() ;

}

TEST(debug,connect_with_serval_addr)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR* connArr[10] = {"192.168.20.35:12340",
                              "192.168.20.36:12340",
                              "123:123",
                              "",
                              ":12340",
                              "192.168.20.40",
                              "localhost:50000",
                              "192.168.20.40:12340",
                              "localhost:12340",
                              "localhost:11810"} ;
   // connect to database
   rc = connection.connect( connArr, 10, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   rc = connection.getList( cursor, 5);
   ASSERT_TRUE( rc==SDB_OK ) ;
   // disconnect the connection
   connection.disconnect() ;
}
*/


/*
TEST(debug,next_current)
{
   sdb connection ;
   sdbCollection cl ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   sdbCursor cursor2 ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // insert records
   rc = insertRecords ( cl, 1 ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // query all the record in this collection
   rc = cl.query( cursor ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   rc = connection.getSnapshot( cursor2, 5 ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;

   // check
   // get the next record
   rc = cursor.next( obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout<<"The next record is:"<<endl;
   cout<< obj.toString() << endl ;
   // get the current record
   rc = cursor.current( obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() << endl ;

   // check again
   rc = cursor2.next( obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout<<"The next record is:"<<endl;
   cout<< obj.toString() << endl ;
   rc = cursor2.current( obj ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() << endl ;
   // disconnect the connection
   connection.disconnect() ;
}
*/
/*
TEST(debug, aggregate)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   vector<BSONObj> ob ;
   int iNUM = 5 ;
   int rNUM = 5 ;
   int i = 0 ;
   const char* command[iNUM] ;
   const char* record[rNUM] ;
   command[0] = "{\"$match\":{\"interest\":{\"$exists\":1}}}" ;
   command[1] = "{\"$group\":{\"_id\":\"$major\",\"avg_age\":{\"$avg\":\"$info.age\"},\"major\":{\"$first\":\"$major\"}}}" ;
   command[2] = "{\"$sort\":{\"avg_age\":-1,\"major\":1}}" ;
   command[3] = "{\"$skip\":0}" ;
   command[4] = "{\"$limit\":5}" ;

   record[0] = "{\"no\":1000,\"score\":80,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer th\",\"dep\":\"computer\",\"info\":{\"name\":\"tom\",\"age\":25,\"gender\":\"man\"}}" ;
   record[1] = "{\"no\":1001,\"score\":90,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer sc\",\"dep\":\"computer\",\"info\":{\"name\":\"mike\",\"age\":24,\"gender\":\"lady\"}}" ;
   record[2] = "{\"no\":1002,\"score\":85,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer en\",\"dep\":\"computer\",\"info\":{\"name\":\"kkk\",\"age\":25,\"gender\":\"man\"}}" ;
   record[3] = "{\"no\":1003,\"score\":92,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer en\",\"dep\":\"computer\",\"info\":{\"name\":\"mmm\",\"age\":25,\"gender\":\"man\"}}" ;
   record[4] = "{\"no\":1004,\"score\":88,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer sc\",\"dep\":\"computer\",\"info\":{\"name\":\"ttt\",\"age\":25,\"gender\":\"man\"}}" ;
   const char* m = "{$match:{status:\"A\"}}" ;
   const char* g = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;

   // insert record
   for( i=0; i<rNUM; i++ )
   {
      rc = fromjson( record[i], obj ) ;
      ASSERT_TRUE( rc == SDB_OK ) ;
      cout<<obj.toString()<<endl ;
      rc = cl.insert( obj ) ;
      ASSERT_TRUE( rc == SDB_OK ) ;
   }
   // build bson vector
   for ( i=0; i<iNUM; i++ )
   {
      rc = fromjson( command[i], obj ) ;
      ASSERT_TRUE( rc == SDB_OK ) ;
      cout<<obj.toString()<<endl ;
      ob.push_back( obj ) ;
   }
   // aggregate
   rc = cl.aggregate( cursor, ob ) ;
   cout<<"rc is "<<rc<<endl ;
   // display
   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}
*/

/*
TEST(debug, connect)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   // initialize local variables
   const CHAR* host                = "localhost" ;
   const CHAR* port                = "11810" ;
   const CHAR* username            = "" ;
   const CHAR* password            = "" ;
   const CHAR* user                = "" ;
   const CHAR* passwd              = "" ;
   INT32 rc                        = SDB_OK ;

   // connect to database
   rc = connection.connect( host, port, username, password ) ;
   ASSERT_TRUE( rc==SDB_OK ) << "Failed to connect to sdb, rc = " << rc ;
*/
/*
   // create user
   rc = connection.createUsr( user, passwd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;

   // drop user
   rc = connection.removeUsr( user, passwd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
*/
   // disconnect the connection
/*
   connection.disconnect() ;
}

*/

/*
TEST(debug, explain)
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR* host                = "192.168.20.181" ;
   const CHAR* port                = "11810" ;
   const CHAR* username            = "" ;
   const CHAR* password            = "" ;
   const CHAR* user                = "" ;
   const CHAR* passwd              = "" ;
   INT32 rc                        = SDB_OK ;

   // connect to database
   rc = db.connect( host, port, username, password ) ;
   ASSERT_TRUE( rc==SDB_OK ) << "Failed to connect to sdb, rc = " << rc ;

   // get cs
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   rc = cl.del() ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // insert records
   for ( INT32 i = 0; i < 100; i++ )
   {
      BSONObj obj = BSON ( "firstName" << "John" <<
                   "lastName" << "Smith" <<
                   "age" << i ) ; 
      rc = cl.insert( obj ) ;
      ASSERT_TRUE( rc==SDB_OK ) ;
   }
   // create index
   const CHAR *pIndex = "explain_index" ;
   BSONObj index = BSON( "age" << 1 ) ;
   rc = cl.createIndex( index, pIndex, FALSE, FALSE ) ;
   // test explain
   BSONObj condition = BSON( "age" << BSON( "$gt" << 50 ) ) ;
   BSONObj select    = BSON( "age" << "" ) ;
   BSONObj orderBy   = BSON( "age" << -1 ) ;
   BSONObj hint      = BSON( "" << pIndex ) ;

   BSONObjBuilder bob ;
   bob.appendBool( "Run", TRUE ) ;
   BSONObj option = bob.obj() ;  
   sdbCursor cursor ;
   rc = cl.explain( cursor, condition, select, orderBy, hint, 47, 3, 0, option ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   displayRecord( cursor ) ;  

   
   // disconnect the connection
   db.disconnect() ;
}

TEST(debug, queryone)
{

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   const CHAR *pIndex = "queryone_index" ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // connect to database
   sdbclient::sdb db ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get cs
   sdbclient::sdbCollectionSpace cs ;
   rc = getCollectionSpace( db, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // get collection
   sdbclient::sdbCollection cl ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   // delete data
   rc = cl.del() ;
   // create index
   BSONObj index = BSON( "age" << 1 ) ;
   rc = cl.createIndex( index, pIndex, FALSE, FALSE ) ;
   // insert records
   for ( INT32 i = 0; i < 200; i++ )
   {
      BSONObj obj = BSON ( "firstName" << "John" <<
                   "lastName" << "Smith" <<
                   "age" << i ) ;
      rc = cl.insert( obj ) ;
      ASSERT_TRUE( rc==SDB_OK ) ;
   }
   // define a cursor object for query
   sdbclient::sdbCursor cursor ;
   // build up the query condition
   BSONObj condition = BSON( "age" << BSON( "$gt" << 50 ) ) ;
   BSONObj select    = BSON( "age" << "" ) ;
   BSONObj orderBy   = BSON( "age" << -1 ) ;
   BSONObj hint      = BSON( "" << pIndex ) ;
   BSONObj record ;
   INT32 count = 0 ;
   // in case 1: general
   rc = cl.query( cursor, condition, select, orderBy, hint, 46, 3, 0 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout << "case 1: general dispaly: " << endl ;
//   displayRecord( cursor ) ;
   count = 0 ;
   while ( 0 == (rc = cursor.next(record)) )
   {
      count++ ;
   }
   cout << "count is: " << count << endl ;
   ASSERT_TRUE( 3 == count ) ;
   
   // in case 2: use flag
   rc = cl.query( cursor, condition, select, orderBy, hint, 46, 3, 0x00000200 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout << "case 2: use flag dispaly: " << endl ;
//   displayRecord( cursor ) ;
   count = 0 ;
   while ( 0 == (rc = cursor.next(record)) )
   {
      count++ ;
   }
   cout << "count is: " << count << endl ;
   ASSERT_TRUE( 3 == count ) ;
   

   // in case 3: return 1
   rc = cl.query( cursor, condition, select, orderBy, hint, 46, 1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout << "case 3: return 1 display: " << endl ;
//   displayRecord( cursor ) ;
   count = 0 ;
   while ( 0 == (rc = cursor.next(record)) )
   {
      count++ ;
   }
   cout << "count is: " << count << endl ;
   ASSERT_TRUE( 1 == count ) ;

   // in case 4: return 1
   rc = cl.query( cursor, condition, select, orderBy, hint, 0, -1, 0x00000200 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   cout << "case 4: return all, user flag display: " << endl ;
//   displayRecord( cursor ) ;
   count = 0 ;
   while ( 0 == (rc = cursor.next(record)) )
   {
      count++ ;
   }
   cout << "count is: " << count << endl ;
   ASSERT_TRUE( 149 == count ) ;


   // disconnect the connection
   db.disconnect() ;
}

*/

/*
TEST(debug, queryAndUpdate)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   const CHAR *pField1                      = "testField1" ;
   const CHAR *pField2                      = "testField2" ;
   const CHAR *pIndexName1                  = "testIndex1" ;
//   const CHAR *pIndexName2                  = "testIndex2" ;
   INT32 rc                                 = SDB_OK ;
   INT32 i                                  = 0 ;
   SINT64 count                             = 0 ;
   SINT64 NUM                               = 100 ;
   BSONObj update ;
   BSONObj condition ;
   BSONObj selector ;
   BSONObj orderBy ;
   BSONObj hint ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // create index
   BSONObj index = BSON( pField1 << 1 ) ;
   rc = cl.createIndex( index, pIndexName1, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // insert some record
   for ( i = 0; i < NUM; i++ )
   {
      BSONObj obj = BSON( pField1 << i << pField2 << i ) ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   
   update = BSON( "$set" << BSON( pField2 << 100 ) ) ;
   condition = BSON( pField1 << BSON( "$gte" << 0 ) ) ;
   selector = BSON( pField2 << "" ) ;
   orderBy = BSON( pField1 << -1 ) ;
   hint = BSON( "" << pIndexName1 ) ;

   // test
   rc = cl.queryAndUpdate( cursor, update, condition, selector,
                           orderBy, hint, 0, -1, 0, TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   BSONObj obj ;
   i = 0 ;
   while( SDB_OK == cursor.next( obj ) )
   {
      i++ ;
      BSONObjIterator it( obj ) ;
      BSONElement ele = it.next() ;
      INT32 num = ele.Int() ;
      ASSERT_EQ( 100, num ) ;
   }
   ASSERT_EQ( 100, i ) ;

   // disconnect the connection
   connection.disconnect() ;
}
*/

//TEST(debug, queryAndRemove)
//{
//   sdb connection ;
//   sdbCollectionSpace cs ;
//   sdbCollection cl ;
//   sdbCursor cursor ;
//   // initialize local variables
//   const CHAR *pHostName                    = HOST ;
//   const CHAR *pPort                        = SERVER ;
//   const CHAR *pUsr                         = USER ;
//   const CHAR *pPasswd                      = PASSWD ;
//   const CHAR *pField1                      = "testField1" ;
//   const CHAR *pField2                      = "testField2" ;
//   const CHAR *pIndexName1                  = "testIndex1" ;
//   const CHAR *pIndexName2                  = "testIndex2" ;
//   INT32 rc                                 = SDB_OK ;
//   INT32 i                                  = 0 ;
//   SINT64 count                             = 0 ;
//   SINT64 NUM                               = 100 ;
//   BSONObj condition ;
//   BSONObj selector ;
//   BSONObj orderBy ;
//   BSONObj hint ;
//   BSONObj hint2 ;
//
//   // initialize the work environment
//   rc = initEnv() ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // connect to database
//   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // get cs
//   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // get cl
//   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   
//   // create index
//   BSONObj index = BSON( pField1 << 1 ) ;
//   rc = cl.createIndex( index, pIndexName1, FALSE, FALSE ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   BSONObj index2 = BSON( pField2 << 1 ) ;
//   rc = cl.createIndex( index2, pIndexName2, FALSE, FALSE ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   
//   // insert some record
//   for ( i = 0; i < NUM; i++ )
//   {
//      BSONObj obj = BSON( pField1 << i << pField2 << i ) ;
//      rc = cl.insert( obj ) ;
//      ASSERT_EQ( SDB_OK, rc ) ;
//   }
//   
//   condition = BSON( pField1 << BSON( "$gte" << 0 ) ) ;
//   selector = BSON( pField2 << "" ) ;
//   orderBy = BSON( pField1 << -1 ) ;
//   hint = BSON( "" << pIndexName1 ) ;
//   hint2 = BSON( "" << pIndexName2 ) ;
//
//   BSONObj tmp ;
//
//   // test
//   // case 1: use extend sort
//   rc = cl.queryAndRemove( cursor, condition, selector,
//                           orderBy, hint2, 0, -1, 0 ) ;
//   ASSERT_EQ( SDB_RTN_QUERYMODIFY_SORT_NO_IDX, rc ) ;
//
//   // case 2: does not use extend sort
//   rc = cl.queryAndRemove( cursor, condition, selector,
//                           orderBy, hint, 0, -1, 0 ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // check
//   BSONObj obj ;
//   i = 0 ;
//   while( SDB_OK == cursor.next( obj ) )
//   {
//      i++ ;
//      BSONObjIterator it( obj ) ;
//      BSONElement ele = it.next() ;
//      INT32 num = ele.Int() ;
//      ASSERT_EQ( 100 - i, num ) ;
//   }
//   ASSERT_EQ( 100, i ) ;
//   i = 100 ;
//   while( i-- )
//   {
//      rc = cl.getCount( count ) ;
//      ASSERT_EQ( SDB_OK, rc ) ;
//      if ( 0 == count )
//         break ;
//   }
//   if ( 0 == i )
//   {
//      ASSERT_EQ( 0, count ) ;
//   }
//
//   // disconnect the connection
//   connection.disconnect() ;
//}
//

//TEST( debug, create_remove_id_index )
//{
//   sdb db ;
//   sdbCollectionSpace cs ;
//   sdbCollection cl ;
//   sdbCursor cursor ;
//   // initialize the work environment
//
//   const CHAR *pHostName                    = HOST ;
//   const CHAR *pPort                        = SERVER ;
//   const CHAR *pUsr                         = USER ;
//   const CHAR *pPasswd                      = PASSWD ;
//   const CHAR *pCLFullName                  = "" ;
//   const CHAR *pIndexName                   = "$id" ;
//   INT32 rc                                 = SDB_OK ;
//   INT32 count                              = 0 ;
//   BSONObj obj ;
//   BSONObj record ;
//   BSONObj updater ;
//
//   rc = initEnv() ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // connect to database
//   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   // get cs
//   rc = getCollectionSpace ( db, COLLECTION_SPACE_NAME, cs ) ;
//   // get cl
//   rc = getCollection ( cs, COLLECTION_NAME, cl ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // build record
//   obj = BSON( "a" << 1 ) ;
//   updater = BSON( "$set" << BSON( "a" << 2 ) ) ;
//
//   // insert into collection
//   rc = cl.insert( obj ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // remove $id index
//   rc = cl.dropIdIndex() ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // check
//   rc = cl.getIndexes( cursor, pIndexName ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   while( SDB_OK == (rc = cursor.next( record ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 0, count ) << "Index $id may not be drop" ;
//
//   rc = cl.query( cursor ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   
//   count = 0 ;
//   while( SDB_OK == (rc = cursor.next( record ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 1, count ) ;
//
//   rc = cl.update( updater ) ;
//   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;
//
//   rc = cl.upsert( updater ) ;
//   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;
//
//   rc = cl.del() ;
//   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;
//
//   // create $id index
//   rc = cl.createIdIndex() ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   rc = cl.del() ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   count = 0 ;
//   while( SDB_OK == (rc = cursor.next( record ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 0, count ) ;
//
//   obj = BSON( "$set" << BSON( "a" << 10 ) ) ;
//   rc = cl.upsert( obj ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   rc = cl.query( cursor ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   count = 0 ;
//   while( SDB_OK == (rc = cursor.next( record ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 1, count ) ;
//
//   rc = cl.update( updater ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   obj = BSON( "a" << 2 ) ;
//   rc = cl.query( cursor, obj ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   count = 0 ;
//   while( SDB_OK == (rc = cursor.next( record ) ) )
//   {
//      count++ ;
//   }
//   ASSERT_EQ( 1, count ) ;
//
//
//   // disconnect the connection
//   db.disconnect() ; 
//}
//
//
//TEST( debug, alter_collection )
//{
//   sdb db ;
//   sdbCollectionSpace cs ;
//   sdbCollection cl ;
//   sdbCursor cursor ;
//   // initialize the work environment
//
//   const CHAR *pHostName                    = HOST ;
//   const CHAR *pPort                        = SERVER ;
//   const CHAR *pUsr                         = USER ;
//   const CHAR *pPasswd                      = PASSWD ;
//   const CHAR *pCSName                      = "test_alter_cs" ;
//   const CHAR *pCLName                      = "test_alter_cl" ;
//   const CHAR *pCLFullName                  = "test_alter_cs.test_alter_cl" ;
//   const CHAR *pValue                       = NULL ;
//
//   INT32 rc                                 = SDB_OK ;
//   BSONObjBuilder bob ;
//   BSONObjBuilder bob2 ;
//   BSONElement ele ;
//   BSONObj option ;
//   BSONObj matcher ;
//   BSONObj record ;
//   BSONObj obj ;
//   INT32 n_value = 0 ;
//
//   bob.append( "Name", pCLFullName ) ;
//   matcher = bob.obj() ; 
// 
//   bob2.append( "ReplSize", 0 ) ;
//   bob2.append( "ShardingKey", BSON( "a" << 1 ) ) ;
//   bob2.append( "ShardingType", "hash" ) ;
//   bob2.append( "Partition", 1024 ) ;
//   option = bob2.obj() ;
//
//   rc = initEnv() ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//
//   // connect to database
//   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   if ( FALSE == isCluster( db ) )
//   {
//      return ;
//   }
//
//   // drop cs
//   rc = db.dropCollectionSpace( pCSName ) ;
//   if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
//   {
//      ASSERT_EQ( 0, 1 ) << "failed to drop cs " << pCSName ;
//   }
//
//   // create cs and cl
//   rc = db.createCollectionSpace( pCSName, 4096, cs ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   rc = cs.createCollection( pCLName, cl ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // alter
//   rc = cl.alterCollection( option ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   // check
//   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, matcher ) ;
//   ASSERT_EQ( SDB_OK, rc ) ; 
// 
//   rc = cursor.next( record ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//
//   ele = record.getField( "Name" ) ;
//   ASSERT_EQ( String, ele.type() ) << "bson element is not a string type" ;
//   pValue = ele.String().c_str() ;
//   ASSERT_EQ( 0, strcmp( pValue, pCLFullName ) ) << "after alter cl, the cl's name is not what we want" ;
//
//   ele = record.getField( "ReplSize" ) ;
//   n_value = ele.Int() ;
//   ASSERT_EQ( 7, n_value ) << "after alter cl, replSize is not 0" ;
//
//   ele = record.getField( "ShardingKey" ) ;
//   obj = ele.Obj() ;
//   ele = obj.getField( "a" ) ;
//   n_value = ele.Int() ;
//   ASSERT_EQ( 1, n_value ) << "after alter cl, the sharding key is not what we want" ;
//   
//   ele = record.getField( "ShardingType" ) ;
//   pValue = ele.String().c_str() ;
//   ASSERT_EQ( SDB_OK, strcmp( pValue, "hash") ) << "after alter cl, the sharding type is not what we want" ;
//   ele = record.getField( "Partition" ) ;
//   n_value = ele.Int() ;
//   ASSERT_EQ( 1024, n_value ) ;
//   
//   rc = db.dropCollectionSpace( pCSName ) ;
//   ASSERT_EQ( SDB_OK, rc ) ;
//   
//   db.disconnect() ;
//}
