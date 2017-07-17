/**************************************************
* @Description: test case for c driver
*               ( manual test case,not in CI )
*			    concurrent test with multi domain
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
sdbDomainHandle domain[ThreadNum] = { 0 } ;
char* domainName[ThreadNum] ;
const char* groupName = "group1" ;

int setup()
{
	int rc = SDB_OK ;
	bson option ;
    bson_init( &option ) ;

   	// connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect sdb in the beginning, rc = %d\n", rc ) ;

	// make domain name
	for( int i = 0;i < ThreadNum;i++ )
	{
		char temp[100] = "concurrentTestDomain" ;
	   	char number[10] ;
	   	sprintf( number, "%d", i ) ;
	   	strcat( temp, number ) ;
	   	domainName[i] = strdup( temp ) ; 
	}

	// option { AutoSplit: true, Groups: [ groupName ] }
   	bson_append_bool( &option, "AutoSplit", true ) ;
   	bson_append_start_array( &option, "Groups" ) ;
   	bson_append_string( &option, "0", groupName ) ;
   	bson_append_finish_array( &option ) ;
   	bson_finish( &option ) ;
   	bson_print( &option ) ;

	// create domain with option
	for( int i = 0;i < ThreadNum;i++ )
	{
		rc = sdbCreateDomain( db, domainName[i], &option, &domain[i] ) ;
	   	CHECK_RC( rc, "fail to create doamin %s, rc = %d\n", domainName[i], rc ) ;
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

   	// drop domain
   	for( int i = 0;i < ThreadNum;i++ )
   	{
      	rc = sdbDropDomain( db, domainName[i] ) ;
      	CHECK_RC( rc, "fail to drop domain %s, rc = %d\n", domainName[i], rc ) ;
      	sdbReleaseDomain( domain[i] ) ;
      	free( domainName[i] ) ;
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
	int id ;	// rg id
} ;

void func_domain( ThreadArg* arg )
{
   	int i = arg->id ;
   	// printf( "i: %d\n", i ) ;
   	sdbDomainHandle dom = domain[i] ;
   	int rc = SDB_OK ;
   
   	bson option ;
   	bson_init( &option ) ;
   	bson_append_bool( &option, "AutoSplit", false ) ;
   	bson_finish( &option ) ;
   	rc = sdbAlterDomain( dom, &option ) ;
   	bson_destroy( &option ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to alter domain " << i ;
}

TEST( ConcurrentTest, Domain )
{
	int rc = SDB_OK ;
	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

   	// create multi thread to operate different domain
	Worker * workers[ThreadNum] ;
	ThreadArg arg[ThreadNum] ;
	for( int i = 0;i < ThreadNum;++i )
	{
		arg[i].id = i ;
		workers[i] = new Worker( (WorkerRoutine)func_domain, &arg[i], false ) ;
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
