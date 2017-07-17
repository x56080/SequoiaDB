/**************************************************************
* @Description: test case of $numberLong JSCompatible 
*				TestLink 10968  
* @Modify     : Liang xuewang Init
*			 	2017-01-09
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/testcommon.hpp"

sdbConnectionHandle db = SDB_INVALID_HANDLE ;
sdbCSHandle cs         = SDB_INVALID_HANDLE ;
sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
const char* csModName = "C_drivertest_syncCs" ;
char csName[100] ;
const char* clName = "C_drivertest_syncCl" ;

int setUp()
{
	int rc = SDB_OK ;

    getUniqueName( csModName, csName ) ;
    rc = createNormalCl( &db, &cs, &cl, csName, clName ) ;
    CHECK_RC( rc, "fail to create normal cl, rc = %d\n", rc ) ;

done:
	return rc ;
error:
	goto done ;
}

int tearDown()
{
	int rc = SDB_OK ;
	
	rc = sdbDropCollectionSpace( db, csName ) ;
	CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", csName, rc ) ;
	sdbDisconnect( db ) ;
	sdbReleaseCollection( cl ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseConnection( db ) ;

done:
	return rc ;
error:
	goto done ;
}

TEST( NumberLongTest, JSfalse )
{
	int rc = SDB_OK ;
	rc = setUp() ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// insert int/long/double max min 
	int  a[] = { -2147483648, 0, 2147483647 } ;  // -2^31 0 2^31-1
	long b[] = { -9223372036854775808, -9007199254740992, -9007199254740991, 1, 
				 9007199254740991, 9007199254740992, 9223372036854775807 } ; // -2^63 -2^53 -2^53+1 1 2^53-1 2^53 2^63-1
	bson obj ;
	bson_init( &obj ) ;
	for( int i = 0;i < sizeof(a)/sizeof(a[0]);i++ )
	{
		char key[10] ;
		sprintf( key, "%s%d", "int", i ) ;
		bson_append_int( &obj, key, a[i] ) ;
	}
	for( int i = 0;i < sizeof(b)/sizeof(b[0]);i++ )
	{
		char key[10] ;
        sprintf( key, "%s%d", "long", i ) ;
		bson_append_long( &obj, key, b[i] ) ;
	}
	bson_finish( &obj ) ;
	rc = sdbInsert( cl, &obj ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to insert data" ;
	bson_destroy( &obj ) ;

	// query data
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson selector ;
	bson_init( &selector ) ;
	bson_append_start_object( &selector, "_id" ) ;
	bson_append_int( &selector, "$include", 0 ) ;
	bson_append_finish_object( &selector ) ;
	bson_finish( &selector ) ;
	rc = sdbQuery( cl, NULL, &selector, NULL, NULL, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to query data" ;
	bson_init( &obj ) ;
	rc = sdbNext( cursor, &obj ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get next in cursor" ;

	const char* expect = "{ \"int0\": -2147483648, \"int1\": 0, \"int2\": 2147483647,"
						 " \"long0\": -9223372036854775808, \"long1\": -9007199254740992,"
						 " \"long2\": -9007199254740991, \"long3\": 1, \"long4\": 9007199254740991,"
						 " \"long5\": 9007199254740992, \"long6\": 9223372036854775807 }" ; 
	char real[1024] ;
	bson_sprint( real, 1024, &obj ) ;
	ASSERT_STREQ( expect, real ) << "fail to check query data" ;
	bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;

	rc = tearDown() ;
	ASSERT_EQ( rc, SDB_OK ) ;
}

TEST( NumberLongTest, JStrue )
{
    int rc = SDB_OK ;
	rc = setUp() ;
	ASSERT_EQ( rc, SDB_OK ) ;

	bson_set_js_compatibility( true ) ;
	
    // insert int/long/double max min 
    int  a[] = { -2147483648, 0, 2147483647 } ;  // -2^31 0 2^31-1
    long b[] = { -9223372036854775808, -9007199254740992, -9007199254740991, 1,
                  9007199254740991, 9007199254740992, 9223372036854775807 } ; // -2^63 -2^53 -2^53+1 1 2^53-1 2^53 2^63-1
    bson obj ;
    bson_init( &obj ) ;
    for( int i = 0;i < sizeof(a)/sizeof(a[0]);i++ )
    {
        char key[10] ;
        sprintf( key, "%s%d", "int", i ) ;
        bson_append_int( &obj, key, a[i] ) ;
    }
    for( int i = 0;i < sizeof(b)/sizeof(b[0]);i++ )
    {
        char key[10] ;
        sprintf( key, "%s%d", "long", i ) ;
        bson_append_long( &obj, key, b[i] ) ;
    }
	bson_finish( &obj ) ;
    rc = sdbInsert( cl, &obj ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to insert data" ;
    bson_destroy( &obj ) ;

    // query data
    sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
    bson selector ;
    bson_init( &selector ) ;
    bson_append_start_object( &selector, "_id" ) ;
	bson_append_int( &selector, "$include", 0 ) ;
    bson_append_finish_object( &selector ) ;
    bson_finish( &selector ) ;
    rc = sdbQuery( cl, NULL, &selector, NULL, NULL, 0, -1, &cursor ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to query data" ;
    bson_init( &obj ) ;
    rc = sdbNext( cursor, &obj ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to get next in cursor" ;
	const char* expect = "{ \"int0\": -2147483648, \"int1\": 0, \"int2\": 2147483647,"
						 " \"long0\": { \"$numberLong\": \"-9223372036854775808\" },"
						 " \"long1\": { \"$numberLong\": \"-9007199254740992\" },"
						 " \"long2\": -9007199254740991, \"long3\": 1, \"long4\": 9007199254740991,"
						 " \"long5\": { \"$numberLong\": \"9007199254740992\" },"
						 " \"long6\": { \"$numberLong\": \"9223372036854775807\" } }" ;
    char real[1024] ;
    bson_sprint( real, 1024, &obj ) ;
    ASSERT_STREQ( expect, real ) << "fail to check query data" ;
    bson_destroy( &obj ) ;
    sdbReleaseCursor( cursor ) ;

	rc = tearDown() ;
	ASSERT_EQ( rc, SDB_OK ) ;
}

