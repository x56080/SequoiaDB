/************************************************************************
*@Description : Test lob abnormal use:not exist lob,not closed lob,write
*               lob with read mode.
*@Modify List :
*               2016-11-21   Liang xuewang   Init
************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <malloc.h>
#include "../common/testcommon.hpp"

TEST( lobAbnormalTest, NotExistLob )
{
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   	sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   	sdbCSHandle cl         = SDB_INVALID_HANDLE ;
   	sdbLobHandle lob       = SDB_INVALID_HANDLE ;
   	const char* CsModName  = "lobAbnormalTestCs" ;
   	char CsName[100] ;
   	const char* ClName     = "lobAbnormalTestCl" ;
   	int rc = SDB_OK ;
   
   	// connect sdb,create cs cl
   	getConf() ;
   	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
   	getUniqueName( CsModName,CsName ) ;
   	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cs " << CsName ;
   	rc = sdbCreateCollection( cs, ClName, &cl ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cl " << ClName ; 
   
   	// read not exist lob
   	bson_oid_t oid ;
   	bson_oid_gen( &oid ) ;
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   	ASSERT_EQ( SDB_FNE, rc ) << "fail to test read not exist lob" ;
   
   	// get not exist lob size
   	SINT64 size ;
   	rc = sdbGetLobSize( lob, &size ) ;
   	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test get not exist lob size" ;
   
   	// get not exist lob create time
   	UINT64 mills ;
   	rc = sdbGetLobCreateTime( lob, &mills ) ;
   	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test get not exist lob create time" ;
   
   	// close not exist lob
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test close not exist lob" ;
   
   	// remove not exist lob
   	rc = sdbRemoveLob( lob, &oid ) ;
   	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to remove not exist lob" ;   

   	// drop cs,release handle
   	rc = sdbDropCollectionSpace( db, CsName ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs in the end" ;
   	sdbReleaseCollection( cl ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;
}

TEST( lobAbnormalTest, NotClosedLob )
{
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   	sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   	sdbCSHandle cl         = SDB_INVALID_HANDLE ;
   	sdbLobHandle lob       = SDB_INVALID_HANDLE ;
   	const char* CsModName  = "lobAbnormalTestCs" ;
   	char CsName[100] ;
   	const char* ClName     = "lobAbnormalTestCl" ;
   	int rc = SDB_OK ;
   
   	// connect sdb,create cs cl
   	getConf() ;
   	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
   	getUniqueName( CsModName,CsName ) ;
   	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cs " << CsName ;
   	rc = sdbCreateCollection( cs, ClName, &cl ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cl " << ClName ; 
   
   	// create lob buffer
   	int lobSize = 1024*1024*2 ;
   	char* lobWriteBuffer = ( char* )malloc( lobSize ) ;
   	if( !lobWriteBuffer )
	{
		printf( "out of memory for lob buffer.\n" ) ;
		return ;
	}
	memset( lobWriteBuffer, 'a', lobSize ) ;
	
	// open lob,write lob,without close lob
   	bson_oid_t oid ;
   	bson_oid_gen( &oid ) ;
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   	rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to write lob"  ;
   
   	// read lob when lob is not closed
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   	ASSERT_EQ( SDB_LOB_IS_NOT_AVAILABLE, rc ) << "fail to test open lob when lob is not closed" ;
   
   	// remove lob when lob is not closed
   	rc = sdbRemoveLob( cl, &oid ) ;
   	ASSERT_EQ( SDB_LOB_IS_NOT_AVAILABLE, rc ) << "fail to test remove lob when lob is not closed" ;
   
   	// close lob
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to close lob in the end" ;
   
   	// free buffer
   	free( lobWriteBuffer ) ;
   
   	// drop cs,release handle
   	rc = sdbDropCollectionSpace( db, CsName ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs in the end" ;
   	sdbReleaseCollection( cl ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;
}

TEST( lobAbnormalTest, WriteLobWithReadMode )
{
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   	sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   	sdbCSHandle cl         = SDB_INVALID_HANDLE ;
   	sdbLobHandle lob       = SDB_INVALID_HANDLE ;
   	const char* CsModName  = "lobAbnormalTestCs" ;
   	char CsName[100] ;
   	const char* ClName     = "lobAbnormalTestCl" ;
   	int rc = SDB_OK ;
   
   	// connect sdb,create cs cl
   	getConf() ;
   	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
   	getUniqueName( CsModName,CsName ) ;
   	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cs " << CsName ;
   	rc = sdbCreateCollection( cs, ClName, &cl ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cl " << ClName ;
   
   	// create lob buffer
   	int lobSize = 1024*1024*2 ;
   	char* lobWriteBuffer = ( char* )malloc( lobSize ) ;
   	if( !lobWriteBuffer )
	{
		printf( "out of memory for lob buffer.\n" ) ;
		return ;
	}
	memset( lobWriteBuffer, 'a', lobSize ) ; 

   	// open lob,write lob,close lob
   	bson_oid_t oid ;
   	bson_oid_gen( &oid ) ;
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   	rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   
   	// write lob with read mode
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   	rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   	ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test write lob with read mode" ;
   
   	// close lob
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob in the end" ;
   
   	// free buffer
   	free( lobWriteBuffer ) ;
   
   	// drop cs,release handle
   	rc = sdbDropCollectionSpace( db, CsName ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs in the end" ;
   	sdbReleaseCollection( cl ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;
}
