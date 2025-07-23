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
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

// global

void selectCreateCollection( sdb &db, sdbCollection *cl, const CHAR *clName )
{
   sdbCollectionSpace cs ;
   INT32 rc = SDB_OK ;
   const CHAR *csName = "select_query_cs" ;
   //const CHAR *clName = "select_query_cl" ;
   const CHAR *hostName = HOST ;
   const CHAR *svcPort = SERVER ;
   const CHAR *usr = USER ;
   const CHAR *passwd = PASSWD ;

   rc = db.connect( hostName, svcPort, usr, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //cout << "Host: " << hostName << endl ;
   //cout << "svcPort: " << svcPort << endl ;
   rc = db.createCollectionSpace( csName, 4096, cs ) ;
   if( SDB_DMS_CS_EXIST == rc )
   {
      rc = db.getCollectionSpace( csName, cs ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj optionObj = BSON( "ReplSize" << 0 ) ;
   //cout << "option object: " << optionObj.toString() << endl ;
   rc = cs.createCollection( clName, optionObj, *cl ) ;
   if( SDB_DMS_EXIST == rc )
   {
      rc = cs.dropCollection( clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = cs.createCollection( clName, *cl ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) ;
}

// selector: $elemMatch
TEST( select, elementMatch )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_elementMatch" ;

   selectCreateCollection( db, &cl, clName ) ;
   // { Group: [{"GroupInfo":[ { "GroupName":"group1", "SvcType": 1000, "Name": 41000},...]}]}
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // $elementMatch: {"Group.GroupInfo":{"$elemMatch":{"Name":41000}}}
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo" << BSON( "$elemMatch" <<
                     BSON( "Name" << 41000 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

// selector: $elemMatchOne
TEST( select, elementMatchOne )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_elementMatchOne" ;

   selectCreateCollection( db, &cl, clName ) ;
   // { Group: [{"GroupInfo":[ { "GroupName":"group1", "SvcType": 1000, "Name": 41000},...]}]}
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // $elementMatch: {"Group.GroupInfo":{"$elemMatch":{"Name":41000}}}
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo" << BSON( "$elemMatchOne" <<
                     BSON( "Name" << 41000 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

// selector: $slice
TEST( select, slice )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_slice" ;

   selectCreateCollection( db, &cl, clName ) ;
   // { Group:["rg1", "rg2", "rg3", "rg4", "rg5", "rg6", "rg7", "rg8"]}
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( "rg1" << "rg2" << "rg3" <<
               "rg4" << "rg5" << "rg6" << "rg7" << "rg8" ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // $elementMatch: {"Group.GroupInfo":{"$elemMatch":{"Name":41000}}}
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group" << BSON( "$slice" << BSON_ARRAY( -7 << 3 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

// selector: $default
TEST( select, _default )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_default" ;

   selectCreateCollection( db, &cl, clName ) ;
   // { Group: [{"GroupInfo":[ { "GroupName":"group1", "SvcType": 1000, "Name": 41000},...]}]}
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // $elementMatch: {"Group.GroupInfo":{"$elemMatch":{"Name":41000}}}
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo.GroupName" << BSON( "$default" <<
                     BSON( "Name" << 41000 ) ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

// selector: $include
TEST( select, include )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_include" ;

   selectCreateCollection( db, &cl, clName ) ;
   // { Group: [{"GroupInfo":[ { "GroupName":"group1", "SvcType": 1000, "Name": 41000},...]}]}
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // $elementMatch: {"Group.GroupInfo":{"$elemMatch":{"Name":41000}}}
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo.GroupName" << BSON( "$include" << 0 ) ) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor1, condObj, selectObj ) ;
   BSONObj queryObj ;
   INT32 cnt = 0 ;
   while( SDB_DMS_EOC != cursor.next( queryObj ) )
   {
      cout << "<Return Record>:\n" << queryObj.toString() << endl ;
      cnt++ ;
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "success to test" << endl ;
}

// selector: abnormal
TEST( select, includeAbnormal )
{
   sdb db ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor cursor1 ;
   INT32 rc = SDB_OK ;
   const CHAR *clName = "select_query_includeAbnormal" ;

   selectCreateCollection( db, &cl, clName ) ;
   // { Group: [{"GroupInfo":[ { "GroupName":"group1", "SvcType": 1000, "Name": 41000},...]}]}
   BSONObj obj ;
   obj = BSON( "Group" << BSON_ARRAY( BSON( "GroupInfo" << BSON_ARRAY( BSON(
               "GroupName" << "group1" << "SvcType" << 1000 << "Name" <<
               41000 ) << BSON( "GroupName" << "group2" << "SvcType" << 1001 <<
               "Name" << 42000 ) << BSON( "GroupName" << "group3" << "SvcType" <<
               1002 << "Name" << 43000 ) << BSON( "GroupName" << "group4" <<
               "SvcType" << 1003 << "Name" << 41000 ) ) ) ) ) ;
   cout << "<Insert Record>:\n" << obj.toString() << endl ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // $elementMatch: {"Group.GroupInfo":{"$elemMatch":{"Name":41000}}}
   BSONObj condObj ;
   BSONObj selectObj ;
   selectObj = BSON( "Group.GroupInfo.GroupName" << BSON( "$include" << 1 ) <<
                     "Group.GroupInfo.GroupName" << BSON( "$include" << 1 ) <<
                     "Group.GroupInfo.GroupName" << BSON( "$include" << 0 )) ;
   cout << "<Query Conditions>:\n" << selectObj.toString() << endl ;
   rc = cl.query( cursor, condObj, selectObj ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   cout << "success to test, throw -6[Invalid Arguement]" << endl ;
}
