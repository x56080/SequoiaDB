/***********************************************************************
 * @Description: testcase for connectionPool
 *               seqDB-28048: Init connPool with error passwd and zero initCnt
 * @Modify:      Chenwenjia
 *               2022/09/29
 ***********************************************************************/
#include <sdbConnectionPool.hpp>
#include <iostream>
#include <string>
#include <gtest/gtest.h>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

class connPoolAuthTest28048 : public testBase
{
protected:
   sdbConnectionPool pool ;
   string url ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      testBase::SetUp() ;

      url = ARGS->coordUrl() ;
      rc = db.createUsr( "sdbadmin","sequoiadb" ) ;
      ASSERT_EQ( SDB_OK, rc ) << "create user fail" ;
   }

   void TearDown()
   {
      db.removeUsr( "sdbadmin", "sequoiadb" ) ;
      pool.close() ;
   }
} ;

TEST_F( connPoolAuthTest28048, authFail28048 )
{
   INT32 rc = SDB_OK ;
   sdbConnectionPoolConf conf ;
   sdb* conn = NULL ;
   conf.setAuthInfo( "sdbadmin", "errorPasswd" ) ;
   //when initCnt=0, initConnPool should be succ and get conn should be fail
   conf.setConnCntInfo( 0, 5, 5, 10 ) ;

   rc = pool.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   rc = pool.getConnection( conn ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "should be fail to get connection" ;
}