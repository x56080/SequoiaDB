/*
 * @Description   : seqDB-():C驱动功能测试
 * @Author        : Cheng jingjing
 * @CreateTime    : 2022.11.30
 * @LastEditTime  : 2022.11.30
 * @LastEditors   : Cheng jingjing
 */
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class getCollectionStat1: public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* indexName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "getCollectionStat1" ;
      clName = "getCollectionStat1" ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;

      // create cs cl
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = sdbCreateCollection( cs, clName, &cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

      // insert data
      bson* docs[10] ;
      for( INT32 i = 0; i < 10; i++ )
      {
         docs[i] = bson_create() ;
         rc = bson_append_int( docs[i], "a", i ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
         rc = bson_finish( docs[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) ;
      }
      rc = sdbBulkInsert( cl, 0, docs, 10 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      for( INT32 i = 0; i < 10; i++ )
      {
         bson_dispose( docs[i] ) ;
      }
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( getCollectionStat1, getCollectionStat )
{
   INT32 rc = SDB_OK ;
   // analyze
   CHAR clFullName[ 2 * MAX_NAME_SIZE + 2 ] = { 0 } ;
   sprintf( clFullName, "%s%s%s", csName, ".", clName ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "Collection", clFullName ) ;
   bson_finish( &option ) ;
   rc = sdbAnalyze( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &option ) ;

   // get cl statistics
   bson res ;
   bson_init( &res ) ;
   rc = sdbCLGetCollectionStat( cl, &res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // bson_print( &res ) ;
   bson_iterator it ;
   bson_find( &it, &res, "Collection") ;
   ASSERT_EQ( 0, strncmp( clFullName, bson_iterator_string( &it ), strlen( bson_iterator_string( &it ) ) ) ) ;
   bson_find( &it, &res, "IsDefault" ) ;
   ASSERT_EQ( FALSE, bson_iterator_bool( &it ) ) ;
   bson_find( &it, &res, "IsExpired" ) ;
   ASSERT_EQ( FALSE, bson_iterator_bool( &it ) ) ;
   bson_find( &it, &res, "AvgNumFields" ) ;
   ASSERT_EQ( 10, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "SampleRecords" ) ;
   ASSERT_EQ( 10, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "TotalRecords" ) ;
   ASSERT_EQ( 10, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "TotalDataPages" ) ;
   ASSERT_EQ( 1, bson_iterator_long( &it ) ) ;
   bson_find( &it, &res, "TotalDataSize" ) ;
   ASSERT_EQ( 290, bson_iterator_long( &it ) ) ;

   bson_destroy( &res ) ;
}
