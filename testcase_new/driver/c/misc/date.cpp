/**************************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-2507
* @Modify:      Liang xuewang Init
*			 	2017-06-19
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "../common/testcommon.hpp"

const char* user 	   = "" ;
const char* passwd     = "" ;
const char* csModName  = "dateTestCs" ;
char 		csName[50] = { 0 } ;
const char* clName 	   = "dateTestCl" ; 
sdbConnectionHandle db = SDB_INVALID_HANDLE ;
sdbCSHandle			cs = SDB_INVALID_HANDLE ;
sdbCollectionHandle cl = SDB_INVALID_HANDLE ;

#define ASSERT_RC( rc, msg ) \
if( rc != SDB_OK ) \
{ \
   printf( "%s,rc=%d\n", msg, rc ) ; \
   exit(1) ; \
}

class DateTest : public testing::Test
{
public:
	static void SetUpTestCase() ;
	static void TearDownTestCase() ;
} ;

void DateTest::SetUpTestCase()
{
	int rc = SDB_OK ;
	//  connect sdb, create cs cl
    getConf() ;
    rc = sdbConnect( HOSTNAME, SVCNAME, user, passwd, &db ) ;
    ASSERT_RC( rc, "fail to connect sdb" ) ;
    getUniqueName( csModName, csName ) ;
    rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
    ASSERT_RC( rc, "fail to create cs" ) ;
    rc = sdbCreateCollection( cs, clName, &cl ) ;
    ASSERT_RC( rc, "fail to create cl" ) ;
}

void DateTest::TearDownTestCase()
{
	int rc = SDB_OK ;
	// drop cs,disconnect and release handle
    rc = sdbDropCollectionSpace( db, csName ) ;
    ASSERT_RC( rc, "fail to drop cs" ) ;
    sdbDisconnect( db ) ;
    sdbReleaseCollection( cl ) ;
    sdbReleaseCS( cs ) ;
    sdbReleaseConnection( db ) ;
}

TEST_F( DateTest, json_bson )
{
	int rc = SDB_OK ;

	// normal date to insert, [ 0000-01-01, 9999-12-31 ]
	const char* normalDate[] = {
		"{ \"myDate\": { \"$date\": \"0000-01-01\" } }",
		"{ \"myDate\": { \"$date\": \"1840-01-01\" } }",
		"{ \"myDate\": { \"$date\": \"9999-12-31\" } }"
	} ;
	const char* abnormalDate[] = {
		"{ \"myDate\": { \"$date\": \"0000-01-00\" } }",
		"{ \"myDate\": { \"$date\": \"10000-01-01\" } }"
	} ;

	// insert normal date and query
	bson obj, sel, res ;
	bson_init( &sel ) ;
	rc = bson_append_string( &sel, "myDate", "" ) ;
	ASSERT_EQ( rc, BSON_OK ) << "fail to append myDate on sel" ;
	rc = bson_finish( &sel ) ;
	ASSERT_EQ( rc, BSON_OK ) << "fail to finish myDate sel" ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	int i ;
	for( i = 0;i < sizeof(normalDate)/sizeof(const char*);i++ )
	{
		rc = sdbDelete( cl, NULL, NULL ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to truncate cl " << clName ;
		bson_init( &obj ) ;
		ASSERT_TRUE( jsonToBson( &obj, normalDate[i] ) ) << "fail to check jsonToBson " << normalDate[i] ;
		rc = sdbInsert( cl, &obj ) ;
		bson_destroy( &obj ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to insert date " << normalDate[i] ;
		rc = sdbQuery( cl, NULL, &sel, NULL, NULL, 0, -1, &cursor ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to query cl " << clName ;
		bson_init( &res ) ;
		rc = sdbNext( cursor, &res ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to get next of cursor in cl " << clName ; 
		// bson_print( &res ) ;
		char buffer[1024] = { 0 } ;
        ASSERT_TRUE( bsonToJson( buffer, sizeof(buffer), &res, false ,false ) ) ;
        ASSERT_STREQ( buffer, normalDate[i] ) ;
		bson_destroy( &res ) ;
		sdbReleaseCursor( cursor ) ;	
	}

	// check abnormal date
    for( i = 0;i < sizeof(abnormalDate)/sizeof(const char*);i++ )
    {
        bson_init( &obj ) ;
        ASSERT_FALSE( jsonToBson( &obj, abnormalDate[i] ) ) << "fail to check jsonToBson " << abnormalDate ;
        bson_destroy( &obj ) ;
    }
}

TEST_F( DateTest, mills )
{
	int rc = SDB_OK ;
	
	bson_date_t mills[] = { 
		-62167248352000,   // 0000-01-01 00:00:00
		253402271999000,   // 9999-12-31 23:59:59
		-62167248353000,   // -0001-12-31 23:59:59 
		253402272000000    // 10000-01-01 00:00:00
	} ;

	// insert mills and query
	bson obj, sel, res ;	
	bson_init( &sel ) ;
	rc = bson_append_string( &sel, "myDate", "" ) ;
	ASSERT_EQ( rc, BSON_OK ) << "fail to append myDate on sel" ;
    rc = bson_finish( &sel ) ;
	ASSERT_EQ( rc, BSON_OK ) << "fail to finish sel" ;
	
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	int i ;
	for( i = 0;i < sizeof(mills)/sizeof(bson_date_t);i++ )
    {
		// printf( "mills: %ld\n", mills[i] ) ;
        rc = sdbDelete( cl, NULL, NULL ) ;
        ASSERT_EQ( rc, SDB_OK ) << "fail to truncate cl " << clName ;

        bson_init( &obj ) ;
		rc = bson_append_date( &obj, "myDate", mills[i] ) ;
		ASSERT_EQ( rc, BSON_OK ) << "fail to append myDate on obj, mills: " << mills[i] ;
        rc = bson_finish( &obj ) ;
		ASSERT_EQ( rc, BSON_OK ) << "fail to finish obj, mills: " << mills[i] ;

        rc = sdbInsert( cl, &obj ) ;
        bson_destroy( &obj ) ;
        ASSERT_EQ( rc, SDB_OK ) << "fail to insert date " << mills[i] ;

        rc = sdbQuery( cl, NULL, &sel, NULL, NULL, 0, -1, &cursor ) ;
        ASSERT_EQ( rc, SDB_OK ) << "fail to query cl " << clName ;

        bson_init( &res ) ;
        rc = sdbNext( cursor, &res ) ;
        ASSERT_EQ( rc, SDB_OK ) << "fail to get next of cursor in cl " << clName ;
        
		// bson_print( &res ) ;
		bson_iterator it ;
		bson_find( &it, &res, "myDate" ) ;
		bson_type type = bson_iterator_type( &it ) ;
		ASSERT_EQ( type, BSON_DATE ) ;
		bson_date_t date = bson_iterator_date( &it ) ;
		ASSERT_EQ( date, mills[i] ) ;
		bson_destroy( &res ) ;

        sdbReleaseCursor( cursor ) ;
    }
}	
