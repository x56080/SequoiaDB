/******************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1958
 *               seqDB-9649:SSL功能未开启，C++客户端使用SSL
 *				     test ssl when usessl is false in configure file
 * @Modify:      Liang xuewang Init
 *               2017-07-20
 ******************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

// 测试关闭ssl，sdb( useSSL=false )
TEST( sslFalseTest9649, sdbFalse9649 )
{
	INT32 rc = SDB_OK ;

	sdb db( FALSE ) ;
	rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb when ssl closed" ;

   const CHAR* csName = "sslTestCs9649" ;
   const CHAR* clName = "sslTestCl9649" ;
	sdbCollectionSpace cs ;
   sdbCollection cl ;
	rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

	rc = db.dropCollectionSpace( csName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
	
	db.disconnect() ;
}

// 测试关闭ssl，sdb( useSSL=true )
TEST( sslFalse, sdbTrue )
{
	INT32 rc = SDB_OK ;

	sdb db( TRUE ) ;
	rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
	ASSERT_EQ( SDB_NETWORK, rc ) << "fail to test connect secure sdb when ssl closed" ;
}
