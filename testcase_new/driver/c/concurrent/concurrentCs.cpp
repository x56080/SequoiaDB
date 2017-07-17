/*************************************************************
* @Description: test case for c driver
*               concurrent test with multi cs
* @Modify:      Liang xuewang Init
*				2016-11-10
**************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "../common/impWorker.hpp"
#include "../common/testcommon.hpp"

using import::Worker ;
using import::WorkerRoutine ;
using import::WorkerArgs ;

#define ThreadNum 5

sdbConnectionHandle db = SDB_INVALID_HANDLE ;
sdbCSHandle cs[ThreadNum] ;
char* CsName[ThreadNum] ;

int setup()
{
	int rc = SDB_OK ;

   	// connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect sdb in the beginning, rc = %d\n", rc ) ;

	// make cs name
	for( int i = 0;i < ThreadNum;i++ )
	{
		char temp[100] = "concurrentTestCs" ;
	   	char number[10] ;
	   	sprintf( number, "%d", i ) ;
	   	strcat( temp, number ) ;
	   	char name[100] ;
	   	getUniqueName( temp,name ) ;
	   	CsName[i] = strdup( name ) ; 
	}

	// create cs
	for( int i = 0;i < ThreadNum;i++ )
	{
	   	rc = sdbCreateCollectionSpace( db, CsName[i], SDB_PAGESIZE_4K, &cs[i] ) ;
	   	CHECK_RC( rc, "fail to create cs %s, rc = %d\n", CsName[i], rc ) ;
	}

done:
	return rc ;
error:
	goto done ;
}

int teardown()
{
   	int rc = SDB_OK ;

   	// drop cs
   	for( int i = 0;i < ThreadNum;i++ )
   	{
      	rc = sdbDropCollectionSpace( db, CsName[i] ) ;
      	CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", CsName[i], rc ) ;
      	sdbReleaseCS( cs[i] ) ;
      	free( CsName[i] ) ;
   	}

  	// disconnect
   	sdbDisconnect( db ) ;
   	sdbReleaseConnection( db ) ;

done:
	return rc ;
error:
	goto done ;
}

class ThreadArg : public WorkerArgs
{
public:
	sdbCSHandle cs ;            // cs handle
	int id ;				    // cs id
} ;

void func_cs( ThreadArg* arg )
{
   	int i = arg->id ;
   	sdbCSHandle cs = arg->cs ;
   
   	sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   	char ClName[100] ;
   	sprintf( ClName, "%s%d", "concurrentTestCl", i ) ;
   
   	int rc = SDB_OK ;
   	// create cl
   	rc = sdbCreateCollection( cs, ClName, &cl ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cl " << ClName << " in cs " << i ;
   	// release cl before get cl
	sdbReleaseCollection( cl ) ;
   	// get cl
   	rc = sdbGetCollection1( cs, ClName, &cl ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to get cl " << ClName << " in cs " << i ;
   	// drop cl
   	rc = sdbDropCollection( cs, ClName ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to drop cl in cs " << i ;
   	// get cs name
   	char temp[100] ;
   	rc = sdbGetCSName( cs, temp, sizeof(temp) ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to get cs name of cs " << i ;
   	ASSERT_STREQ( temp, CsName[i] ) << "fail to check cs name of cs " << i ;     
   	// release cl
   	sdbReleaseCollection( cl ) ;
}

TEST( ConcurrentTest, Collectionspace )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

   	// create multi thread to operate different cl
	Worker * workers[ThreadNum] ;
	ThreadArg arg[ThreadNum] ;
	for( int i = 0;i < ThreadNum;++i )
	{
		arg[i].id = i ;
	   	arg[i].cs = cs[i] ;
		workers[i] = new Worker( (WorkerRoutine)func_cs, &arg[i], false ) ;
		workers[i]->start() ;
	}
	for( int i = 0;i < ThreadNum;++i )
	{
		workers[i]->waitStop() ;
		delete workers[i] ;
	}

	rc = teardown() ;
	ASSERT_EQ( rc, SDB_OK ) ;
}
