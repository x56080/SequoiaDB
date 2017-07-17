/**************************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-1110
*               SEQUOIADBMAINSTREAM-849
* @Modify     : Liang xuewang Init
*			 	2016-11-23
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/testcommon.hpp"

char *CsModName = "c_driver_test" ;
char CsName[100] ;
char *ClName = "idIndex" ;
sdbConnectionHandle db = 0 ;
sdbCSHandle cs = 0 ;
sdbCollectionHandle cl = 0 ;

int setup()
{
	int rc = SDB_OK ;
	bson options ;
    bson_init( &options ) ;
	int num = 2000, i ;
    bson* obj[num] ;

	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect sdb, rc = %d\n", rc ) ;

	getUniqueName( CsModName, CsName ) ;
	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
	CHECK_RC( rc, "fail to create cs %s, rc = %d\n", CsName, rc ) ;

	// option is { ReplSize: 0 }, { Compressed: true }, { AutoIndexId: false }
	bson_append_int( &options, "ReplSize", 0 ) ;
	bson_append_bool( &options, "Compressed", true ) ;
	bson_append_bool( &options, "AutoIndexId", false ) ;
	bson_finish( &options ) ;
	// bson_print( &options ) ;

	// create cl
	rc = sdbCreateCollection1( cs, ClName, &options, &cl ) ;
	CHECK_RC( rc, "fail to create cl %s, rc = %d\n", ClName, rc ) ;
	bson_destroy( &options ) ;

	// insert records like { "_id": 1, "f1": 2, "f2": 3 }
	for( i = 0;i < num;++i )
	{
		obj[i] = bson_create() ;
		bson_append_int( obj[i], "_id", i ) ;
		bson_append_int( obj[i], "f1", i+1 ) ;
		bson_append_int( obj[i], "f2", i+2 ) ;
		bson_finish( obj[i] ) ;
	}
	rc = sdbBulkInsert( cl, 0, obj, num ) ;
	CHECK_RC( rc, "fail to bulk insert, rc = %d\n", rc ) ;
	for( i = 0;i < num;++i )
	{
		bson_dispose( obj[i] ) ;
	}

done:
	return rc ;
error:
	goto done ;
}

int teardown()
{
	int rc = SDB_OK ;

	// drop cs cl disconnect
	rc = sdbDropCollection( cs, ClName ) ;
	CHECK_RC( rc, "fail to drop cl %s, rc = %d\n", ClName, rc ) ;
	rc = sdbDropCollectionSpace( db, CsName ) ;
	CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", CsName, rc ) ;
	sdbDisconnect( db ) ;

	// release handle
	sdbReleaseCollection( cl ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseConnection( db ) ;

done:
	return rc ;
error:
	goto done ;
}

TEST( indexTest, createIdIndex )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// create id index
	bson option ;
	bson_init( &option ) ;
	bson_append_int( &option, "SortBufferSize", 128 ) ;
	rc = sdbCreateIdIndex( cl, &option ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create id index" ;
	bson_destroy( &option ) ;

	// get id index
	sdbCursorHandle cursor ;
	rc = sdbGetIndexes( cl, "$id", &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get id index" ;
		
	// query record 
	const char* c = "{ _id: 555 }" ;
	bson cond ;
	jsonToBson( &cond, c ) ;
	const char* s = "{ _id: \"\" }" ;
	bson sel ;
	jsonToBson( &sel, s ) ;
	const char* h = "{ _id: 0 }" ;
	bson hint ;
	jsonToBson( &hint, h ) ;
	rc = sdbQuery( cl, &cond, &sel, NULL, &hint, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to query record" ;
	bson obj ;
	bson_init( &obj ) ;
	rc = sdbNext( cursor, &obj ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get query cursor doc" ;
	char result[100] = { 0 } ;
	bson_sprint( result, sizeof(result), &obj ) ;
	char *expect = "{ \"_id\": 555 }" ;
	ASSERT_EQ( 0, strcmp( expect, result ) ) << "fail to check query result, expect: "
											 << expect << " actual: " << result ;
	bson_destroy( &obj ) ;
	sdbCloseCursor( cursor ) ;
	
	// explain
	rc = sdbExplain( cl, &cond, &sel, NULL, NULL, 0, 0, -1, NULL, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to explain query" ;
	bson_init( &obj ) ;
	rc = sdbNext( cursor, &obj ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get explain cursor doc" ;
	bson_iterator it ;
	bson_find( &it, &obj, "ScanType" ) ;
	const char* scanType = bson_iterator_string( &it ) ;
	ASSERT_STREQ( scanType, "ixscan" ) << "fail to check scan type" ;
	bson_find( &it, &obj, "IndexName" ) ;
	const char* indexName =  bson_iterator_string( &it ) ;
	ASSERT_STREQ( indexName, "$id" ) << "fail to check index name" ;
	sdbCloseCursor( cursor ) ;
	
	// drop id index
	rc = sdbDropIdIndex( cl ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to drop id index" ;
	
	// update after drop id index
	bson rule ;
	bson_init( &rule ) ;
    bson_append_start_object( &rule, "$inc" ) ;
    bson_append_int( &rule, "f1", 1 ) ;
    bson_append_finish_object( &rule ) ;
    bson_finish( &rule ) ;
    rc = sdbUpdate( cl, &rule, NULL, NULL ) ;
    ASSERT_EQ( rc, SDB_RTN_AUTOINDEXID_IS_FALSE ) << "fail to test update after drop id index" ;
	
	// destroy bson and release cursor
	bson_destroy( &rule ) ;
	bson_destroy( &obj ) ;
	bson_destroy( &cond ) ;
	bson_destroy( &sel ) ;
	bson_destroy( &hint ) ;
	sdbReleaseCursor( cursor ) ;

	rc = teardown() ;
	ASSERT_EQ( rc, SDB_OK ) ;
}
