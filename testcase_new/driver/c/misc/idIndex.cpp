/**************************************************************
* @Description: test case for Jira questionaire
*				    SEQUOIADBMAINSTREAM-1110
*               SEQUOIADBMAINSTREAM-849
* @Modify     : Liang xuewang Init
*			 	    2016-11-23
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

void prepareCl()
{
	int rc = SDB_OK ;

	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;

	getUniqueName( CsModName, CsName ) ;
	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
	if( rc == SDB_DMS_CS_EXIST )
	{
		rc = sdbDropCollectionSpace( db, CsName ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to drop cs existed" ;
	    rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;	
	}
	ASSERT_EQ( rc, SDB_OK ) << "fail to create cs" ;

	// option is {ReplSize:0},{Compressed:true},{AutoIndexId:false}
	bson options ;
	bson_init( &options ) ;
	bson_append_int( &options, "ReplSize", 0 ) ;
	bson_append_bool( &options, "Compressed", true ) ;
	bson_append_bool( &options, "AutoIndexId", false ) ;
	bson_finish( &options ) ;
	// bson_print( &options ) ;
	// create cl
	rc = sdbCreateCollection1( cs, ClName, &options, &cl ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to create cl" ;
	// destroy options bson
	bson_destroy( &options ) ;

	// insert records like {"_id":1,"f1":2,"f2":3}
	int num = 2000, i ;
	bson* obj[num] ;
	for( i = 0;i < num;++i )
	{
		obj[i] = bson_create() ;
		bson_append_int( obj[i], "_id", i ) ;
		bson_append_int( obj[i], "f1", i+1 ) ;
		bson_append_int( obj[i], "f2", i+2 ) ;
		bson_finish( obj[i] ) ;
	}
	rc = sdbBulkInsert( cl, 0, obj, num ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to insert record in the "<<i<<" times" ;
	for( i = 0;i < num;++i )
	{
		bson_dispose( obj[i] ) ;
	}
}

void cleanResource()
{
	int rc = SDB_OK ;

	// drop cs cl disconnect
	rc = sdbDropCollection( cs, ClName ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to drop cl" ;
	rc = sdbDropCollectionSpace( db, CsName ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to drop cs" ;
	sdbDisconnect( db ) ;

	// release handle
	sdbReleaseCollection( cl ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseConnection( db ) ;
}

TEST(indexTest,createIdIndex)
{
	// prepare:create cl and insert records
	prepareCl() ;

	// create id index
	int rc = SDB_OK ;
	bson option ;
	bson_init( &option ) ;
	bson_append_int( &option, "SortBufferSize", 128 ) ;
	rc = sdbCreateIdIndex( cl, &option ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to create id index" ;
	bson_destroy( &option ) ;

	// get id index
	sdbCursorHandle cursor ;
	rc = sdbGetIndexes( cl, "$id", &cursor ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to get index" ;
		
	// query record 
	const char* c = "{_id:555}" ;
	bson cond ;
	jsonToBson( &cond, c ) ;
	const char* s = "{_id:\"\"}" ;
	bson sel ;
	jsonToBson( &sel, s ) ;
	const char* h = "{_id:0}" ;
	bson hint ;
	jsonToBson( &hint, h ) ;
	rc = sdbQuery( cl, &cond, &sel, NULL, &hint, 0, -1, &cursor ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to query record" ;
	bson obj ;
	bson_init( &obj ) ;
	rc = sdbNext( cursor, &obj ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to get query cursor doc" ;
	char result[100] = {0} ;
	bson_sprint( result, sizeof(result), &obj ) ;
	char *expect = "{ \"_id\": 555 }" ;
	EXPECT_EQ( 0, strcmp(expect,result) )<<"fail to check query result,expect"<<expect<<" actual:"<<result ;
	bson_destroy( &obj ) ;
	sdbCloseCursor( cursor ) ;
	
	// explain
	rc = sdbExplain( cl, &cond, &sel, NULL, NULL, 0, 0, -1, NULL, &cursor ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to explain query" ;
	bson_init( &obj ) ;
	rc = sdbNext( cursor, &obj ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to get explain cursor doc" ;
	bson_iterator it ;
	bson_find( &it, &obj, "ScanType" ) ;
	const char* scanType = bson_iterator_string( &it ) ;
	EXPECT_STREQ( scanType, "ixscan" ) << "fail to check scan type" ;
	bson_find( &it, &obj, "IndexName" ) ;
	const char* indexName =  bson_iterator_string( &it ) ;
	EXPECT_STREQ( indexName, "$id" ) << "fail to check index name" ;
	sdbCloseCursor( cursor ) ;
	
	// drop id index
	rc = sdbDropIdIndex( cl ) ;
	EXPECT_EQ( rc, SDB_OK ) << "fail to drop id index" ;
	
	// update after drop id index
	bson rule ;
	bson_init( &rule ) ;
    bson_append_start_object ( &rule, "$inc" ) ;
    bson_append_int ( &rule, "f1", 1 ) ;
    bson_append_finish_object ( &rule ) ;
    bson_finish ( &rule ) ;
    rc = sdbUpdate( cl, &rule, NULL, NULL ) ;
    EXPECT_EQ( rc, SDB_RTN_AUTOINDEXID_IS_FALSE ) << "fail to test update after drop id index" ;
	
	// destroy bson and release cursor
	bson_destroy( &rule ) ;
	bson_destroy( &obj ) ;
	bson_destroy( &cond ) ;
	bson_destroy( &sel ) ;
	bson_destroy( &hint ) ;
	sdbReleaseCursor( cursor ) ;

	// clean resource:drop cs cl disconnect releaseHandle
	cleanResource() ;
}
