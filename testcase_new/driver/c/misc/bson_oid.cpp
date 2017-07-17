/**************************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-1285
* @Modify:      Liang xuewang Init
*			 	2017-02-28
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "../common/testcommon.hpp"
#include "../common/impWorker.hpp"

using namespace import ;

const char* csModName = "bsonTestCS" ;
char csName[100] ;
const char* clName = "bsonTestCL" ;
sdbConnectionHandle _db = SDB_INVALID_HANDLE ;
sdbCSHandle         _cs = SDB_INVALID_HANDLE ;
sdbCollectionHandle _cl = SDB_INVALID_HANDLE ;

int setup()
{
	int rc = SDB_OK ;
   	
   	getUniqueName( csModName, csName ) ;
   	rc = createNormalCl( &_db, &_cs, &_cl, csName, clName ) ;
   	CHECK_RC( rc, "fail to create normal cl, rc = %d\n", rc ) ;

done:
	return rc ;
error:
	goto done ;
}

int teardown()
{
   	int rc = SDB_OK ;
   	
   	rc = sdbDropCollectionSpace( _db, csName ) ;
   	CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", csName, rc ) ;
   	sdbDisconnect( _db ) ;
   	sdbReleaseCollection( _cl ) ;
   	sdbReleaseCS( _cs ) ;
   	sdbReleaseConnection( _db ) ;

done:
	return rc ;
error:
	goto done ;
}

class ThreadArgs : public WorkerArgs
{
public:
	int num ;
   	int tid ;
} ;

void bulkInsert( ThreadArgs* args )
{
   	int num = args->num ;
   	int tid = args->tid ;
   	int rc = SDB_OK ;
   	sdbConnectionHandle db ;
   	sdbCSHandle cs ;
   	sdbCollectionHandle cl ;

   	// connect and get cs cl
   	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to connect in thread " << tid ;
   	rc = sdbGetCollectionSpace( db, csName, &cs ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to get cs " << csName << " in thread " << tid ;
   	rc = sdbGetCollection1( cs, clName, &cl ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to get cl " << clName << " in thread " << tid ; 

   	// bulk insert record
   	int i = 0 ;
   	bson* rec[num] ;
   	while( i < num )
   	{
    	rec[i] = bson_create() ;
      	bson_append_int( rec[i], "a", i + tid * num ) ;
      	bson_finish( rec[i] ) ;
      	i++ ;
   	}
   	rc = sdbBulkInsert( cl, 0, rec, num ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to bulk insert in thread " << tid ;
   	i = 0 ;
   	while( i < num )
   	{
    	bson_dispose( rec[i] ) ;
	  	i++ ;
   	}

   	// disconnect and release handle
   	sdbDisconnect( db ) ;
   	sdbReleaseCollection( cl ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;
}

TEST( BsonTest, multiBulkInsert )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

   	int ThreadNum = 20 ;
   	int RecordNum = 100 ;
   	Worker* workers[ThreadNum] ;
   	ThreadArgs args[ThreadNum] ;
   	for( int i = 0;i < ThreadNum;i++ )
   	{
    	args[i].num = RecordNum ;
      	args[i].tid = i ;
      	workers[i] = new Worker( (WorkerRoutine)bulkInsert, &args[i], false ) ;
      	workers[i]->start() ;
   	}
   	for( int i = 0;i < ThreadNum;i++ )
   	{
      	workers[i]->waitStop() ;
   	}
   	SINT64 count = 0 ;
   	rc = sdbGetCount( _cl, NULL, &count ) ;
   	ASSERT_EQ( RecordNum * ThreadNum, count ) ;

	rc = teardown() ;
	ASSERT_EQ( rc, SDB_OK ) ;
}
