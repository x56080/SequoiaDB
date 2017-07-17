/************************************************************************
*@Description : Test lob basic operation: write lob, read lob, seek read
*               lob and drop lob in collection.
*               Test get lob create time.
*@Modify List :
*               2016-11-21   Liang xuewang   Init
************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <malloc.h>
#include <time.h>
#include <sys/time.h>
#include "../common/testcommon.hpp"

TEST( lobNormalTest, BasicOperation )
{
	sdbConnectionHandle db  = SDB_INVALID_HANDLE ;
   	sdbCSHandle cs          = SDB_INVALID_HANDLE ;
   	sdbCSHandle cl          = SDB_INVALID_HANDLE ;
   	sdbLobHandle lob        = SDB_INVALID_HANDLE ;
   	const char* CsModName   = "lobNormalTestCs" ;
   	char CsName[100] ;
   	const char* ClName      = "lobNormalTestCl" ;
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
   
   	// create lob buffer(write and read)
   	int lobSize = 1024*1024*2 ;
   	char* lobWriteBuffer = ( char* )malloc( lobSize ) ;
   	char* lobReadBuffer  = ( char* )malloc( lobSize ) ;
   	if( !lobWriteBuffer || !lobReadBuffer )
	{
		printf( "out of memory for lob buffer.\n" ) ;
		return ;
	}
	memset( lobWriteBuffer, 'a', lobSize ) ;
	memset( lobReadBuffer, 0, lobSize ) ;
	
	// open lob,write lob,close lob
	bson_oid_t oid ;
	bson_oid_gen( &oid ) ;
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   	rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after write" ;
   
   	// open lob,get lobsize,close lob
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   	SINT64 size ;
   	rc = sdbGetLobSize( lob, &size ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to get lob size" ;
   	ASSERT_EQ( lobSize, size ) << "lob Size: " << lobSize
                              << "get lob Size: " << size ;
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after get lob size" ;
   
   	// open lob,read lob,close lob
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   	UINT32 readSize ;
   	rc = sdbReadLob( lob, lobSize, lobReadBuffer, &readSize ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   	ASSERT_EQ( lobSize, readSize ) << "lob Size: " << lobSize
                                  << "read lob Size: " << readSize ;
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after read" ;
   
   	// open lob,seek read lob,close lob
   	memset( lobReadBuffer, 0, lobSize ) ;
   	int seekSize = lobSize/2 + 31 ;
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   	rc = sdbSeekLob( lob, seekSize, SDB_LOB_SEEK_SET ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   	rc = sdbReadLob( lob, lobSize, lobReadBuffer, &readSize ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to read lob after seek" ;
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after seek read" ;
      
   	// remove lob in the end
   	rc = sdbRemoveLob( cl, &oid ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to remove lob" ;
   
   	// free buffer
   	free( lobWriteBuffer ) ;
   	free( lobReadBuffer ) ;
   
   	// drop cs,release handle
   	rc = sdbDropCollectionSpace( db, CsName ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs in the end" ;
   	sdbReleaseCollection( cl ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;
}


TEST( lobNormalTest, getLobCreateTime )
{
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   	sdbCSHandle cs         = SDB_INVALID_HANDLE ;
   	sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   	sdbLobHandle lob       = SDB_INVALID_HANDLE ;
   	const char* CsModName  = "lobNormalTestCs" ;
   	char CsName[100] ;
   	const char* ClName     = "lobNormalTestCl" ;
   	int rc = SDB_OK ;
   
   	// connect sdb,create cs cl
   	getConf() ;
   	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
   	getUniqueName( CsModName,CsName ) ;
   	rc = sdbCreateCollectionSpace( db, CsName, SDB_PAGESIZE_4K, &cs ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cs " << CsName ;
   	rc = sdbCreateCollection( cs, ClName, &cl ) ;
   	ASSERT_EQ( rc, SDB_OK ) << "fail to create cl" ;
   
   	// create lob buffer
   	int lobSize = 1024*1024*2 ;
   	char* lobWriteBuffer = ( char* )malloc( lobSize ) ;
   	if( !lobWriteBuffer )
	{
		printf( "out of memory for lob buffer.\n" ) ;
		return ;
	}
	memset( lobWriteBuffer, 'a', lobSize ) ;
	
	// get system current time
   	struct timeval tv ;
   	gettimeofday( &tv, NULL ) ;
   
   	// open lob,write lob,close lob
   	bson_oid_t oid ;
   	bson_oid_gen( &oid ) ;
	rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with create mode" ;
   	rc = sdbWriteLob( lob, lobWriteBuffer, lobSize ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to write lob";
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after write" ;
   
   	// open lob,get lob create time,close lob
   	rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to open lob with read mode" ;
   	UINT64 createtime ;
   	rc = sdbGetLobCreateTime( lob, &createtime ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to get lob create time" ;
   	rc = sdbCloseLob( &lob ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to close lob after get lob create time" ;
   
   	// compare system time and lob create time
   	UINT64 systime = tv.tv_sec * 1000 + tv.tv_usec / 1000 ;
   	printf( "SystemTime= %ld\nCreateTime= %ld\n", systime, createtime ) ;
   	ASSERT_LE( systime, createtime ) << "fail to check lob create time" ;
   
   	// free buffer
   	free( lobWriteBuffer ) ;
   
   	// drop cs,release handle
   	rc = sdbDropCollectionSpace( db, CsName ) ;
   	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs in the end" ;
   	sdbReleaseCollection( cl ) ;
   	sdbReleaseCS( cs ) ;
   	sdbReleaseConnection( db ) ;
}
