/************************************************************
* @Description: test case for Jira questionaire
*               ( manual test case,not in CI ) 
*				SEQUOIADBMAINSTREAM-809
* @Modify:      Liang xuewang Init
*				2016-11-11
*************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/testcommon.hpp"

sdbConnectionHandle db = 0 ;
sdbReplicaGroupHandle dataRG = 0,tempRG = 0 ;
sdbNodeHandle tempNode = 0 ;

int setup()
{
    int rc = SDB_OK ;

    // connect to sdb
    getConf() ;
    rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
    CHECK_RC( rc, "fail to connect sdb, rc = %d\n", rc ) ;	

done:
	return rc ;
error:
	goto done ;
}

int teardown()
{
	int rc = SDB_OK ;

	// disconnect and release handle
    sdbDisconnect( db ) ;
    sdbReleaseNode( tempNode ) ;
    sdbReleaseReplicaGroup( tempRG ) ;
    sdbReleaseReplicaGroup( dataRG ) ;
    sdbReleaseConnection( db ) ;

done:
	return rc ;
error:
	goto done ;
}

TEST( AttachAndDetachNodeTest, onlyAttachAndOnlyDetach )
{
	int rc = SDB_OK ;
	vector<string> groups ;
	const char* rgname ;
	char tempNodeSvcName[10] ;
	char tempNodeDbPath[100] ;

	rc = setup() ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// check standalone
	if( isStandalone( db ) )
	{
		printf( "sdb connection is standalone.\n" ) ;
		sdbDisconnect( db ) ;
		sdbReleaseConnection( db ) ;
		return ;
	}

	// get data group dataRG
	rc = getGroups( db, groups ) ;
	ASSERT_EQ( rc, SDB_OK ) ;
	ASSERT_GT( groups.size(), 0 ) << "no data group" ;
	rgname = groups[0].c_str() ;
	rc = sdbGetReplicaGroup( db, rgname, &dataRG ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get rg " << rgname ;

	// create tempNode
	getIdlePort( tempNodeSvcName ) ;
	printf( "temp node svcname: %s\n", tempNodeSvcName ) ;
	
	strcpy( tempNodeDbPath, RSRVNODEDIR ) ;
	strcat( tempNodeDbPath, tempNodeSvcName ) ;
	printf( "temp node dbpath: %s\n", tempNodeDbPath ) ;
    
	getHost() ;
	rc = sdbCreateNode( dataRG, HOST, tempNodeSvcName, tempNodeDbPath, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create tempNode" ;
	
	// detach tempNode from dataRG
	rc = sdbDetachNode( dataRG, HOST, tempNodeSvcName, NULL ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to detach tempNode" ;
	rc = sdbGetNodeByHost( dataRG, HOST, tempNodeSvcName, &tempNode ) ;
	ASSERT_EQ( rc, SDB_CLS_NODE_NOT_EXIST ) << "fail to check detach" ;
	
	// create tempRG
    rc = sdbCreateReplicaGroup( db, "temp", &tempRG ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to create tempRG" ;
	
	// attach tempNode to tempRG
    rc = sdbAttachNode( tempRG, HOST, tempNodeSvcName, NULL ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to attach tempNode" ;
    rc = sdbGetNodeByHost( tempRG, HOST, tempNodeSvcName, &tempNode ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to check attach" ;
    
	// start tempRG
    rc = sdbStartReplicaGroup( tempRG ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to start tempRG" ;
	
	// stop tempRG
	rc = sdbStopReplicaGroup( tempRG ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to stop tempRG" ;
	
	// remove tempRG
	rc = sdbRemoveReplicaGroup( db, "temp" ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to remove tempRG" ;

	rc = teardown() ;
	ASSERT_EQ( rc, SDB_OK ) ;
}
