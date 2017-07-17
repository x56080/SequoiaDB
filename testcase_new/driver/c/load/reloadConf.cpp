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

int isMasterNode( sdbReplicaGroupHandle& rg, const char* host, const char* svc, bool* res )
{
	int rc = SDB_OK ;
    sdbNodeHandle master ;
	const char *host1, *svc1, *nodename1 ;
    int nodeId1 ;

    rc = sdbGetNodeMaster( rg, &master ) ;
    while( rc == SDB_CLS_NODE_NOT_EXIST )
    {
        rc = sdbGetNodeMaster( rg, &master ) ;
    }
    CHECK_RC( rc, "fail to get master node, rc = %d\n", rc ) ;
    rc = sdbGetNodeAddr( master, &host1, &svc1, &nodename1, &nodeId1 ) ;
    CHECK_RC( rc, "fail to get master node addr, rc = %d\n", rc ) ;

    if( strcmp(host, host1) == 0 && strcmp(svc, svc1) == 0 )
    	*res = true ;
	else
		*res = false ;

done:
	sdbReleaseNode( master ) ;
	return rc ;
error:
	goto done ;
}

// get a slave data node which is on the same machine with coord
int createSlaveNode( sdbConnectionHandle& db, sdbReplicaGroupHandle& rg, sdbNodeHandle& node, 
					   const char** host, const char** svc, const char** nodename, int* nodeId )
{
	int rc = SDB_OK ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson obj ;
    bson_init( &obj ) ;
	char installPath[20], dbpath[50], port[10] ;
	
	// list rg
	rc = sdbListReplicaGroups( db, &cursor ) ;
	CHECK_RC( rc, "fail to list rg, rc = %d\n", rc ) ;
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
		vector<string> vec ;
		rc = getGroupNodes( db, rgname, vec ) ;
		CHECK_RC( rc, "fail to get rg %s nodes, rc = %d\n", rgname, rc ) ;
		// if rg has only one node, after reelect and change primary node to new add node, 
		// then stop the primary node, group can't make elect
		if( vec.size() == 1 )  continue ;   
	
		rc = sdbGetReplicaGroup( db, rgname, &rg ) ;
        CHECK_RC( rc, "fail to get rg %s, rc = %d\n", rgname, rc ) ;
		break ;			
	}
	
	getHost() ;  // get local hostname
	getInstallPath( installPath ) ;
	getIdlePort( port ) ;
	printf( "idle port: %s\n", port ) ;
	sprintf( dbpath, "%s%s%s", installPath, "/database/data/", port ) ;
	rc = sdbCreateNode( rg, HOST, port, dbpath, NULL ) ;
	CHECK_RC( rc, "fail to create node %s:%s dbpath: %s, rc = %d\n", HOST, port, dbpath, rc ) ;
	rc = sdbGetNodeByHost( rg, HOST, port, &node ) ;
	CHECK_RC( rc, "fail to get node %s:%s, rc = %d\n", HOST, port, rc ) ;
	rc = sdbGetNodeAddr( node, host, svc, nodename, nodeId ) ;
	CHECK_RC( rc, "fail to get node addr, rc = %d\n", rc ) ;	

done:
	bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;
	return rc ;
error:
	goto done ;
}

int getLSN( sdbConnectionHandle& db, int64_t* offset, int* version )
{
	int rc = SDB_OK ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson sel; 
	bson_init( &sel ); 
	bson obj ;
    bson_init( &obj ) ;
	bson_iterator it, sub_it ;  

	bson_append_string( &sel, "CurrentLSN", "" ) ;
	bson_finish( &sel ) ; 
	rc = sdbGetSnapshot( db, SDB_SNAP_DATABASE, NULL, &sel, NULL, &cursor ) ;
	CHECK_RC( rc, "fail to get snapshot database, rc = %d\n", rc ) ;
	
	rc = sdbNext( cursor, &obj ) ;
	CHECK_RC( rc, "fail to get next, rc = %d\n", rc ) ;

	bson_find( &it, &obj, "CurrentLSN" ) ;
	// bson_print( &obj ) ;
	bson_iterator_subiterator( &it, &sub_it ) ;
	bson_iterator_next( &sub_it ) ;
	*offset = bson_iterator_long( &sub_it ) ;
	bson_iterator_next( &sub_it ) ;
    *version = bson_iterator_int( &sub_it ) ;

done:
	bson_destroy( &sel ) ;
	bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;
	return rc ;
error:
	goto done ;
} 

