/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-17941: sdbCreateUsr2接口支持用户级审计日志
 *                 seqDB-17957: sdbCreateUsr2接口option参数校验
 * @Modify:        liuxiaoxuan Init
 *                 2019-06-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/testcommon.hpp"

sdbConnectionHandle db  = SDB_INVALID_HANDLE ;
sdbConnectionHandle cataDB = SDB_INVALID_HANDLE ;
const char* userName = "user_17941_17957" ;
const char* passwd = "passwd_17941_17957" ;

int setUp()
{
    INT32 rc = SDB_OK ;
  
    // connect sdb  
    getConf() ;
    rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
    CHECK_RC( rc, "fail to connect sdb, rc = %d\n", rc ) ;

    if( isStandalone( db ) )
    {
        printf( "run mode is stand alone.\n" ) ;
        goto done ;
    }

    // get catalog master node connection
    int groupId ;
    sdbReplicaGroupHandle rg ;
    sdbNodeHandle masterNode ;
    groupId = 1 ;
    rg = SDB_INVALID_HANDLE ;
    masterNode = SDB_INVALID_HANDLE ;
    rc = sdbGetReplicaGroup1( db, groupId, &rg ) ;
    CHECK_RC( rc, "fail to get SYSCatalogGroup, rc = %d\n", rc ) ;
    rc = sdbGetNodeMaster( rg, &masterNode ) ;
    CHECK_RC( rc, "fail to get master, rc = %d\n", rc ) ;
   
    const char* hostName ;
    const char* svcName ;
    const char* nodeName ;
    int nodeId ;
    hostName = NULL ;
    svcName = NULL ;
    nodeName = NULL ;
    nodeId = -1 ;
    rc = sdbGetNodeAddr( masterNode, &hostName, &svcName, &nodeName, &nodeId ) ;
    CHECK_RC( rc, "fail to get node address, rc = %d\n", rc ) ;
    rc = sdbConnect( hostName, svcName, USER, PASSWD, &cataDB ) ;
    CHECK_RC( rc, "fail to connect to catalog master node: %s:%s, rc = %d\n", hostName, svcName, rc ) ;
    
    done:
       return rc ;
    error:
       goto done ;
}

int tearDown()
{
    int rc = SDB_OK ;
	
    if( isStandalone( db ) )
    {
        printf( "run mode is stand alone.\n" ) ;
        goto done ;
    }
    
    rc = sdbRemoveUsr( db, userName, passwd ) ;
    CHECK_RC( rc, "fail to remove usr, rc = %d\n", rc ) ;
	   sdbDisconnect( db ) ;
    sdbDisconnect( cataDB ) ;
	   sdbReleaseConnection( db ) ;
    sdbReleaseConnection( cataDB ) ;

    done:
       return rc ;
    error:
       goto done ;
}

TEST( createUsr17941_17957, withOption17941 )
{
    int rc = SDB_OK ;
    // setup
    rc = setUp() ;
    if( isStandalone( db ) )  return ;
    ASSERT_EQ( rc, SDB_OK ) ;

    const char* mask = "CLUSTER|DML|DDL|DCL|!INSERT|UPDATE" ;
    bson obj ;
    bson_init( &obj ) ;
    bson_append_string( &obj, "AuditMask", mask ) ;
    bson_finish( &obj ) ;
    rc = sdbCreateUsr2( db, userName, passwd, &obj ) ;
    ASSERT_EQ( SDB_OK, rc ) << "fail to create user" ; 
    bson_destroy( &obj ) ;
   
    sdbCollectionHandle cl  = SDB_INVALID_HANDLE ;
    rc = sdbGetCollection( cataDB, "SYSAUTH.SYSUSRS", &cl ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to get collection SYSAUTH.SYSUSRS" ;
   
    CHAR destMask[64] ;
    bson subobj ;
    bson_init( &obj ) ;
    bson_init( &subobj ) ;
    sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
    rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to query" ;
    rc = sdbNext( cursor, &obj ) ;
    ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
    bson_iterator it, sub ;
    bson_find( &it, &obj, "Options" ) ;
    bson_iterator_subobject( &it, &subobj ) ;
    bson_find( &sub, &subobj, "AuditMask" ) ;
    bson_print( &subobj ) ;
    string srcMask = bson_iterator_string( &sub ) ;
    strcpy( destMask, srcMask.c_str() ) ;
    ASSERT_EQ( 0, strcmp( destMask, mask ) ) << "check auditmask wrong, expect:" << mask << " actual:" << destMask ;
    bson_destroy( &obj ) ;
    bson_destroy( &subobj ) ;
    sdbReleaseCursor( cursor ) ;
    sdbReleaseCollection( cl ) ;
    
    // teardown
    rc = tearDown() ;
    ASSERT_EQ( rc, SDB_OK ) ;
}

TEST( createUsr17941_17957, paramCheck17957 )
{
    int rc = SDB_OK ;
    // setup
    rc = setUp() ;
    if( isStandalone( db ) )  return ;
    ASSERT_EQ( rc, SDB_OK ) ;

    // invalid parameter
    bson obj ;
    bson_init( &obj ) ;
    bson_append_string( &obj, "AuditMask", "test" ) ;
    bson_finish( &obj ) ;
    rc = sdbCreateUsr2( db, userName, passwd, &obj ) ;
    ASSERT_EQ( SDB_INVALIDARG, rc ) ; 
    bson_destroy( &obj ) ;
    
    // empty string
    const char* mask = " " ;
    bson_init( &obj ) ;
    bson_append_string( &obj, "AuditMask", mask ) ;
    bson_finish( &obj ) ;
    rc = sdbCreateUsr2( db, userName, passwd, &obj ) ;
    ASSERT_EQ( SDB_OK, rc ) << "fail to create user" ; 
    bson_destroy( &obj ) ;
   
    sdbCollectionHandle cl  = SDB_INVALID_HANDLE ;
    rc = sdbGetCollection( cataDB, "SYSAUTH.SYSUSRS", &cl ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to get collection SYSAUTH.SYSUSRS" ;
   
    CHAR destMask[64];
    bson subobj ;
    bson_init( &obj ) ;
    bson_init( &subobj ) ;
    sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
    rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to query" ;
    rc = sdbNext( cursor, &obj ) ;
    ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
    bson_iterator it, sub ;
    bson_find( &it, &obj, "Options" ) ;
    bson_iterator_subobject( &it, &subobj ) ;
    bson_find( &sub, &subobj, "AuditMask" ) ;
    string srcMask = bson_iterator_string( &sub ) ;
    strcpy( destMask, srcMask.c_str() ) ;
    ASSERT_EQ( 0, strcmp( destMask, mask ) ) << "check auditmask wrong, expect:" << mask << " actual:" << destMask ;
    bson_destroy( &obj ) ;
    bson_destroy( &subobj ) ;
    sdbReleaseCursor( cursor ) ;
    sdbReleaseCollection( cl ) ;
    
    // teardown
    rc = tearDown() ;
    ASSERT_EQ( rc, SDB_OK ) ;
}
