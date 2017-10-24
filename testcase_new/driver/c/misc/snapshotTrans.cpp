/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2316
 *               seqDB-13059:获取事务快照
 * @Modify:      Liang xuewang Init
 *			 	     2017-10-24
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"

class snapshotTransTest : public testing::Test
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;
      csName = "snapshotTransTestCs" ;
      clName = "snapshotTransTestCl" ;
      rc = createNormalCl( &db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
   }
} ;

// SDB_SNAP_TRANSACTIONS 9
TEST_F( snapshotTransTest, SDB_SNAP_TRANSACTIONS )
{
   INT32 rc = SDB_OK ;

   // begin trans
   rc = sdbTransactionBegin( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to begin trans" ;

   // insert doc
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 1 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   bson_destroy( &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // snapshot trans
   sdbCursorHandle cursor ;
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot trans" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   ASSERT_NE( BSON_EOO, bson_find( &it, &obj, "TransactionID" ) ) ;
   // bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // commit trans
   rc = sdbTransactionCommit( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to commit trans" ;

   // snapshot trans
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot trans" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to get next" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( snapshotTransTest, SDB_SNAP_TRANSACTIONS_CURRENT )
{
   INT32 rc = SDB_OK ;

   // begin trans
   rc = sdbTransactionBegin( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to begin trans" ;

   // insert doc
   bson doc ;
   bson_init( &doc ) ;
   bson_append_int( &doc, "a", 1 ) ;
   bson_finish( &doc ) ;
   rc = sdbInsert( cl, &doc ) ;
   bson_destroy( &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // snapshot trans
   sdbCursorHandle cursor ;
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS_CURRENT, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot trans" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   ASSERT_NE( BSON_EOO, bson_find( &it, &obj, "TransactionID" ) ) ;
   // bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // commit trans
   rc = sdbTransactionCommit( db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to commit trans" ;

   // snapshot trans
   rc = sdbGetSnapshot( db, SDB_SNAP_TRANSACTIONS_CURRENT, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot trans" ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to get next" ;
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