// wait sync finish, lsn is equal
int waitSync( sdbReplicaGroupHandle& rg, const char* host, const char* svc )
{
	int rc = SDB_OK ;
	sdbConnectionHandle db, db1 ;
	sdbNodeHandle master ;
	const char *host1, *svc1, *nodename1 ;
    int nodeId1 ;
	int64_t offset, offset1 ;                                                 
    int version, version1 ;

    rc = sdbGetNodeMaster( rg, &master ) ;
    CHECK_RC( rc, "fail to get master node, rc = %d\n", rc ) ;
    rc = sdbGetNodeAddr( master, &host1, &svc1, &nodename1, &nodeId1 ) ;
    CHECK_RC( rc, "fail to get master node addr, rc = %d\n", rc ) ;

	rc = sdbConnect( host, svc, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect node %s:%s, rc = %d\n", host, svc, rc ) ;
	rc = sdbConnect( host1, svc1, USER, PASSWD, &db1 ) ;
	CHECK_RC( rc, "fail to connect master node %s:%s, rc = %d\n", host1, svc1, rc ) ;
	
	do
	{
		rc = getLSN( db, &offset, &version ) ;
		CHECK_RC( rc, "fail to get lsn of node, rc = %d\n", rc ) ;
		rc = getLSN( db1, &offset1, &version1 ) ;
		CHECK_RC( rc, "fail to get lsn of master node, rc = %d\n", rc ) ;
	} while( offset != offset1 || version != version1 ) ;
	printf( "node offset: %ld,version: %d\n", offset, version ) ;
	printf( "master node offset: %ld,version: %d\n", offset1, version1 ) ; 

done:
	sdbDisconnect( db ) ;
	sdbDisconnect( db1 ) ;
	sdbReleaseConnection( db ) ;
	sdbReleaseConnection( db1 ) ;
	return rc ;
error:
	goto done ;
}

int changeNodeConf( const char* svc, const char* conf, int value )
{
	int rc = SDB_OK ;

	char installPath[20] ;
	getInstallPath( installPath ) ;
	
	char confFile[100] ;
	sprintf( confFile, "%s%s%s%s", installPath, "/conf/local/", svc, "/sdb.conf" ) ;
	FILE* fp = fopen( confFile, "r+" ) ;
	if( fp == NULL )
	{
		printf( "fail to open conf file: %s\n", confFile ) ;
		exit( 1 ) ;
	}
	
	char buffer[100] ;
	sprintf( buffer, "%s%s%d", conf, "=", value ) ;
	char s[100] ;
	int len = 0 ;
	while( fgets(s,sizeof(s),fp) != NULL )
	{
		len += strlen( s ) ;
		char* idx ;
		if( ( idx = strstr( s, conf ) ) != NULL )
		{
			len -= strlen( s ) ;
        	break ;
		}
	}
	if( fseek( fp, len, SEEK_SET ) != 0 )
	{
		printf( "fail to seek file,file: %s,offset: %d\n", confFile, len ) ;
		exit( 1 ) ;
	}
	fprintf( fp, "%s", buffer ) ;
	fclose( fp ) ;

	return rc ;
}

TEST( reloadConf, weight )
{
	int rc = SDB_OK ;
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;

    // connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
	if( isStandalone( db ) ) return ;

	// create a slave node
	sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
	sdbNodeHandle node = SDB_INVALID_HANDLE ;
	const char *host, *svc, *nodename ;
    int nodeId ;
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
