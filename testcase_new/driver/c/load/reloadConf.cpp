/**************************************************************
* @Description: test case for Jira questionaire Task
*				SEQUOIADBMAINSTREAM-2165
*				seqDB-11001:reloadConf
*               修改数据组备节点的配置文件weight=20
*				重新选主，检查备节点升级为主节点
* @Modify     : Liang xuewang Init
*			 	2017-01-22
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "../common/testcommon.hpp"

/*
#define CHECK_RC_CODE( rc, msg ) \
if( rc != SDB_OK ) \
{ \
	printf( "%s,rc = %d\n", msg, rc ) ; \
	return rc ; \
}

void getInstallPath( char* path )
{
	const char* installFile = "/etc/default/sequoiadb" ;
	FILE* fp = fopen( installFile, "r" ) ;
	if( fp == NULL )
	{
		printf( "fail to open file /etc/default/sequoiadb\n" ) ;
		exit(1) ;
	}
	char s[50] ;
	while( fgets(s,sizeof(s),fp) != NULL  )
	{
		char* idx ;
		if( (idx=strstr(s,"INSTALL_DIR=")) == NULL )
			continue ;
		strcpy( path, idx+12 ) ;
		break ;
	}
	fclose( fp ) ;
	int len = strlen(path) ;
	path[len-1] = '\0' ;
	if( strcmp(path, "") == 0 )
	{
		printf( "fail to get install path\n" ) ;
		exit(1) ;
	}
}

INT32 isMasterNode( sdbReplicaGroupHandle& rg, const char* host, const char* svc, bool* res )
{
	INT32 rc = SDB_OK ;

    sdbNodeHandle master ;
    rc = sdbGetNodeMaster( rg, &master ) ;
    CHECK_RC_CODE( rc, "fail to get master node in function isMasterNode" ) ;
    const char *host1, *svc1, *nodename1 ;
    INT32 nodeId1 ;
    rc = sdbGetNodeAddr( master, &host1, &svc1, &nodename1, &nodeId1 ) ;
    CHECK_RC_CODE( rc, "fail to get master node addr in function isMasterNode" ) ;

    if( strcmp(host, host1) == 0 && strcmp(svc, svc1) == 0 )
    	*res = true ;
	else
		*res = false ;

	sdbReleaseNode( master ) ;
	return rc ;
}

// get a slave data node which is on the same machine with coord
INT32 createSlaveNode( sdbConnectionHandle& db, sdbReplicaGroupHandle& rg, sdbNodeHandle& node, 
					   const char** host, const char** svc, const char** nodename, INT32* nodeId )
{
	INT32 rc = SDB_OK ;
	
	// list rg
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	rc = sdbListReplicaGroups( db, &cursor ) ;
	CHECK_RC_CODE( rc, "fail to list rg" ) ;
	bson obj ;	
	bson_init( &obj ) ;
	while( sdbNext( cursor, &obj ) == SDB_OK )
	{
		bson_iterator it ;
		bson_find( &it, &obj, "GroupName" ) ;
		const char* rgname = bson_iterator_string( &it ) ;
		if( strcmp(rgname, "SYSCoord") == 0 || strcmp(rgname, "SYSCatalogGroup") == 0 )
		{
			bson_destroy( &obj ) ;
			bson_init( &obj ) ;
			continue ;
		}

		rc = sdbGetReplicaGroup( db, rgname, &rg ) ;
        CHECK_RC_CODE( rc, "fail to get rg" ) ;
		break ;			
	}
	
	getHost() ;  // get local hostname
	char installPath[20], dbpath[50], port[10] ;
	getInstallPath( installPath ) ;
	getIdlePort( port ) ;
	printf( "idle port: %s\n", port ) ;
	sprintf( dbpath, "%s%s%s", installPath, "/database/data/", port ) ;
	rc = sdbCreateNode( rg, HOST, port, dbpath, NULL ) ;
	CHECK_RC_CODE( rc, "fail to create a slave node" ) ;
	rc = sdbGetNodeByHost( rg, HOST, port, &node ) ;
	CHECK_RC_CODE( rc, "fail to get a slave node" ) ;
	rc = sdbGetNodeAddr( node, host, svc, nodename, nodeId ) ;
	CHECK_RC_CODE( rc, "fail to get node addr" ) ;	

	bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;
	return rc ;
}

INT32 getLSN( sdbConnectionHandle& db, int64_t* offset, int* version )
{
	INT32 rc = SDB_OK ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	
	bson sel; 
	bson_init( &sel ); 
	bson_append_string( &sel, "CurrentLSN", "" ) ;
	bson_finish( &sel ) ; 
	rc = sdbGetSnapshot( db, SDB_SNAP_DATABASE, NULL, &sel, NULL, &cursor ) ;
	bson_destroy( &sel ) ;
	CHECK_RC_CODE( rc, "fail to get snapshot database" ) ;
	
	bson obj ;
	bson_init( &obj ) ;
	rc = sdbNext( cursor, &obj ) ;
	CHECK_RC_CODE( rc, "fail to get next in snapshot database" ) ;

	bson_iterator it, sub_it ;
	bson_find( &it, &obj, "CurrentLSN" ) ;
	// bson_print( &obj ) ;
	bson_iterator_subiterator( &it, &sub_it ) ;
	bson_iterator_next( &sub_it ) ;
	*offset = bson_iterator_long( &sub_it ) ;
	bson_iterator_next( &sub_it ) ;
    *version = bson_iterator_int( &sub_it ) ;

	bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;
	return SDB_OK ;
} 

// wait sync finish, lsn is equal
INT32 waitSync( sdbReplicaGroupHandle& rg, const char* host, const char* svc )
{
	INT32 rc = SDB_OK ;
	sdbConnectionHandle db, db1 ;
	sdbNodeHandle master ;

    rc = sdbGetNodeMaster( rg, &master ) ;
    CHECK_RC_CODE( rc, "fail to get master node in function isMasterNode" ) ;
    const char *host1, *svc1, *nodename1 ;
    INT32 nodeId1 ;
    rc = sdbGetNodeAddr( master, &host1, &svc1, &nodename1, &nodeId1 ) ;
    CHECK_RC_CODE( rc, "fail to get master node addr in function isMasterNode" ) ;

	rc = sdbConnect( host, svc, USER, PASSWD, &db ) ;
	CHECK_RC_CODE( rc, "fail to connect node" ) ;
	rc = sdbConnect( host1, svc1, USER, PASSWD, &db1 ) ;
	CHECK_RC_CODE( rc, "fail to connect master node" ) ;
	int64_t offset, offset1 ;
	int version, version1 ;
	do
	{
		rc = getLSN( db, &offset, &version ) ;
		CHECK_RC_CODE( rc, "fail to get lsn of node" ) ;
		rc = getLSN( db1, &offset1, &version1 ) ;
		CHECK_RC_CODE( rc, "fail to get lsn of master node" ) ;
	} while( offset != offset1 || version != version1 ) ;
	printf( "node offset: %ld,version: %d\n", offset, version ) ;
	printf( "master node offset: %ld,version: %d\n", offset1, version1 ) ; 

	sdbDisconnect( db ) ;
	sdbDisconnect( db1 ) ;
	sdbReleaseConnection( db ) ;
	sdbReleaseConnection( db1 ) ;
	return SDB_OK ;
}

INT32 changeNodeConf( const char* svc, const char* conf, int value )
{
	INT32 rc = SDB_OK ;

	char installPath[20] ;
	getInstallPath( installPath ) ;
	
	char confFile[100] ;
	sprintf( confFile, "%s%s%s%s", installPath, "/conf/local/", svc, "/sdb.conf" ) ;
	FILE* fp = fopen( confFile, "r+" ) ;
	if( fp == NULL )
	{
		printf( "fail to open conf file: %s\n", confFile ) ;
		exit(1) ;
	}
	
	char buffer[100] ;
	sprintf( buffer, "%s%s%d", conf, "=", value ) ;
	char s[100] ;
	int len = 0 ;
	while( fgets(s,sizeof(s),fp) != NULL )
	{
		len += strlen(s) ;
		char* idx ;
		if( (idx=strstr(s,conf)) != NULL )
		{
			len -= strlen(s) ;
        	break ;
		}
	}
	if( fseek(fp,len,SEEK_SET) != 0 )
	{
		printf( "fail to seek file,file: %s,offset: %d\n", confFile, len ) ;
		exit(1) ;
	}
	fprintf( fp, "%s", buffer ) ;
	fclose( fp ) ;

	return rc ;
}

TEST( reloadConf, weight )
{
	INT32 rc = SDB_OK ;
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;

    // connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
	if( isStandalone(db) ) return ;

	// create a slave node
	sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
	sdbNodeHandle node = SDB_INVALID_HANDLE ;
	const char *host, *svc, *nodename ;
    INT32 nodeId ;
	rc = createSlaveNode( db, rg, node, &host, &svc, &nodename, &nodeId ) ;
	ASSERT_EQ( rc, SDB_OK ) ;
	printf( "node: name %s,svc %s,nodename %s,nodeId %d\n", host, svc, nodename, nodeId ) ;

	// start node and wait sync finish
	rc = sdbStartNode( node ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to start node" ;
	rc = waitSync( rg, host, svc ) ;
	ASSERT_EQ( rc, SDB_OK ) ;
	
	// change slave node weight to 20
	rc = changeNodeConf( svc, "weight", 20 ) ;
	ASSERT_EQ( rc, SDB_OK ) ;

 	// reload conf
    rc = sdbReloadConfig( db, NULL ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to reload conf" ;

	// reelect and check master
	rc = sdbReelect( rg, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to reelect in rg" ;
	bool isMaster = false ;
	rc = isMasterNode( rg, host, svc, &isMaster ) ;
	ASSERT_EQ( rc, SDB_OK ) ;
	ASSERT_TRUE( isMaster ) << "fail to check node to be master after reelect" ;	

	// stop and remove node
	rc = sdbStopNode( node ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to stop node after test" ;
	do
	{
		rc = isMasterNode( rg, host, svc, &isMaster ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to check node is master node or not" ;
	} while( isMaster ) ;
	rc = sdbRemoveNode( rg, host, svc, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to remove node after test" ;

	// disconnect and release
	sdbDisconnect( db ) ;
	sdbReleaseConnection( db ) ;
	sdbReleaseReplicaGroup( rg ) ;
	sdbReleaseNode( node ) ;
}
*/