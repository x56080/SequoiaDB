/***********************************************************
* @Description: testcase for c driver
*               concurrent test with multi cl
* @Modify:      Liang xuewang Init
*				2016-11-10
***********************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "../common/impWorker.hpp"
#include "../common/testcommon.hpp"

using import::Worker ;
using import::WorkerRoutine ;
using import::WorkerArgs ;

#define ThreadNum 5

sdbConnectionHandle db = SDB_INVALID_HANDLE ;
sdbCSHandle cs = SDB_INVALID_HANDLE ;
sdbCollectionHandle cl[ThreadNum] ;
const char* CsModName = "concurrentTestCs" ;
char CsName[100] ;
char* ClName[ThreadNum] ;

// run before all test case
int setup()
{
	int rc = SDB_OK ;

   	// connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect sdb in the beginning, rc = %d\n", rc ) ;

	// create cs
	getUniqueName( CsModName,CsName ) ;
	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
	CHECK_RC( rc, "fail to create cs %s, rc = %d\n", CsName, rc ) ;

	// make cl name
	for( int i = 0;i < ThreadNum;i++ )
	{
	   char temp[100] = "concurrentTestCl" ;
	   char number[10] ;
	   sprintf( number, "%d", i ) ;
	   strcat( temp, number ) ;
	   ClName[i] = strdup( temp ) ; 
	}

	// create cl 
	for( int i = 0;i < ThreadNum;i++ )
	{
	   rc = sdbCreateCollection( cs, ClName[i], &cl[i] ) ;
	   CHECK_RC( rc, "fail to create cl %s, rc = %d\n", ClName[i], rc ) ;
	}

done:
	return rc ;
error:
	goto done ;
}

// run after all test case
int teardown()
{
	int rc = SDB_OK ;

   	// drop cs
   	rc = sdbDropCollectionSpace( db, CsName ) ;
   	CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", CsName, rc ) ;

   	// release cl 
   	for( int i = 0;i < ThreadNum;i++ )
   	{
    	sdbReleaseCollection( cl[i] ) ;
      	free( ClName[i] ) ;
   	}

   	// disconnect
   	sdbDisconnect( db ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;

done:
	return rc ;
error:
	goto done ;
}

class ThreadArg : public WorkerArgs
{
public:
	sdbCollectionHandle cl ;    // cl handle
	int id ;				    // cl id
} ;

void func_cl( ThreadArg* arg )
{
	sdbCollectionHandle cl = arg->cl ;
	int i = arg->id ;
	int rc = SDB_OK ;
	
	// insert record { "a": i }
	bson record ;
	bson_init( &record ) ;
	bson_append_int( &record, "a", i ) ;
	bson_finish( &record ) ;
	rc = sdbInsert( cl, &record ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to insert record" ;
	
	// query record find( { "a": i }, { "a": "" } )
	bson select ;
	bson_init( &select ) ;
	bson_append_string( &select, "a", "" ) ;
	bson_finish( &select ) ;
	sdbCursorHandle cursor ;
	rc = sdbQuery( cl, &record, &select, NULL, NULL, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to query record" ;
	sdbReleaseCursor( cursor ) ;
	
	// update record update( { "$set": { "a": -1 } }, { "a": i } )
	bson update ;
	bson_init( &update ) ;
	bson_append_start_object( &update, "$set" ) ;
	bson_append_int( &update, "a", -1 ) ;
	bson_append_finish_object( &update ) ;
	bson_finish( &update ) ;
	rc = sdbUpdate( cl, &update, &record, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to update record" ;
	
	// query record find( { "a": -1 }, { "a": "" } )
	bson expect ;
	bson_init( &expect ) ;
	bson_append_int( &expect, "a", -1 ) ;
	bson_finish( &expect ) ;
	rc = sdbQuery( cl, &expect, &select, NULL, NULL, 0, -1, &cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to check update a:-1" ;
	
	// destroy bson
	bson_destroy( &record ) ;
	bson_destroy( &select ) ;
	bson_destroy( &update ) ;
	bson_destroy( &expect ) ;
	
	// close and release cursor
	rc = sdbCloseCursor( cursor ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to close cursor" ;
	sdbReleaseCursor( cursor ) ;
}

TEST( ConcurrentTest, Collection )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// create multi thread to operate different cl
	Worker * workers[ThreadNum] ;
	ThreadArg arg[ThreadNum] ;
	for( int i = 0;i < ThreadNum;++i )
	{
		arg[i].cl = cl[i] ;
		arg[i].id = i ; 
		workers[i] = new Worker( (WorkerRoutine)func_cl, &arg[i], false ) ;
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
