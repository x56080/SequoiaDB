/**************************************************************
* @Description: test case for Jira questionaire
*				( manual test case,not in CI )
*				SEQUOIADBMAINSTREAM-1958
*				test ssl with ssl is allowed in configure file
* @Modify:      Liang xuewang Init
*               2016-11-10
**************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/testcommon.hpp"

const char* CsModName = "sslTestCs" ;
char CsName[100] ;
char ClName[] = "sslTestCl" ;
sdbConnectionHandle connection = 0 ;
sdbCSHandle cs = 0 ;
sdbCollectionHandle cl = 0 ;

// 测试开启ssl，使用sdbSecureConnect正常连接创建集合空间、集合
TEST( ssl, sdbSecureConnect )
{
	int rc = SDB_OK ;

	// 使用ssl连接sdb并创建集合空间、集合
	getConf() ;
	rc = sdbSecureConnect( HOSTNAME, SVCNAME, USER, PASSWD, &connection ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to secure connect when ssl is open" ;

	getUniqueName( CsModName, CsName ) ;
	rc = sdbCreateCollectionSpace( connection, CsName, SDB_PAGESIZE_4K, &cs ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << CsName ;

	rc = sdbCreateCollection( cs, ClName, &cl ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << ClName ;

	// 删除集合、集合空间、断开连接
	rc = sdbDropCollection( cs, ClName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << ClName ;
	
	rc = sdbDropCollectionSpace( connection, CsName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << CsName ;

	sdbDisconnect( connection ) ;
	// 释放句柄
	sdbReleaseCollection( cl ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseConnection( connection ) ;
}

// 测试开启ssl，使用sdbSecureConnect1正常连接创建集合空间、集合
TEST( ssl, sdbSecureConnect1 )
{
	int rc = SDB_OK ;

	// 使用ssl连接sdb并创建集合空间、集合
	getConf() ;
	char ConnAddr[200] ;
	sprintf( ConnAddr, "%s:%s", HOSTNAME, SVCNAME ) ;
	const char* ConnAddrs[1] ;
	ConnAddrs[0] = ConnAddr ;
	int arrSize = sizeof( ConnAddrs ) / sizeof( ConnAddrs[0] ) ;
	rc = sdbSecureConnect1( ConnAddrs, arrSize, USER, PASSWD, &connection ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to secure connect when ssl is open again" ;
	
	getUniqueName( CsModName, CsName ) ;
	rc = sdbCreateCollectionSpace( connection, CsName, SDB_PAGESIZE_4K, &cs ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << CsName ;

	rc = sdbCreateCollection( cs, ClName, &cl ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << ClName ;

	// 删除集合、集合空间、断开连接
	rc = sdbDropCollection( cs, ClName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << ClName ;

	rc = sdbDropCollectionSpace( connection, CsName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << CsName ;
	
	sdbDisconnect( connection ) ;
	// 释放句柄
	sdbReleaseCollection( cl ) ;
    sdbReleaseCS( cs ) ;
 	sdbReleaseConnection( connection ) ;
} 
