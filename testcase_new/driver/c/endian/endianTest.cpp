/****************************************************************************
* @Description: test case for c driver  
*				( manual test case,not in CI )
*				test send and recv msg between big-endian and little-endian
* @Modify:		Liang xuewang Init
*				2016-11-10
*****************************************************************************/
#include <gtest/gtest.h>
#include <client.h>

const char* HostName = "192.168.28.2" ;  // ppc--big endian  x86--little endian
const char* SvcName = "11810" ;

TEST( EndianTest, BigAndLittle )
{
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   	sdbCSHandle cs = SDB_INVALID_HANDLE ;
   	sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   	const char* CsName = "EndianTestCs" ;
   	const char* ClName = "EndianTestCl" ;
   	int rc = SDB_OK ;
   
   	// connect to sdb
   	rc = sdbConnect( HostName, SvcName, USER, PASSWD, &db ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
   	// create cs
   	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create cs " << CsName ;
	// create cl 
	rc = sdbCreateCollection( cs, ClName, &cl ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create cl " << ClName ;
	
	// insert record { a: 1 }
	bson obj ;
	bson_init( &obj ) ;
	bson_append_int( &obj, "a", 1 ) ;
	bson_finish( &obj ) ;
	rc = sdbInsert( cl, &obj ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to insert record { a: 1 }" ;
	
	// query record { a: 1 }
	bson sel ;
	bson_init( &sel ) ;
	bson_append_string( &sel, "a", "" ) ;
	bson_finish( &sel ) ;
	rc = sdbQuery( cl, &obj, &sel, NULL, NULL, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to query record { a: 1 }" ;
	// check result
	bson ret ;
	bson_init( &ret ) ;
	rc = sdbNext( cursor, &ret ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get result" ;
	bson_iterator it ;
	bson_iterator_init( &it, &ret ) ;
	ASSERT_EQ( 1, bson_iterator_int(&it) ) << "fail to check result" ;
	sdbReleaseCursor( cursor ) ;
	
	// update record { $set: { a: 100 } }
	bson update ;
	bson_init( &update ) ;
	bson_append_start_object( &update, "$set" ) ;
	bson_append_int( &update, "a", 100 ) ;
	bson_append_finish_object( &update ) ;
	bson_finish( &update ) ;
	rc = sdbUpdate( cl, &update, &obj, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to update record" ;
	
	// query record { a: 100 }
	bson cond ;
	bson_init( &cond ) ;
	bson_append_int( &cond, "a", 100 ) ;
	bson_finish( &cond ) ;
	rc = sdbQuery( cl, &cond, &sel, NULL, NULL, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to query record { a: 100 }" ;
	// check result
	bson_destroy( &ret ) ;
	bson_init( &ret ) ;
	rc = sdbNext( cursor, &ret ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get result" ;
	bson_iterator_init( &it, &ret ) ;
	ASSERT_EQ( 100, bson_iterator_int(&it) ) << "fail to check result" ;
	sdbReleaseCursor( cursor ) ;
	
	// delete record { a: 100 }
	rc = sdbDelete( cl, &cond, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to delete record { a: 100 }" ;
	// query record { a: 100 }
	rc = sdbQuery( cl, &cond, &sel, NULL, NULL, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to query record { a: 100 }" ;
	// check result
	bson_destroy( &ret ) ;
	bson_init( &ret ) ;
	rc = sdbNext( cursor, &ret ) ;
	ASSERT_EQ( rc, SDB_DMS_EOC ) << "fail to check delete" ;
	
	// destroy bson
	bson_destroy( &obj ) ;
	bson_destroy( &sel ) ;
	bson_destroy( &ret ) ;
	bson_destroy( &update ) ;
	bson_destroy( &cond ) ;
	
	// drop cs
	rc = sdbDropCollectionSpace( db, CsName ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to drop cs " << CsName ;
   	// release handle
   	sdbDisconnect( db ) ;
   	sdbReleaseCursor( cursor ) ;
   	sdbReleaseCollection( cl ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;
}
 
