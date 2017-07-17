/**************************************************
* @Description: test case for c driver
*				concurrent test with multi lob
* @Modify:      Liang xuewang Init
*				2016-11-10
**************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "../common/impWorker.hpp"
#include "../common/testcommon.hpp"

using import::Worker ;
using import::WorkerRoutine ;
using import::WorkerArgs ;

#define ThreadNum 5

sdbConnectionHandle db = SDB_INVALID_HANDLE ;
sdbCSHandle cs 		   = SDB_INVALID_HANDLE ;
sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
const char* CsModName  = "concurrentTestCs" ;
char CsName[100] ;
const char* ClName 	   = "concurrentTestCl" ;
sdbLobHandle lob[ThreadNum] ;
bson_oid_t oid[ThreadNum] ;

int setup()
{
	int rc = SDB_OK ;

	// create cs
	getUniqueName( CsModName,CsName ) ;
	rc = createNormalCl( &db, &cs, &cl, CsName, ClName ) ;
	CHECK_RC( rc, "fail to create normal cl, rc = %d\n", rc ) ;

	// open lob
	for( int i = 0;i < ThreadNum;i++ )
	{
	   bson_oid_gen( &oid[i] ) ;
	   rc = sdbOpenLob( cl, &oid[i], SDB_LOB_CREATEONLY, &lob[i] ) ;
	   CHECK_RC( rc, "fail to open lob, rc = %d\n", rc ) ;
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
   	rc = sdbDropCollectionSpace( db, CsName ) ;
   	CHECK_RC( rc, "fail to drop cs %s, rc = %d\n", CsName, rc ) ;

   	// disconnect
   	sdbDisconnect( db ) ;
   	sdbReleaseCollection( cl ) ;
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
	sdbLobHandle lob ;          // lob handle
	int id ;				    // lob id
} ;

void func_lobWrite( ThreadArg* arg )
{
	sdbLobHandle lob = arg->lob ;
   	int i = arg->id ;
   	int rc = SDB_OK ;
   
   	int size = 24*1024*1024 ;
	char *lobBuffer = ( char * ) malloc ( size ) ;
	if( !lobBuffer )
	{
		printf( "out of memory for lobBuffer in lobWrite.\n" ) ;
		return ;
	}
	memset( lobBuffer, 'a', size ) ;
	
	// write lob
	rc = sdbWriteLob( lob, lobBuffer, size ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to write lob " << i ;
	// close lob
	rc = sdbCloseLob( &lob ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to close lob " << i ;
	
	free( lobBuffer ) ;
}

void func_lobRead( ThreadArg* arg )
{
   	sdbLobHandle lob = arg->lob ;
   	int i = arg->id ;
   	int rc = SDB_OK ;
   
   	int size = 10*1024*1024 ;
	char* lobBuffer = ( char* ) calloc ( size, sizeof(char) ) ;
	if( !lobBuffer )
	{
		printf( "out of memory for lobBuffer in lobRead.\n" ) ;
		return ;
	}
	unsigned int readlen = 0 ;
	
	// read lob
	rc = sdbReadLob( lob, size, lobBuffer, &readlen ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to read lob " << i ;
	ASSERT_EQ( size, readlen ) << "fail to check read lob length,i = " << i ;
	// close lob
	rc = sdbCloseLob( &lob ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to close lob " << i ;
	
	free( lobBuffer ) ;
}

TEST( ConcurrentTest, Lob )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

   	// create multi thread to operate different lob write
	Worker * workers[ThreadNum] ;
	ThreadArg arg[ThreadNum] ;
	for( int i = 0;i < ThreadNum;++i )
	{
		arg[i].lob = lob[i] ;
		arg[i].id = i ; 
		workers[i] = new Worker( (WorkerRoutine)func_lobWrite, &arg[i], false ) ;
		workers[i]->start() ;
	}
	for( int i = 0;i < ThreadNum;++i )
	{
		workers[i]->waitStop() ;
		delete workers[i] ;
	}

	// open lob with read mode
	for( int i = 0;i < ThreadNum;++i )
	{
	   	rc = sdbOpenLob( cl, &oid[i], SDB_LOB_READ, &lob[i] ) ;
	   	ASSERT_EQ( rc, SDB_OK ) << "fail to open lob with read mode,i = " << i ;
	}

	// create multi thread to operate different lob read
	for( int i = 0;i < ThreadNum;++i )
	{
		arg[i].lob = lob[i] ;
		arg[i].id = i ; 
		workers[i] = new Worker( (WorkerRoutine)func_lobRead, &arg[i], false ) ;
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
