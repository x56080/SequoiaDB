/**************************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-2148
* @Modify     : Liang xuewang Init
*			 	2017-01-06
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/testcommon.hpp"

int getCurrentSessionId( sdbConnectionHandle db, SINT64* sessionId )
{
    int rc = SDB_OK ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
    bson condition ;
    bson_init( &condition ) ;
	bson selector ;
    bson_init( &selector ) ;
	bson obj ;
    bson_init( &obj ) ;
	bson_iterator it ;

    // list current session
    bson_append_bool( &condition, "Global", false ) ;
    bson_finish( &condition ) ;
    bson_append_string( &selector, "SessionID", "" ) ;
    bson_finish( &selector ) ;
    rc = sdbGetList( db, SDB_LIST_SESSIONS_CURRENT, &condition, &selector, NULL, &cursor ) ;
    CHECK_RC( rc, "fail to list current session, rc = %d\n", rc ) ;

	// get session id
    rc = sdbNext( cursor, &obj ) ;
    CHECK_RC( rc, "fail to get next in cursor, rc = %d\n", rc ) ;
    bson_iterator_init( &it, &obj ) ;
    *sessionId = bson_iterator_int( &it ) ;

done:
	bson_destroy( &condition ) ;                            
    bson_destroy( &selector ) ;
	bson_destroy( &obj ) ;	
	sdbReleaseCursor( cursor ) ;
	return rc ;
error:
	goto done ;
}

int getNodeSessionIds( sdbConnectionHandle db, const char* nodeName, SINT64 sessionId[], 
						 char sessionType[1024][20], int size )
{
	int rc = SDB_OK ;
	bson condition ;
    bson_init( &condition ) ;
	bson selector ;
    bson_init( &selector ) ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson obj ;
    bson_init( &obj ) ;
	int i = 0 ;

	// get node sessions
    bson_append_string( &condition, "NodeName", nodeName ) ;
    bson_append_bool( &condition, "Global", false ) ;
    bson_finish( &condition ) ;
    bson_append_string( &selector, "SessionID", "" ) ;
	bson_append_string( &selector, "Type", "" ) ;
    bson_finish( &selector ) ;
    rc = sdbGetList( db, SDB_LIST_SESSIONS, &condition, &selector, NULL, &cursor ) ;
    CHECK_RC( rc, "fail to list sessions on node, rc = %d\n", rc ) ;

    // get session ids
    while( !( rc = sdbNext( cursor, &obj ) ) && i < size )
    {
        bson_iterator it ;
		bson_find( &it, &obj, "SessionID" ) ;
        sessionId[i] = bson_iterator_int( &it ) ;
		bson_find( &it, &obj, "Type" ) ;
		strcpy( sessionType[i], bson_iterator_string( &it ) ) ;
		i++ ;
        bson_destroy( &obj ) ;
        bson_init( &obj ) ;
    }
	rc = SDB_OK ;

done:
	bson_destroy( &condition ) ;
    bson_destroy( &selector ) ;
    bson_destroy( &obj ) ;
    sdbReleaseCursor( cursor ) ;
	return rc ;
error:
	goto done ;
}

TEST( forceSession, currentSession )
{
	int rc = SDB_OK ;
    sdbConnectionHandle db = SDB_INVALID_HANDLE ;
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
    if( isStandalone( db ) )
		return ;
	SINT64 sessionId = -1 ;
	int groupId[] = { 1, 2, 1000 } ;   // catalogRG/coordRG/dataRG

	const char* hostName = NULL ;
    const char* svcName = NULL ;
    const char* nodeName = NULL ;
    int nodeId = -1 ;

	for( int i = 0;i < sizeof(groupId)/sizeof(groupId[0]);i++ )
	{
    	// get node addr and connect
    	sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
    	sdbNodeHandle node = SDB_INVALID_HANDLE ;
    	rc = sdbGetReplicaGroup1( db, groupId[i], &rg ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to get group " << groupId[i] ;
    	rc = sdbGetNodeSlave( rg, &node ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to get slave node of group " << groupId[i] ;
    	rc = sdbGetNodeAddr( node, &hostName, &svcName, &nodeName, &nodeId ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to get node addr " << nodeName  ;
		printf( "node: hostName=%s svcName=%s nodeName=%s nodeId=%d\n", hostName, svcName, nodeName, nodeId ) ;
    	sdbDisconnect( db ) ;
		sdbReleaseConnection( db ) ;
    	rc = sdbConnect( hostName, svcName, USER, PASSWD, &db ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to connect node" ;

     	// force node current session
    	rc = getCurrentSessionId( db, &sessionId ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to get current session id before force";
    	rc = sdbForceSession( db, sessionId, NULL ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to test force curent session" ;
    	
		// reconect and check session id
		SINT64 sessionId1 = -1 ;
		rc = getCurrentSessionId( db, &sessionId1 ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to get current session id after force" ;
		ASSERT_EQ( sessionId, sessionId1 ) << "fail to check session id unchanged when force current session" ;	
	
        // reconect to coord
    	sdbDisconnect( db ) ;
    	sdbReleaseConnection( db ) ;
		rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
    	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
	}
	
	sdbDisconnect( db ) ;
    sdbReleaseConnection( db ) ;
}

TEST( forceSession, withOption )
{
	int rc = SDB_OK ;
    sdbConnectionHandle db = SDB_INVALID_HANDLE ;
    rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
	if( isStandalone( db ) )
        return ;
	SINT64 oldSessionId = -1 ;
    SINT64 newSessionId = -1 ;

	const char* groupName = "SYSCatalogGroup" ;
	const int groupId = 1 ;
	const char* hostName = NULL ;
	const char* svcName = NULL ;
	const char* nodeName = NULL ;
	int nodeId = -1 ;
	
	// get node addr and connect
    sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
    sdbNodeHandle node = SDB_INVALID_HANDLE ;
    rc = sdbGetReplicaGroup1( db, groupId, &rg ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to get SYSCatalog group" ;
    rc = sdbGetNodeSlave( rg, &node ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to get slave node of group " ;
    rc = sdbGetNodeAddr( node, &hostName, &svcName, &nodeName, &nodeId ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to get node addr " << nodeName  ;
    sdbDisconnect( db ) ;
    sdbReleaseConnection( db ) ;
    rc = sdbConnect( hostName, svcName, USER, PASSWD, &db ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to connect node " << nodeName ;
	printf( "node: hostName=%s svcName=%s nodeName=%s nodeId=%d\n", hostName, svcName, nodeName, nodeId ) ;

	// get session ids on node	
	SINT64 sessionIds[1024] ;
	char sessionTypes[1024][20] ;
	memset( sessionIds, 0, sizeof(sessionIds) ) ;
	rc = getNodeSessionIds( db, nodeName, sessionIds, sessionTypes, 1024 ) ;
	ASSERT_EQ( rc, SDB_OK ) ;

	// get agent session id to force
	SINT64 sessionId = -1 ;
	for( int i = 0;i < 1024 && sessionIds[i] != 0;i++ )
	{
		if( strcmp( sessionTypes[i], "Agent" ) == 0 )
			sessionId = sessionIds[i] ;
	}
	if( sessionId == -1 ) return ;
	
	// make option
	bson option ;
	bson_init( &option ) ;
	bson_append_string( &option, "HostName", hostName ) ;
	bson_append_string( &option, "svcname", svcName ) ;
	bson_append_int( &option, "NodeID", nodeId ) ;
	bson_append_int( &option, "GroupID", groupId ) ;
	bson_append_string( &option, "GroupName", groupName ) ;
	bson_finish( &option ) ;
	bson_print( &option ) ;	

	// force session with db connected to coord
	sdbDisconnect( db ) ;
	sdbReleaseConnection( db ) ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	rc = sdbForceSession( db, sessionId, &option ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to force session " << sessionId ;
	
	sdbDisconnect( db ) ;
    sdbReleaseConnection( db ) ;		 
}	
