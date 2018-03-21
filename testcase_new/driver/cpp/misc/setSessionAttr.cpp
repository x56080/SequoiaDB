/******************************************************************
 * @Description : test setSessionAttr and check
 *                SEQUOIADBMAINSTREAM-3251
 *                test compatibility between 2.8.4 and 2.8.5 2.9
 *                don't run in ci
 * @Author      : Liang xuewang
 *                2018-02-11
 ******************************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "testcommon.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace bson ;

class setSessionAttrTest : public testing::Test
{
protected:
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* host ;
   const CHAR* svc ;
   const CHAR* user ;
   const CHAR* passwd ;

   void SetUp()
   {
      csName = "setSessionAttrTestCs" ;
      clName = "setSessionAttrTestCl" ;
      host = "192.168.31.57" ;
      svc = "11810" ;
      user = "" ;
      passwd = "" ;
      INT32 rc = SDB_OK ;
      rc = db.connect( host, svc, user, passwd ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      BSONObj option = BSON( "Group" << "group1" << "ReplSize" << 0 ) ;
      rc = cs.createCollection( clName, option, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = db.dropCollectionSpace( csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      db.disconnect() ;
   }
} ;

TEST_F( setSessionAttrTest, attrChar )
{
   INT32 rc = SDB_OK ;

   const CHAR* attrs[] = { "M", "S", "A", "m", "s", "a" } ;
   INT32 size = sizeof(attrs) / sizeof(attrs[0]) ;
   for( INT32 i = 0;i < size;i++ )
   {
      cout << "PreferedInstance: " << attrs[i] << endl ;
      BSONObj option = BSON( "PreferedInstance" << attrs[i] ) ;
      rc = db.setSessionAttr( option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
      sdbCursor cursor ;
      rc = cl.explain( cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to explain" ;
      BSONObj obj ;
      rc = cursor.next( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
      cout << "Node: " << obj.getField( "NodeName" ).String() << endl ;
      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   }
}

TEST_F( setSessionAttrTest, attrInt )
{
   INT32 rc = SDB_OK ;

   const INT32 attrs[] = { 1, 2, 3, 8, 9, 10 } ;
   INT32 size = sizeof(attrs) / sizeof(attrs[0]) ;
   for( INT32 i = 0;i < size;i++ )
   {
      cout << "PreferedInstance: " << attrs[i] << endl ;
      BSONObj option = BSON( "PreferedInstance" << attrs[i] ) ;
      rc = db.setSessionAttr( option ) ;
      if( attrs[i] > 7 )
      {
         ASSERT_EQ( SDB_INVALIDARG, rc ) ;
      }
      else
      {
         ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
         sdbCursor cursor ;
         rc = cl.explain( cursor ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to explain" ;
         BSONObj obj ;
         rc = cursor.next( obj ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
         cout << "Node: " << obj.getField( "NodeName" ).String() << endl ;
         rc = cursor.close() ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
      }
   }
}

TEST_F( setSessionAttrTest, attrArr )
{
   INT32 rc = SDB_OK ;

   cout << "PreferedInstance: [ 1, 2 ]" << endl ;
   BSONObj option = BSON( "PreferedInstance" << BSON_ARRAY( 1 << 2 ) ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to setSessionAttr" ;
}
