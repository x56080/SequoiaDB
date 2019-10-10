/**************************************************************************
 * @Description :   test sdbGetList
 * @Modify      :   liuxiaoxuan
 * @testlink    :   seqDB-19948
 *                  2019-10-10
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"

class list19948 : public testing::Test
{
protected:
   sdbConnectionHandle db ;
   const CHAR* tmpUsr1 ;
   const CHAR* tmpPasswd1 ;
   const CHAR* tmpUsr2 ;
   const CHAR* tmpPasswd2 ;
   const CHAR* tmpUsr3 ;
   const CHAR* tmpPasswd3 ; 

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      getConf() ;
      rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      if( isStandalone( db ) )
      {
         printf( "Run mode is standalone\n" ) ;
         return ;
      }

      // create user
      tmpUsr1 = "test1" ;
      tmpPasswd1 = "test1" ;
      tmpUsr2 = "test2" ;
      tmpPasswd2 = "test2" ;
      tmpUsr3 = "test3" ;
      tmpPasswd3 = "test3" ;
      rc = sdbCreateUsr( db, tmpUsr1, tmpPasswd1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbCreateUsr( db, tmpUsr2, tmpPasswd2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbCreateUsr( db, tmpUsr3, tmpPasswd3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;

      if( !isStandalone( db ) )
      {
          // remove user
          rc = sdbRemoveUsr( db, tmpUsr1, tmpPasswd1 ) ;
          ASSERT_EQ( SDB_OK, rc ) ;
          rc = sdbRemoveUsr( db, tmpUsr2, tmpPasswd2 ) ;
          ASSERT_EQ( SDB_OK, rc ) ;
          rc = sdbRemoveUsr( db, tmpUsr3, tmpPasswd3 ) ;
          ASSERT_EQ( SDB_OK, rc ) ;
      }
      sdbDisconnect( db ) ;
      sdbReleaseConnection( db ) ;
   }
} ;

TEST_F( list19948, list_user )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // get list
   INT32 listType = SDB_LIST_USERS ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson cond ;
   bson selector ;
   bson orderby ;
   bson hint ;
   bson_init( &cond ) ;
   bson_init( &selector ) ;
   bson_init( &orderby ) ;
   bson_init( &hint ) ;
   bson_append_start_object( &cond, "User" ) ;
   bson_append_int( &cond, "$isnull", 0 ) ;
   bson_append_finish_object( &cond ) ;
   bson_append_string( &selector, "User", "" ) ;
   bson_append_int( &orderby, "User", 1 ) ;
   bson_finish( &cond );
   bson_finish( &selector );
   bson_finish( &orderby );
   bson_finish( &hint );
   rc = sdbGetList1( db, listType, &cond, &selector, &orderby, &hint, 1, 2, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &cond );
   bson_destroy( &selector );
   bson_destroy( &orderby );
   bson_destroy( &hint );
 
   // check result
   bson ret ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_iterator it ;
   bson_find( &it, &ret, "User" ) ;
   ASSERT_STREQ( "test2", bson_iterator_string( &it ) ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_find( &it, &ret, "User" ) ;
   ASSERT_STREQ( "test3", bson_iterator_string( &it ) ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_DMS_EOC ) ;
   bson_destroy( &ret ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;
}
