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

const char* user   = "" ;
const char* passwd = "" ;
const char* csModName = "bsonTestCS" ;
char csName[100] ;
const char* clName = "bsonTestCL" ;
sdbConnectionHandle _db = SDB_INVALID_HANDLE ;
sdbCSHandle         _cs = SDB_INVALID_HANDLE ;
sdbCollectionHandle _cl = SDB_INVALID_HANDLE ;

#define ASSERT_RC_CODE( rc, msg ) \
if( rc != SDB_OK ) \
{ \
   printf( "%s,rc=%d\n", msg, rc ) ; \
   exit(1) ; \
}

class BsonTest : public testing::Test
{
public:
	static void SetUpTestCase() ;
	static void TearDownTestCase() ;
} ;

void BsonTest::SetUpTestCase()
{
   INT32 rc = SDB_OK ;
   // connect sdb
   getConf() ;
   rc = sdbConnect( HOSTNAME, SVCNAME, user, passwd, &_db ) ;
   ASSERT_RC_CODE( rc, "fail to connect sdb" )
   // create cs cl
   getUniqueName( csModName, csName ) ;
   rc = sdbCreateCollectionSpace( _db, csName, SDB_PAGESIZE_4K, &_cs ) ;
   ASSERT_RC_CODE( rc, "fail to create cs" )
   rc = sdbCreateCollection( _cs, clName, &_cl ) ;
   ASSERT_RC_CODE( rc, "fail to create cl" )      
}

void BsonTest::TearDownTestCase()
{
   INT32 rc = SDB_OK ;
   // drop cs
   rc = sdbDropCollectionSpace( _db, csName ) ;
   ASSERT_RC_CODE( rc, "fail to drop cs" )
   // disconnect and release handle
   sdbDisconnect( _db ) ;
   sdbReleaseCollection( _cl ) ;
   sdbReleaseCS( _cs ) ;
   sdbReleaseConnection( _db ) ;
}

class ThreadArgs : public WorkerArgs
{
public:
   INT32 num ;
   INT32 tid ;
} ;

void bulkInsert( ThreadArgs* args )
{
   INT32 num = args->num ;
   INT32 tid = args->tid ;
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   // connect and get cs cl
   rc = sdbConnect( HOSTNAME, SVCNAME, user, passwd, &db ) ;
   ASSERT_RC_CODE( rc, "fail to connect in thread" )
   rc = sdbGetCollectionSpace( db, csName, &cs ) ;
   ASSERT_RC_CODE( rc, "fail to get cs in thread" )
   rc = sdbGetCollection1( cs, clName, &cl ) ;
   ASSERT_RC_CODE( rc, "fail to get cl in thread" )  

   // bulk insert record
   INT32 i = 0 ;
   bson* rec[num] ;
   while( i < num )
   {
      rec[i] = bson_create() ;
      bson_append_int( rec[i], "a", i + tid * num ) ;
      bson_finish( rec[i] ) ;
      i++ ;
   }
   rc = sdbBulkInsert( cl, 0, rec, num ) ;
   ASSERT_RC_CODE( rc, "fail to bulk insert" )
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

TEST_F( BsonTest, multiBulkInsert )
{
   INT32 rc = SDB_OK ;
   INT32 ThreadNum = 20 ;
   INT32 RecordNum = 100 ;
   Worker* workers[ThreadNum] ;
   ThreadArgs args[ThreadNum] ;
   for( INT32 i = 0;i < ThreadNum;i++ )
   {
      args[i].num = RecordNum ;
      args[i].tid = i ;
      workers[i] = new Worker( (WorkerRoutine)bulkInsert, &args[i], false ) ;
      workers[i]->start() ;
   }
   for( INT32 i = 0;i < ThreadNum;i++ )
   {
      workers[i]->waitStop() ;
   }
   SINT64 count = 0 ;
   rc = sdbGetCount( _cl, NULL, &count ) ;
   ASSERT_EQ( RecordNum * ThreadNum, count ) ;
}
