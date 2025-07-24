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

   Source File Name = collection.cpp

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

#define INDEXNAMEDEF         "indexNameDef"

TEST(collection,getCount_without_condition)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   SINT64 count                             = 0 ;
   SINT64 NUM                               = 1000 ;

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
   // insert some record
   rc = insertRecords ( cl, NUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the count of record in currrent collection
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( NUM, count ) ;
   cout<<"The total number of records is "<<count<<endl ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getCount_with_condition)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   SINT64 count                             = 0 ;
   BSONObj condition1 ;
   BSONObj condition2 ;
   BSONObj obj ;

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
   // build up the condition1 that "age is 25"
   condition1 = BSON ( "age" << 50 ) ;
   // get the count of specified record in currrent collection
   rc = cl.getCount( count, condition1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Condition1 is"<<condition1<<endl ;
   cout<<"The number of matched records is "<<count<<endl ;
   // build up the condition2 that "age greater then 25"
   condition2 = BSON ( "age" << BSON ( "$gt" << 50 ) ) ;
   // get the count of specified record in currrent collection
   rc = cl.getCount( count, condition2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Condition2 is"<<condition2<<endl ;
   cout<<"The number of records is "<<count<<endl ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,bulkInsert)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   SINT64 NUM                               = 10000 ;
   SINT64 totalNum                          = 0 ;
   int count                                = 0 ;
   vector<BSONObj> objList ;
   BSONObj obj ;

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
   // count the total number of records before bulkInsert
   rc = cl.getCount (  totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("Before bulk insert, the total number \
of records is %lld\n",totalNum ) ;
   printf( "Bulk insert records." OSS_NEWLINE ) ;
   // allocate memory and add data
   for ( count = 0; count < NUM; count++ )
   {
      obj = BSON ( "firstName" << "John" <<
                   "lastName" << "Smith" <<
                   "age" << 50 ) ;
      objList.push_back ( obj ) ;
   }
   // bulk insert,if the argument "flags" is set FLG_INSERT_CONTONDUP,
   // datebase will not stop bulk insert while one failed with dup key
   rc = cl.bulkInsert( 0, objList ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // count the total number of records after insert
   rc = cl.getCount ( totalNum ) ;
   CHECK_MSG("%s%lld%s%lld%s%d\n", " NUM = ", NUM,
             " totalNum = ", totalNum, " rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( NUM, totalNum ) ;
   printf("After bulk insert,the total number \
of records is %lld\n",totalNum ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,bulkInsert_empty)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   vector<BSONObj> objList ;
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
   // count the total number of records before bulkInsert
   rc = cl.bulkInsert( 0, objList ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,insert_without_iterator)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   // initialize the work environment
   rc = initEnv() ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build up the record to insert
   createEnglishRecord( obj ) ;
   cout<<"the insert record is(notice the id):"<<endl;
   cout<<toJson(obj)<<endl;
   // insert into collection
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,insert_Chinese_record)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize the work environment

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, cs ) ;
   // get cl
   rc = getCollection ( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build up the Chinese record to insert
   createChineseRecord( obj ) ;
   cout<<"the insert Chinese record is:"<<endl;
   cout << obj.toString () << endl ;
   // insert into collection
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,update_condition_is_true)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONElement ele ;
   BSONObj rule ;
   BSONObj obj ;
   BSONObjBuilder ob ;
   BSONObj updateCondition ;

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
   // insert records
   rc = insertRecords( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   cl.query(cursor) ;
   rc = cursor.current( obj ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // pick up the updatecondition
   // for the use of updating
   ele = obj.getField ( "_id" ) ;
   ASSERT_TRUE( !ele.eoo() ) ;
   ob.append ( ele ) ;
   updateCondition = ob.obj () ;
   //set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"The update rule is:"<<endl;
   cout << rule.toString() << endl ;
   //update current record
   rc = cl.update( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,update_condition_is_false)
{

   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONElement ele ;
   BSONObj rule ;
   BSONObj updateCondition ;

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
   // set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"the update rule is:"<<endl;
   cout << rule.toString() << endl ;
   // set update condition
   // the condition doesn't exit in any record
   updateCondition = BSON ( "condition" << "the condition doesn't exist in any record" ) ;
   rc = cl.update( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,upsert_condition_is_true)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONElement ele ;
   BSONObj rule ;
   BSONObj obj ;
   BSONObj updateCondition ;
   BSONObjBuilder ob ;
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
   // insert records
   rc = insertRecords( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   sdbCursor cursor ;
      //first, need to query all the record
   cl.query(cursor) ;
      //second, get the current one
   rc = cursor.current(obj) ;
   ASSERT_EQ( SDB_OK, rc ) ;
      //then pick up the updatecondition
      //for the use of updating
   ele = obj.getField ( "_id" ) ;
   ASSERT_TRUE( !ele.eoo() ) ;
   ob.append ( ele ) ;
   updateCondition = ob.obj () ;
      //set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 100 ) ) ;
   cout<<"the update rule is:"<<endl;
   cout << rule.toString() << endl ;
   //update current record
   rc = cl.upsert( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,upsert_condition_is_false)
{

   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONElement ele ;
   BSONObj rule ;
   BSONObj obj ;
   BSONObj updateCondition ;

   // initialize the work environment
   rc = initEnv() ;
   // connect to database

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"the update rule is:"<<endl;
   cout << rule.toString() << endl ;
   // set update condition
   // the condition doesn't exit in any record
   updateCondition = BSON ( "condition" << "the condition doesn't exist in any record" ) ;
   rc = cl.upsert( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,del_without_condition)
{

   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;

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
   // set delete all the record in current collection
   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,del_with_condition)
{

   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONObj condition ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build up the delete condition
   condition  = BSON ( "age" << 50 ) ;
   // delete the specified records in current collection
   rc = cl.del( condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,query)
{

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condition ;
   // connect to database
   sdbclient::sdb connection ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   sdbclient::sdbCollectionSpace cs ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   sdbclient::sdbCollection cl ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // define a cursor object for query
   sdbclient::sdbCursor cursor ;
   // build up the query condition
   condition = BSON ( "age" << 50 ) ;
   // query the specified records in current collection
   rc = cl.query( cursor, condition  ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,createIndex)
{
   sdb connection(TRUE) ;
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

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // build a bson for index definition
   obj = BSON ( "name" << 1 << "age" << -1 ) ;
   // create index
   rc = cl.createIndex( obj, INDEXNAMEDEF, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get the newly build index
   rc = cl.getIndexes( cursor, INDEX_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "After creating index ,the current index is:\n" ) ;
   cout << obj.toString() << endl ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getIndexes)
{
   sdb connection(TRUE) ;
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

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get the index
   rc = cl.getIndexes( cursor, "$id" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "The current index we get is:\n" ) ;
   cout << obj.toString() << endl ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,dropIndex)
{
   sdb connection(TRUE) ;
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

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   obj = BSON ( "name" << 1 << "age" << -1 ) ;
   // build a bson for index definition
   // create index
   rc = cl.createIndex( obj, INDEXNAMEDEF, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop index
   rc = cl.dropIndex( INDEX_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the index again
   rc = cl.getIndexes( cursor, INDEX_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getCollectionName)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get the collection name
   clName = cl.getCollectionName() ;
   // print the cl name
   cout<<"The cl name is ："<<clName<<endl ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getCSName)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *csName                       = NULL ;
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

   // get the collection name
   csName = cl.getCSName() ;
   // print the cl name
   cout<<"The cs name is ："<<csName<<endl ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getFullName)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *fullName                       = NULL ;
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

   // get the collection name
   fullName = cl.getFullName() ;
   // print the cl name
   cout<<"The full name is ："<<fullName<<endl ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,aggregate)
{
   sdb connection(TRUE) ;
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
   int iNUM = 2 ;
   int rNUM = 4 ;
   int i = 0 ;
   const char* command[iNUM] ;
   const char* record[rNUM] ;
   command[0] = "{$match:{status:\"A\"}}" ;
   command[1] = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;
   record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}" ;
   record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}" ;
   record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}" ;
   record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}" ;
   const char* m = "{$match:{status:\"A\"}}" ;
   const char* g = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;

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

   // insert record
   for( i=0; i<rNUM; i++ )
   {
      rc = fromjson( record[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<obj.toString()<<endl ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   // build bson vector
   for ( i=0; i<iNUM; i++ )
   {
      rc = fromjson( command[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
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

TEST(collection, aggregate_2)
{
   sdb connection(TRUE) ;
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

   // insert record
   for( i=0; i<rNUM; i++ )
   {
      rc = fromjson( record[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<obj.toString()<<endl ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   // build bson vector
   for ( i=0; i<iNUM; i++ )
   {
      rc = fromjson( command[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
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


TEST( collection, getQueryMeta )
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor datacursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   BSONObj temp ;
   BSONObj empty ;
   BSONObj condition ;
   BSONObj orderBy ;
   BSONObj hint ;
   string st ;
   const char* st1 = "tbscan" ;
   const char* st2 = "ixscan" ;
   long i = 0 ;

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
   // condition
   condition = BSON( "age"<<BSON("$gt" << 0)<<"age"<<BSON( "$lt" << 100 ) ) ;
   cout<<"condition is: "<<condition.toString()<<endl ;
   // hint
   int flag = 0 ;
   if( 0 == flag )
   {
      hint = BSON( "" << "ageIndex" ) ;
   }
   else
   {
      BSONObjBuilder ob1 ;
      ob1.appendNull ( "" ) ;
      hint = ob1.obj() ;
   }
   // orderBy
   orderBy = BSON( "Indexblocks"<<1 ) ;
   // TO DO:
   rc = cl.getQueryMeta( cursor, condition, empty, hint, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   while ( !( rc = cursor.next( obj ) ) )
   {
      cout<<obj.toString()<<endl ;
      BSONObjBuilder bo ;
      obj.getField( "ScanType" ).Val( st ) ;
      cout<<"ScanType is: "<<st.c_str()<<endl ;
      if ( !st.compare( st1 ) )
      {
         bo.appendAs( obj.getField( "Datablocks" ), "Datablocks" ) ;
      }
      else if ( !st.compare( st2 ) )
      {
         bo.appendAs( obj.getField( "Indexblocks" ), "Indexblocks" ) ;
      }
      else
      {
         cout<<"the \"ScanType\" is "<<st.c_str()
             <<",not \"tbscan\" or \"ixscan\"."<<endl ;
         ASSERT_EQ( 1, 0 ) ;
      }
      BSONObj hint = bo.obj() ;
      cout<<"hint is: "<<hint.toString()<<endl ;
      rc = cl.query( datacursor, empty, empty, empty, hint, 0, -1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      while ( !( rc = datacursor.next( temp ) ) )
      {
//         cout<<temp.toString()<<endl ;
         i++ ;
      }
   }
   cout<<"the record number is: "<<i<<endl ;
   ASSERT_EQ( 1, 1 ) ;
   connection.disconnect() ;

}

TEST( collection, getQueryMeta_select_is_null )
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor datacursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   BSONObj temp ;
   BSONObj empty ;
   BSONObj condition ;
   BSONObj orderBy ;
   BSONObj hint ;
   BSONObjBuilder ob1 ;
   string st ;
   const char* st1 = "tbscan" ;
   const char* st2 = "ixscan" ;
   long i = 0 ;

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
   // build condition
   condition = BSON( "age"<<BSON("$gt" << 50) ) ;
   cout<<"condition is: "<<condition.toString()<<endl ;
   // select
   int flag = 1 ;
   if( 0 == flag )
   {
      hint = BSON( "" << "ageIndex" ) ;
   }
   else
   {
      ob1.appendNull ( "" ) ;
      hint = ob1.obj() ;
   }
   // orderBy
   orderBy = BSON( "Datablocks"<<1 ) ;
   // TO DO:
   rc = cl.getQueryMeta( cursor, condition, empty, hint, 0, -1 ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   while ( !( rc = cursor.next( obj ) ) )
   {
      cout<<obj.toString()<<endl ;
      BSONObjBuilder bo ;
      obj.getField( "ScanType" ).Val( st ) ;
      cout<<"ScanType is: "<<st.c_str()<<endl ;
      if ( !st.compare( st1 ) )
      {
         bo.appendAs( obj.getField( "Datablocks" ), "Datablocks" ) ;
      }
      else if ( !st.compare( st2 ) )
      {
         bo.appendAs( obj.getField( "Indexblocks" ), "Indexblocks" ) ;
      }
      else
      {
         cout<<"the \"ScanType\" is "<<st.c_str()
             <<",not \"tbscan\" or \"ixscan\"."<<endl ;
         ASSERT_TRUE( 1 == 0 ) ;
      }
      BSONObj hint = bo.obj() ;
      cout<<"hint is: "<<hint.toString()<<endl ;
      rc = cl.query( datacursor, empty, empty, empty, hint, 0, -1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      while ( !( rc = datacursor.next( temp ) ) )
      {
//         cout<<temp.toString()<<endl ;
         i++ ;
      }
   }
   cout<<"the record number is: "<<i<<endl ;
   ASSERT_TRUE( 1==1 ) ;
   connection.disconnect() ;

}

TEST(collection, attachCollection)
{
   sdb connection(TRUE) ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   INT32 NUM                                = 100 ;
   INT32 i                                  = 0 ;
   SINT64 count                             = 0 ;
   BSONObj obj ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // test whether it is in the cluser environment, because
   // 'SDB_SNAP_CATALOG' could not use in standalone
   rc = connection.getList( cursor, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "attachCollection use \
in the cluser environment only" << endl ;
      ASSERT_EQ( SDB_RTN_COORD_ONLY, rc );
      return ;
   }
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create main cl
   rc = cs.createCollection( "main",
                             BSON("IsMainCL"<<true<<"ShardingKey"<<BSON("id"<<1)),
                             cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // attach the sub collection "testbar"
   obj =  BSON( "LowBound" << BSON("id"<<0) << "UpBound" << BSON("id"<<100) ) ;
   rc = cl.attachCollection ( COLLECTION_FULL_NAME, obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert some data for test
   for ( i=0; i < NUM; i++ )
   {
      rc = cl.insert(BSON("id"<<i)) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   INT32 cnt = 0 ;
   do
   {
      rc = cl.getCount ( count ) ;
      ++cnt ;
   }while( 100 != count && 100 > cnt ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "NUM is: " << NUM << "count is: " << count << endl ;
   ASSERT_EQ( NUM, count ) ;
   // detach
   rc = cl.detachCollection ( COLLECTION_FULL_NAME ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // after detach, use a invalid record to test again
   rc = cl.insert(BSON("id"<<101)) ;
   cout << "rc is: " << rc << endl ;
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}



/* some doubtful test */
/*
TEST(collection,insert_with_iterator)
{
   sdb connection(TRUE) ;
   sdbCollection collection ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONElement ele ;
   BSONObj obj ;
   const char *key                          = NULL ;
   const char *val                          = NULL ;
   int value                                = 5 ;

   // initialize the work environment
   initEnv() ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( connection, COLLECTION_FULL_NAME, collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build the record to insert
   obj = BSON ( "a" << 1 ) ;
   cout<<"The insert record is :"<<endl;
   cout << obj.toString() << endl ;
   // insert into collection
   rc = collection.insert( obj, &ele ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Read the record by the iterator:"<<endl;
//   bson_iterator_next( &it ) ;
   //key = bson_iterator_key( &it ) ;
   //val = bson_iterator_string( &it ) ;
//   value = bson_iterator_int( &it ) ;
   //printf("The insert record is {%s:%d}\n", key, value ) ;
   //printf("The insert record is {%s:%s}\n", key, val ) ;
//   cout<<"{"<<key<<":"
  //     <<value<<"}"<<endl ;

   //ASSERT_EQ( 1, 0 ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,create)
{
   ASSERT_TRUE( 0==0) ;
}

TEST(collection,drop)
{
   ASSERT_TRUE( 0==0) ;
}

*/

/*******************************************************************************
*@Description : query one testcase.[db.foo.bar.findOne()]
*@Modify List :
*               2014-10-24   xiaojun Hu   Init
*******************************************************************************/
TEST( collection, sdbCppQueryOne )
{
/*
   INT32 rc = SDB_OK ;

   sdbclient::sdb db ;
   rc = db.connect( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
*/
}
