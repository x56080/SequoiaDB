/*****************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-859
* @Modify:      Liang xuewang Init
*				2016-11-10
*****************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "../common/impWorker.hpp"
#include "../common/testcommon.hpp"

#define ThreadNum 10

char* CsName[ThreadNum] ;
char* ClName[ThreadNum] ;

sdbConnectionHandle db ;
sdbCSHandle cs[ThreadNum] = { 0 } ;
sdbCollectionHandle cl[ThreadNum] = { 0 } ;

int setup()
{
	int rc = SDB_OK ;
	sdbClientConf config ;
	bson option ;
    bson_init( &option ) ;
	
	// close cache
	config.enableCacheStrategy = 0 ;
    config.cacheTimeInterval = 0 ;
    rc = initClient( &config ) ;
	CHECK_RC( rc, "fail to init client, rc = %d\n", rc ) ;

	// make CsName
    for( int i = 0;i < ThreadNum;++i )
    {
        char temp[20] = "C_drivertest" ;
        char number[20] ;
        sprintf( number, "%d", i ) ;
        strcat( temp, number ) ;
        char name[100] ;
        getUniqueName( temp, name ) ;
        CsName[i] = strdup( name ) ;
    }

    // make ClName
    for( i = 0;i < ThreadNum;++i )
    {
        char temp[20] = "mutex_test" ;
        char number[20] ;
        sprintf( number, "%d", i ) ;
        strcat( temp, number ) ;
        ClName[i] = strdup( temp ) ;
    }

    // connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect sdb in the beginning, rc = %d\n", rc ) ;

	// create cs and cl
	// make option { "ReplSize": 0 }
	bson_append_int( &option, "ReplSize", 0 ) ;
	bson_finish( &option ) ;
	for( int i = 0;i < ThreadNum;++i )
	{
    	rc = sdbCreateCollectionSpace( db, CsName[i], SDB_PAGESIZE_4K, &cs[i] ) ;
    	CHECK_RC( rc, "fail to create cs %s, rc = %d\n", CsName, rc ) ;	
    	rc = sdbCreateCollection1( cs[i], ClName[i], &option, &cl[i] ) ;
		CHECK_RC( rc, "fail to create cl %s, rc = %d\n", ClName[i], rc ) ;
	}
	bson_destroy( &option ) ;

done:
	return rc ;
error:
	goto done ;
}

int teardown()
{
	int rc = SDB_OK ;

	for( int i = 0;i < ThreadNum;++i )
	{
		// drop cs 
		rc = sdbDropCollectionSpace( db, CsName[i] ) ;
		CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", CsName[i], rc ) ;
		// release handle
		sdbReleaseCollection( cl[i] ) ;
		sdbReleaseCS( cs[i] ) ;
		// free malloc space(strdup)
    	free( CsName[i] ) ;
    	free( ClName[i] ) ;
	}
	// disconnect
    sdbDisconnect( db ) ;
	// release handle
	sdbReleaseConnection( db ) ;

done:
	return rc ;
error:
	goto done ;
}

class ThreadArg : public import::WorkerArgs
{
public:
	sdbCollectionHandle cl ;	// collection
	int cid ;				    // collection id
} ;

// thread_function CRUD with cl
void func_cl( ThreadArg* arg )
{
	sdbCollectionHandle cl = arg->cl ;
	int i = arg->cid ;
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
	bson_append_finish_object(&update) ;
	bson_finish( &update ) ;
	rc = sdbUpdate( cl, &update, &record, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) ;

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

// thread_function query then close cursor
// main thread will disconnect between threads
void func_closeCursor1( ThreadArg *arg )
{
    sdbCollectionHandle cl = arg->cl ;
    int i = arg->cid ;
    int rc = SDB_OK ;
    
	// query record find( { "a": i }, { "a": "" } )
    bson record ;
    bson_init( &record ) ;
    bson_append_int( &record, "a", -1 ) ;
    bson_finish( &record ) ;
    sdbCursorHandle cursor ;
    rc = sdbQuery( cl, &record, NULL, NULL, NULL, 0, -1, &cursor ) ;
    ASSERT_TRUE( rc == SDB_OK || rc == SDB_NOT_CONNECTED || rc == SDB_NETWORK ) << "fail to query record, rc = " << rc ;
	printf( "thread %d, query record return: %d\n", i, rc ) ;
    bson_destroy( &record ) ;
   
	// close and release cursor
    rc = sdbCloseCursor( cursor ) ;
    ASSERT_TRUE( rc == SDB_OK || rc == SDB_INVALIDARG || rc == SDB_NETWORK ) << "fail to close cursor, rc = " << rc ;
	printf( "thread %d, close cursor return: %d\n", i, rc ) ;
    sdbReleaseCursor( cursor ) ;
}

// thread_function query then close cursor
// main thread will close all cursor between multi threads
void func_closeCursor2( ThreadArg *arg )
{
    sdbCollectionHandle cl = arg->cl ;
    int i = arg->cid ;
    int rc = SDB_OK ;
  
    // query record find( { "a": i }, { "a": "" } )
    bson record ;
    bson_init( &record ) ;
    bson_append_int( &record, "a", -1 ) ;
    bson_finish( &record ) ;
    sdbCursorHandle cursor ;
    rc = sdbQuery( cl, &record, NULL, NULL, NULL, 0, -1, &cursor ) ;
    ASSERT_TRUE( rc == SDB_OK || rc == SDB_NOT_CONNECTED ) << "fail to query record, rc = " << rc ;
	printf( "thread %d, query record return: %d\n", i, rc ) ;
    bson_destroy( &record ) ;

    // close and release cursor
    rc = sdbCloseCursor( cursor ) ;
    ASSERT_TRUE( rc == SDB_OK || rc == SDB_INVALIDARG ) << "fail to close cursor, rc = " << rc ;
	printf( "thread %d, close cursor return: %d\n", i, rc ) ;
    sdbReleaseCursor( cursor ) ;
}

// multi threads operate multi cs cl
// after threads close cursor and stop,main thread close all cursor and disconnect
TEST( SocketMutexTest, cl )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// create multi thread to operate different cl
	import::Worker * workers[ThreadNum] ;
	ThreadArg arg[ThreadNum] ;
	for( int i = 0;i < ThreadNum;++i )
	{
		arg[i].cl = cl[i] ;
		arg[i].cid = i ; 
		workers[i] = new import::Worker( (import::WorkerRoutine)func_cl, &arg[i], false ) ;
		workers[i]->start() ;
	}
	for( int i = 0;i < ThreadNum;++i )
	{
		workers[i]->waitStop() ;
		delete workers[i] ;
	}
	
	// close all cursors in the end
	rc = sdbCloseAllCursors( db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to close all cursor" ;
	sdbDisconnect( db ) ;
	for( int i = 0;i < ThreadNum;++i )
	{
		sdbReleaseCollection( cl[i] ) ;
		sdbReleaseCS( cs[i] ) ;
	}
	sdbReleaseConnection( db ) ;

	// after test,reconnect and get cs cl
    rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
    ASSERT_EQ( rc, SDB_OK ) ;
	for( int i = 0;i < ThreadNum;++i )
	{
    	rc = sdbGetCollectionSpace( db, CsName[i], &cs[i] ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to get cs " << CsName[i] ;
    	rc = sdbGetCollection1( cs[i], ClName[i], &cl[i] ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to get cl " << ClName[i] ;
	}
}

// diconnect between multi threads
TEST( SocketMutexTest, disconnect )
{
	int rc = SDB_OK ;

	// create multi thread to operate different cl
    import::Worker * workers[ThreadNum] ;
    ThreadArg arg[ThreadNum] ;
    for( int i = 0;i < ThreadNum;++i )
    {
        arg[i].cl = cl[i] ;
        arg[i].cid = i ;
        workers[i] = new import::Worker( (import::WorkerRoutine)func_closeCursor1, &arg[i], false ) ;
        workers[i]->start() ;
    }
    
    // thread 0-ThreadNum/2 close cursor before main thread disconnect
	for( int i = 0;i < ThreadNum/2;++i )
	{
    	workers[i]->waitStop() ;
    	delete workers[i] ;
	}
	
	// disconnect between threads
    sdbDisconnect( db ) ;

	// release handle
    for(int i = 0;i < ThreadNum;++i )
    {
        sdbReleaseCollection( cl[i] ) ;
        sdbReleaseCS( cs[i] ) ;
    }
    sdbReleaseConnection( db ) ;

	// thread ThreadNum/2-ThreadNum close cursor after main thread disconnect
	for(int i = ThreadNum/2;i < ThreadNum;++i )
    {
        workers[i]->waitStop() ;
        delete workers[i] ;
    }

    // after test,reconnect and get cs cl
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( rc, SDB_OK ) ;
	for(int i = 0;i < ThreadNum;++i )
	{
		rc = sdbGetCollectionSpace( db, CsName[i], &cs[i] ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to get cs " << CsName[i] ;
		rc = sdbGetCollection1( cs[i], ClName[i], &cl[i] ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to get cl " << ClName[i] ;
	}	
}

// close all cursors between multi threads
TEST( SocketMutexTest, closeAllCursor )
{
	int rc = SDB_OK ;
    
	// create multi thread to operate different cl
    import::Worker * workers[ThreadNum] ;
    ThreadArg arg[ThreadNum] ;
    for( int i = 0;i < ThreadNum;++i )
    {
        arg[i].cl = cl[i] ;
        arg[i].cid = i ;
        workers[i] = new import::Worker( (import::WorkerRoutine)func_closeCursor2, &arg[i], false ) ;
        workers[i]->start() ;
    }
	
    // thread 0-ThreadNum/2 close cursor before main thread closeAllCursor
    for(int i = 0;i < ThreadNum/2;++i )
    {
        workers[i]->waitStop() ;
        delete workers[i] ;
    }

	// close all cursors between threads
    rc = sdbCloseAllCursors( db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to close all cursor" ;

	// thread ThreadNum/2-ThreadNum close cursor after main thread closeAllCursor
	for( int i = ThreadNum/2;i < ThreadNum;++i )
    {
        workers[i]->waitStop() ;
        delete workers[i] ;
    }

	rc = teardown() ;
	ASSERT_EQ( rc, SDB_OK ) ;	
}
