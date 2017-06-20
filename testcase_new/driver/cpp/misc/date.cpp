/**************************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-2507
* @Modify:      Liang xuewang Init
*			 	2017-02-28
***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "../testcommon.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

const char* USER   = "" ;
const char* PASSWD = "" ;
const char* csName = "dateTestCs" ;
const char* clName = "dateTestCl" ;

sdb db ;
sdbCollectionSpace cs ;
sdbCollection cl ;

#define ASSERT_RC( rc, msg ) \
if( rc != SDB_OK ) \
{ \
   cout << msg << ", rc = " << rc << endl ; \
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
	// connect and create cs cl
	getConf() ;
	rc = db.connect( HOSTNAME, SVCNAME, USER, PASSWD ) ;
	ASSERT_RC( rc, "fail to connect sdb" ) 
	rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
	ASSERT_RC( rc, "fail to create cs" )
	rc = cs.createCollection( clName, cl ) ;
	ASSERT_RC( rc, "fail to create cl" )
}

void DateTest::TearDownTestCase()
{
	INT32 rc = SDB_OK ;
	// drop cs and disconnect
	rc = db.dropCollectionSpace( csName ) ;
	ASSERT_RC( rc, "fail to drop cs" )
	db.disconnect() ;
}

TEST_F( DateTest, Date_t )
{
	unsigned long long mills[] = {
		-62167248000000,   // 0000-01-01 00:00:00
        253402271999000,   // 9999-12-31 23:59:59
        -62167248001000,   // -0001-12-31 23:59:59 
        253402272000000    // 10000-01-01 00:00:00
	} ;
	int i ;
	int rc = SDB_OK ;
	for( i = 0;i < sizeof(mills)/sizeof(mills[0]);i++ )
	{
		BSONObjBuilder b ;
    	BSONObj obj, res ;
    	BSONObj sel = BSON( "myDate" << "" ) ;
    	sdbCursor cursor ;
	
		rc = cl.truncate() ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to truncate cl" ;

		Date_t dt( mills[i] ) ;
		b.appendDate( "myDate", dt ) ;
		obj = b.obj() ;
		
		rc = cl.insert( obj ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to insert cl, mills: " << mills[i] ;
		
		rc = cl.query( cursor, _sdbStaticObject, sel ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to query cl, mills: " << mills[i] ;

		rc = cursor.next( res ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to get next, mills: " << mills[i] ;

		// cout << mills[i] << " " << res << endl ;
		BSONElement ele = res.getField( "myDate" ) ;
		BSONType type = ele.type() ;
		ASSERT_EQ( type, Date ) ;
		Date_t date = ele.Date() ;
		ASSERT_EQ( date, mills[i] ) ;
		// cout << date.toString() << endl ;
	}
}
