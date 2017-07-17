/******************************************************************************************
* @Description: test case because of ci problems
*				src testcase in trunk/testcases/hlt/lob_testcases/lobAbnormalTestcase.cpp
* 				test drop cs and write lob at the same time with multi thread
* @Modify:      Liang xuewang Init
*				2016-11-10
******************************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "../common/impWorker.hpp"
#include "../common/testcommon.hpp"

int createSplitCollection( sdbConnectionHandle* db, sdbCSHandle* cs, sdbCollectionHandle* cl,
							 const char* CsName, const char* ClName )
{
	int rc = SDB_OK ; 
	bson option ;
    bson_init(&option) ;	
	
	// connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, db ) ;
	CHECK_RC( rc, "fail to connect sdb, rc = %d\n", rc ) ;

	// create collection space
	rc = sdbCreateCollectionSpace( *db, CsName, SDB_PAGESIZE_4K, cs ) ;
	if( rc == SDB_DMS_CS_EXIST )
	{
		rc = sdbDropCollectionSpace( *db, CsName ) ;
		CHECK_RC( rc, "fail to drop existed cs %s, rc = %d\n", CsName, rc ) ;
		rc = sdbCreateCollectionSpace( *db, CsName, SDB_PAGESIZE_4K, cs ) ;
	}
	CHECK_RC( rc, "fail to create cs %s, rc = %d\n", CsName, rc ) ;

	// make options { "ShardingKey": { "no": -1 }, "ShardingType": "hash", "Partition": 1024, "ReplSize": 0 }
	bson_append_start_object( &option, "ShardingKey" ) ;
	bson_append_int( &option, "no", -1 ) ;
	bson_append_finish_object( &option ) ;
	bson_append_string( &option, "ShardingType", "hash" ) ;
	bson_append_int( &option, "Partition", 1024 ) ;
	bson_append_int( &option, "ReplSize", 0 ) ;
	bson_finish( &option ) ;
	// bson_print(&option) ;

	// create collection
	rc = sdbCreateCollection1( *cs, ClName, &option, cl ) ;
	CHECK_RC( rc, "fail to create cl %s, rc = %d\n", ClName, rc ) ;

done:
	bson_destroy(&option) ; 
	return rc ;
error:
	goto done ;
}

char* generateLob()
{
	int size = 24*1024*1024 ;
	char *lobBuffer = (char *)malloc(size) ;
	if( !lobBuffer )
	{
		printf( "out of memory for lobBuffer.\n" ) ;
		return NULL;
	}
	memset( lobBuffer, 'a', size ) ;
	return lobBuffer ;
}

class ThreadArg : public import::WorkerArgs
{
public:
	sdbCollectionHandle cl ;
	char *lobBuffer ;
	sdbConnectionHandle db ;
	const char *CsName ;
	sdbLobHandle lob ;
};

void func_lobWrite(ThreadArg *arg)
{
	sdbCollectionHandle cl = arg->cl ;
	char *lobBuffer = arg->lobBuffer ;
	sdbLobHandle lob = arg->lob ;
	int lobNum = 20 ;
	int rc = SDB_OK ;

	for( int i = 0;i < lobNum;++i )
	{
		rc = sdbWriteLob( lob, lobBuffer, strlen(lobBuffer) ) ;
		ASSERT_TRUE( rc == SDB_OK || rc == SDB_DMS_NOTEXIST || 
					 rc == SDB_RTN_CONTEXT_NOTEXIST ) << "fail to write lob, i = " << i << " rc = " << rc ;
		printf( "sdbWriteLob, rc = %d\n", rc ) ; 
	}

	printf( "over to excute lob write.\n" ) ;
}

void func_dropCs(ThreadArg *arg)
{
	ossSleep( getRand()*100 ) ;
	sdbConnectionHandle db = arg->db ;
	const char* CsName = arg->CsName ;
	int rc = SDB_OK ;
	
	rc = sdbDropCollectionSpace( db, CsName ) ;
	ASSERT_TRUE( rc == SDB_OK || rc == SDB_LOCK_FAILED ) << "fail to drop collection space, rc = " << rc ;
	printf( "over to excute drop cs.\n" ) ;
}

TEST(lobTest,dropCsAndWriteLob)
{
	sdbConnectionHandle db  = SDB_INVALID_HANDLE ;
	sdbCSHandle cs          = SDB_INVALID_HANDLE ;
	sdbCollectionHandle cl  = SDB_INVALID_HANDLE ;
	const char* CsModName   = "lxw_lobtest" ;
	char CsName[100] ;
	const char* ClName      = "dropAndWrite" ;
	int rc = SDB_OK ;

	// create collection
	getUniqueName( CsModName,CsName ) ;
	rc = createSplitCollection( &db, &cs, &cl, CsName, ClName ) ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// generate lob
	char *lobBuffer = generateLob() ;
	if( !lobBuffer )
	{
		return ;
	}

	// sdb open lob
	sdbLobHandle lob = SDB_INVALID_HANDLE ;
	bson_oid_t oid ;
	bson_oid_gen( &oid ) ;
	rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to open lob, rc = " << rc ;

	// create thread
	import::Worker *worker1,*worker2 ;
	ThreadArg arg ;
	arg.cl = cl ;
	arg.lobBuffer = lobBuffer ;
	arg.db = db ;
	arg.CsName = CsName ;
	arg.lob = lob ;
	worker1 = new import::Worker( (import::WorkerRoutine)func_lobWrite, &arg, false ) ;
	worker2 = new import::Worker( (import::WorkerRoutine)func_dropCs, &arg, false ) ;
	worker1->start() ;
	worker2->start() ;
	worker2->waitStop() ;
	worker1->waitStop() ;
	delete worker1 ;
	delete worker2 ;

	// sdb close lob
	rc = sdbCloseLob( &lob ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to close lob, rc = " << rc ;
	// release handle
	sdbDisconnect( db ) ;
	sdbReleaseCollection( cl ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseConnection( db ) ;
	free( lobBuffer ) ;
}
