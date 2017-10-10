/*************************************************************
 * @Description: testcase for datasource
 *               seqDB-9522:提供不合法的_userName
 *               seqDB-9523:提供不合法的_passwd
 * @Modify:      Liangxw
 *               2019-09-05
 *************************************************************/
#include <sdbDataSource.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "DS_common.hpp"

using namespace std ;
using namespace sdbclient ;

class invalidUsrTest9522 : public testBase
{
protected:
   sdbDataSource ds ;
   sdbDataSourceConf conf ;
   string url ;

   void SetUp()
   {
      url = ARGS->coordUrl() ;
   }
   void TearDown()
   {
      INT32 rc = ds.disable() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
      ds.close() ;
   }
} ;

// 用户信息非法时，init/enable正常返回, getConnection报错( 9522-9523 )
TEST_F( invalidUsrTest9522, userInfo9522 )
{
   INT32 rc = SDB_OK ;
   
   // init enable and get connection
	sdb* conn = NULL ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;

   // check standalone
   if( isStandalone( *conn ) )
   {
      cout << "Run mode is standalone." << endl ;
      ds.releaseConnection( conn ) ;
      return ;
   }

   // create user
   rc = conn->createUsr( "root", "sequoiadb" ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to craete user" ;

   // release connection and close
   ds.releaseConnection( conn ) ;
	ds.close() ;
	
   // test get connection with illegal username
	conf.setUserInfo( "lxw", "sequoiadb" ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NO_REACHABLE_COORD, rc ) << "fail to test get connection with invalid user" ;
	ds.close() ;
	
   // test get connection with no passwd
	conf.setUserInfo( "root", "" ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to test init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NO_REACHABLE_COORD, rc ) << "fail to test get connection with no passwd" ;
	ds.close() ;
		
   // test get connection with illegal passwd
	conf.setUserInfo( "root", "seq" ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NO_REACHABLE_COORD, rc ) << "fail to test get connection with illegal passwd" ;
	ds.close() ;

   // test get connection with legal user and remove user
	conf.setUserInfo( "root", "sequoiadb" ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   rc = conn->removeUsr( "root", "sequoiadb" ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to remove user" ;
	ds.releaseConnection( conn ) ;
}
