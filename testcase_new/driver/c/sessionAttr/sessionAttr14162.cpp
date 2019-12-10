/**************************************************************
 * @Description : test case of sessionAttr
 * seqDB-14162  : getSessionAttr()获取驱动端缓存信息
 * @Modify      : Liang xuewang
 *                2018-02-12
 **************************************************************/
#include <client.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <stdio.h>
#include "testcommon.hpp"

class sessionAttrTest14162 : public testing::Test 
{
protected:
   sdbConnectionHandle db ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      getConf() ;
      rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ; 
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   void TearDown()
   {
      sdbDisconnect( db ) ;
      sdbReleaseConnection( db ) ;
   }
} ;

TEST_F( sessionAttrTest14162, cache )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }  
   ASSERT_EQ( SDB_OK, rc ) ;

   bson result ;
   bson_init( &result ) ;
   rc = sdbGetSessionAttr( db, &result ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   bson_iterator it ;
   bson_find( &it, &result, "Timeout" ) ;
   ASSERT_EQ( -1, bson_iterator_int( &it ) ) ;
   bson_destroy( &result ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "Timeout", 1000 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   bson_init( &result ) ;
   rc = sdbGetSessionAttr( db, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   bson_find( &it, &result, "Timeout" ) ;
   ASSERT_EQ( 1000, bson_iterator_int( &it ) ) ;
   bson_destroy( &result ) ;

   bson_init( &result ) ;
   rc = sdbGetSessionAttr( db, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr again" ;
   bson_find( &it, &result, "Timeout" ) ;
   ASSERT_EQ( 1000, bson_iterator_int( &it ) ) ;
   bson_destroy( &result ) ;
}
