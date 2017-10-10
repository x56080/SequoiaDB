/********************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1958
 *				     test ssl when usessl is true in configure file
 *				     sequoiadb need to be enterprise
 *               seqDB-9648:SSL功能开启，C++客户端使用SSL
 * @Modify:      Liang xuewang Init
 *               2017-09-22
 ********************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

// 测试开启ssl，sdb( useSSL=true )
TEST( sslTrueTest9648, sdbTrue9648 )
{
	INT32 rc = SDB_OK ;

	sdb db( TRUE ) ;
	rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to connect secure sdb when ssl is open" ;

   const CHAR* csName = "sslTestCs9648" ;
   const CHAR* clName = "sslTestCl9648" ;
	sdbCollectionSpace cs ;
   sdbCollection cl ;
	rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

	rc = db.dropCollectionSpace( csName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
	
	db.disconnect() ;
}

// 测试开启ssl，sdb( useSSL=false )
TEST( sslTrue, sdbFalse )          
{                          
   INT32 rc = SDB_OK ;

   sdb db( FALSE ) ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb when ssl is open" ;

   const CHAR* csName = "sslTestCs9648" ;
   const CHAR* clName = "sslTestCl9648" ; 
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

   db.disconnect() ;
}
