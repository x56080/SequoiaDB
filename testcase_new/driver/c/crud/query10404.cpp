/***************************************************
 * @Description : test insert/update/delete/query 
 * @Modify      : liu xiaoxuan 
 *                seqDB-10404
 *                2019-10-09
 ***************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"

class query10404 : public testing::Test
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   sdbConnectionHandle db ;

   void SetUp()  
   {
      csName = "query10404" ;
      clName = "query10404" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      db = SDB_INVALID_HANDLE ;
      INT32 rc = SDB_OK ;
      rc = createNormalCl( &db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      sdbDisconnect( db ) ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      sdbReleaseConnection( db ) ;
   }
} ;

TEST_F( query10404, query )
{
   INT32 rc = SDB_OK ;
  
   bson* docs[5] ; 
   for( INT32 i = 0; i < 5; i++ )
   {
       docs[i] = bson_create() ;
       bson_append_int( docs[i], "a", i ) ;
       bson_finish( docs[i] ) ;
   }
   rc = sdbBulkInsert( cl, 0, docs, 5 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   for( INT32 i = 0; i < 5; i++ )
   {
      bson_dispose( docs[i] ) ;
   }

   sdbCursorHandle cursor ;
   // QUERY_EXPLAIN: 1024
   bson orderby ;
   bson_init ( &orderby ) ;
   bson_append_int( &orderby, "a", 1 ) ;
   bson_finish( &orderby ) ;
   rc = sdbQuery1( cl, NULL, NULL, &orderby, NULL, 0, 2, 1024, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check result
   bson ret ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_iterator it ;
   bson_find( &it, &ret, "a" ) ;
   ASSERT_EQ( 0, bson_iterator_int(&it) ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_find( &it, &ret, "a" ) ;
   ASSERT_EQ( 1, bson_iterator_int(&it) ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_DMS_EOC ) ;
   bson_destroy( &ret ) ;
   sdbCloseCursor( cursor ) ; 
   sdbReleaseCursor( cursor ) ;

   // update
   bson rule ;
   bson cond ;
   bson_init( &rule ) ;
   bson_init( &cond ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "a", -1 ) ;
   bson_append_finish_object( &rule ) ;
   bson_append_int ( &cond, "a", 0 ) ;
   bson_finish( &rule ) ;
   bson_finish( &cond ) ;
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_destroy( &rule ) ;
   bson_destroy( &cond ) ;

   // check result
   rc = sdbQuery1( cl, NULL, NULL, &orderby, NULL, 0, 1, 1024, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_find( &it, &ret, "a" ) ;
   ASSERT_EQ( -1, bson_iterator_int(&it) ) ;
   bson_destroy( &ret ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;

   // remove 
   bson_init( &cond ) ;
   bson_append_int ( &cond, "a", -1 ) ;
   bson_finish( &cond ) ; 
   rc = sdbDelete( cl, &cond, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &cond ) ;

   // check result
   rc = sdbQuery1( cl, NULL, NULL, &orderby, NULL, 0, 1, 1024, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_find( &it, &ret, "a" ) ;
   ASSERT_EQ( 1, bson_iterator_int(&it) ) ;
   bson_destroy( &orderby ) ;
   bson_destroy( &ret ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;
}
