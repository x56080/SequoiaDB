#include <stdio.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include "testcommon.hpp"
#include "testWorker.hpp"
#include "client.hpp"
#include "sdbDataSourceComm.hpp"
#include "sdbDataSource.hpp"


using namespace std ;
using namespace test ;
using namespace sdbclient ;

#define THREAD_TIME_OUT ( 60 * 6 )

BOOLEAN global_init_flag   = FALSE ;
BOOLEAN case_init_flag     = FALSE ;

struct DS_WorderArgs: public WorkerArgs
{
	sdbDataSource *pDatasource ;
	INT32 returnCode ;
} ;

void ds_WorkerRoutine(WorkerArgs *args)
{
	INT32 rc = SDB_OK ;
	//INT32 sleepMillis = 6000 * 1000 ;
	INT32 sleepMillis = 200 * 1000 ;
	DS_WorderArgs *pArgs = (DS_WorderArgs*)args ;
	sdbDataSource *pDatasource = pArgs->pDatasource ;
	
	while( true )
	{
		sdb *db = NULL ;
		BOOLEAN isValid = FALSE ;
	
		rc = pDatasource->getConnection( db, 500 ) ;
		if ( SDB_OK != rc )
		{
			printf( "error happen is: rc = %d"OSS_NEWLINE, rc ) ;
			break ;
		}
		isValid = db->isValid() ;
		ASSERT_EQ( TRUE, isValid ) ;
		usleep( sleepMillis ) ;
		pDatasource->releaseConnection( db ) ;
	}
	
done:
	pArgs->returnCode = rc ;
	return ;
error:
	goto done ;
}

class DatasourceTest : public testing::Test
{
public:
	static sdb db ;
    static sdbCollectionSpace cs ;
    static sdbCollection cl ;
	
	static const CHAR *pHostName ;
	static const CHAR *pSvcName ;
	static const CHAR *pUser ;
	static const CHAR *pPassword ;
	
	static const CHAR *pCSName ;
	static const CHAR *pCLName ;

public:
	DatasourceTest() {}

public:
	// run before all the testcase
	static void SetUpTestCase() ;

	// run before all the testcase
	static void TearDownTestCase() ;

	// run before every testcase
	virtual void SetUp() ;

	// run before every testcase
	virtual void TearDown() ;
} ;

sdb DatasourceTest::db ;
sdbCollectionSpace DatasourceTest::cs ;
sdbCollection DatasourceTest::cl ;

const CHAR * DatasourceTest::pHostName      = "192.168.20.165" ;
const CHAR * DatasourceTest::pSvcName       = "21810" ;
//const CHAR * DatasourceTest::pSvcName       = "50000" ;
// const CHAR * DatasourceTest::pHostName      = HOST ;
// const CHAR * DatasourceTest::pSvcName       = SERVER ;
const CHAR * DatasourceTest::pUser          = USER ;
const CHAR * DatasourceTest::pPassword      = PASSWD ;
const CHAR * DatasourceTest::pCSName        = COLLECTION_SPACE_NAME ;
const CHAR * DatasourceTest::pCLName        = COLLECTION_NAME ;

void DatasourceTest::SetUpTestCase()
{
	INT32 rc = SDB_OK ;
	BSONObj options = BSON( "PreferedInstance" << "M" ); 

	global_init_flag = FALSE ;
	rc = db.connect( pHostName, pSvcName, pUser, pPassword ) ;
	if ( SDB_OK != rc )
	{
		printf( "Error: %s:%d: failed to connect to database: rc = %d"OSS_NEWLINE,
				__FUNC__, __LINE__, rc ) ;
		goto error ;
	}
	rc = db.setSessionAttr( options ) ;
	if ( SDB_OK != rc )
	{
		printf( "Error: %s:%d: failed to set session attribute, "
				"rc = %d"OSS_NEWLINE, __FUNC__, __LINE__, rc ) ;
		goto error ;
	}
	global_init_flag = TRUE ;

done :
	return ;
error :
	goto done ;
}

void DatasourceTest::TearDownTestCase()
{
	db.disconnect() ;
}

void DatasourceTest::SetUp()
{
    INT32 rc = SDB_OK ;
    case_init_flag = FALSE ;
    if ( FALSE == global_init_flag )
    {
        goto done ;
    }
    // drop and create cs
	rc = db.dropCollectionSpace( pCSName ) ;
    if ( SDB_DMS_CS_NOTEXIST != rc && SDB_OK != rc )
    {
        printf( "Error: %s:%d, failed to drop cs[%s] in db, rc = %d"OSS_NEWLINE,
              __FUNC__, __LINE__, pCSName, rc ) ;
        goto error ;
    }
    rc = db.createCollectionSpace( pCSName, SDB_PAGESIZE_DEFAULT, cs ) ;
    if ( SDB_OK != rc )
    {
        printf( "Error: %s:%d, failed to create cs[%s], rc = %d"OSS_NEWLINE,
             __FUNC__, __LINE__, pCSName, rc ) ;
        goto error ;
    }
    // create cl
    rc = cs.createCollection( pCLName, BSON( "ReplSize" << 0 ), cl ) ;
    if ( SDB_OK != rc )
    {
        printf( "Error: %s:%d, failed to create cl[%s], rc = %d"OSS_NEWLINE,
                 __FUNC__, __LINE__, pCLName, rc ) ;
        goto error ;
    }
    case_init_flag = TRUE ;
done:
	return ;
error:
	goto done ;
}

void DatasourceTest::TearDown()
{
	INT32 rc = SDB_OK ;
	if ( FALSE == case_init_flag )
	{
		return ;
    }
	rc = db.dropCollectionSpace( pCSName ) ;
	if ( SDB_DMS_CS_NOTEXIST != rc && SDB_OK != rc )
	{
		printf( "Error: %s:%d, failed to drop cs[%s] in db, rc = %d"OSS_NEWLINE,
			  __FUNC__, __LINE__, pCSName, rc ) ;
		goto error ; 
	}
done:
	return ;
error:
	goto done ;
}
/*
TEST_F( DatasourceTest, demo_test )
{
	INT32 rc = SDB_OK ;
	INT32 threadCount = 500 ;
	sdbDataSourceConf conf ;
	stringstream ss ;
	string addres ;
	ss << pHostName << ":" << pSvcName ;
	addres = ss.str() ;
	sdbDataSource *pDatasource = new sdbDataSource() ;
	vector<Worker> vec_workers ;
	vector<Worker>::iterator vec_workers_itr ;
	DS_WorderArgs ds_args ;
    ds_args.pDatasource = pDatasource ;
	ds_args.returnCode = 0 ;
	rc = pDatasource->init( addres, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	rc = pDatasource->enable() ;
	ASSERT_EQ( SDB_OK, rc ) ;
	
	for(INT32 i = 0; i < threadCount; ++i) 
	{
		Worker worker(ds_WorkerRoutine, &ds_args) ;
		vec_workers.push_back( worker ) ;
	}
	
	for(vec_workers_itr = vec_workers.begin(); 
	vec_workers_itr != vec_workers.end(); ++vec_workers_itr) 
	{
		vec_workers_itr->start() ;
	}
	
	for(vec_workers_itr = vec_workers.begin(); 
	vec_workers_itr != vec_workers.end(); ++vec_workers_itr) 
	{
		vec_workers_itr->waitStop() ;
	}
	
done:
	pDatasource->disable() ;
	pDatasource->close() ;
	delete pDatasource ;
	return ;
error:
	goto done ;
}
*/

