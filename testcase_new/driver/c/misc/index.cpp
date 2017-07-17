/**************************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-1341
* @Modify:      Liang xuewang Init
*			 	2016-11-10
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common/testcommon.hpp"

char *CsModName = "c_driver_test" ;
char CsName[100] ;
char *ClName = "index" ;
sdbConnectionHandle db = 0 ;
sdbCSHandle cs = 0 ;
sdbCollectionHandle cl = 0 ;

int setup()
{
	int rc = SDB_OK ;
	bson options ;
    bson_init( &options ) ;

	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect sdb, rc = %d\n", rc ) ;

	getUniqueName( CsModName, CsName ) ;
	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
	CHECK_RC( rc, "fail to create cs %s, rc = %d\n", CsName, rc ) ;

	// option is { ShardingKey: { id: 1 } }, { ReplSize: 0 }, { Compressed: true }
	bson_append_start_object( &options, "ShardingKey" ) ;
	bson_append_int( &options, "id", 1 ) ;
	bson_append_finish_object( &options ) ;
	bson_append_int( &options, "ReplSize", 0 ) ;
	bson_append_bool( &options, "Compressed", true ) ;
	bson_finish( &options ) ;
	// bson_print( &options ) ;

	// create cl
	rc = sdbCreateCollection1( cs, ClName, &options, &cl ) ;
	CHECK_RC( rc, "fail to create cl %s, rc = %d\n", ClName, rc ) ;
	bson_destroy( &options ) ;

	// insert records like { "id": 1, "f1": 2, "f2": 3 }
	for( int i = 0;i < 1000;++i )
	{
		bson record ;
		bson_init( &record ) ;
		bson_append_int( &record, "id", i ) ;
		bson_append_int( &record, "f1", i+1 ) ;
		bson_append_int( &record, "f2", i+2 ) ;
		bson_finish( &record ) ;
		// bson_print( &record ) ;
		rc = sdbInsert( cl, &record ) ;
		CHECK_RC( rc, "fail to insert record in the %d time, rc = %d", i, rc ) ;
		bson_destroy( &record ) ;
	}

done:
	return rc ;
error:
	goto done ;
}

int teardown()
{
	int rc = SDB_OK ;

	rc = sdbDropCollection( cs, ClName ) ;
	CHECK_RC( rc, "fail to drop cl %s, rc = %d\n", ClName, rc ) ;
	rc = sdbDropCollectionSpace( db, CsName ) ;
	CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", CsName, rc ) ;
	sdbDisconnect( db ) ;
	sdbReleaseCollection( cl ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseConnection( db ) ;

done:
	return rc ;
error:
	goto done ;
}

TEST( indexTest, createIndex )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// create index
	char *indexName = "myIndex" ;
	bson indexDef ;
	bson_init( &indexDef ) ;
	bson_append_int( &indexDef, "id", -1 ) ;
	bson_finish( &indexDef ) ;
	rc = sdbCreateIndex1( cl, &indexDef, indexName, false, false, 128 ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create index" ;
	bson_destroy( &indexDef ) ;

	// get index
	sdbCursorHandle cursor ;
	rc = sdbGetIndexes( cl, indexName, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get index" ;
	bson obj ;
	bson_init( &obj ) ;
	rc = sdbNext( cursor, &obj ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get the index cursor doc" ;
	bson_iterator it ;
	bson_iterator_init( &it, &obj ) ;
	bson_iterator sub ;
	bson_iterator_subiterator( &it, &sub ) ;
	const char *name = bson_iterator_string( &sub ) ;
	ASSERT_EQ( 0, strcmp( name, indexName ) ) << "index name wrong, expect:" << indexName << " actual:" << name ;
	sdbReleaseCursor( cursor ) ;
		
	// query record 
	const char* c = "{ id: 555 }" ;
	bson cond ;
	jsonToBson( &cond, c ) ;
	const char* s = "{ id: \"\" }" ;
	bson sel ;
	jsonToBson( &sel, s ) ;
	const char* h = "{ id: 0 }" ;
	bson hint ;
	jsonToBson( &hint, h ) ;
	rc = sdbQuery( cl, &cond, &sel, NULL, &hint, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to query record" ;
	rc = sdbNext( cursor, &obj ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get query cursor doc" ;
	char result[100] = { 0 } ;
	bson_sprint( result, sizeof(result), &obj ) ;
	char *expect = "{ \"id\": 555 }" ;
	ASSERT_EQ( 0, strcmp( expect, result ) ) << "fail to check query result, expect:" << expect << " actual:" << result ;
	// destroy bson and release cursor
	bson_destroy( &obj ) ;
	bson_destroy( &cond ) ;
	bson_destroy( &sel ) ;
	bson_destroy( &hint ) ;
	sdbReleaseCursor( cursor ) ;

	rc = teardown() ;
	ASSERT_EQ( rc, SDB_OK ) ;
}
