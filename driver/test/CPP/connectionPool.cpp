/*******************************************************************************
*@Description : Test connection pool of C++ driver, include _maxIdleCountTest
*@Modify List :
*               2021-10-14   QinCheng Yang
*******************************************************************************/

#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "sdbConnectionPoolComm.hpp"
#include "sdbConnectionPool.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

TEST( connectionPool, _maxIdleCountTest )
{
   // check connection pool is ok when maxIdleCount is 0s
   INT32 rc               = SDB_OK ;
   INT32 maxIdleCount     = 0 ;
   string host            = HOST ;
   string svcName         = SERVER ;
   string address         = host + ":" + svcName ;
   sdbConnectionPoolConf conf ;
   sdbConnectionPool connPool ;
   sdb* db1 ;
   sdb* db2 ;

   conf.setAuthInfo( USER, PASSWD ) ;
   conf.setConnCntInfo( 10, 10, maxIdleCount, 500 ) ;

   rc = connPool.init( address, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connPool.enable() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check idle conn num after enable
   ASSERT_EQ( maxIdleCount, connPool.getIdleConnNum() ) ;

   rc = connPool.getConnection( db1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connPool.getConnection( db2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check idle conn num after getConn
   ASSERT_EQ( maxIdleCount, connPool.getIdleConnNum() ) ;

   connPool.releaseConnection( db1 ) ;
   connPool.releaseConnection( db2 ) ;
   // check idle conn num after release conn
   ASSERT_EQ( maxIdleCount, connPool.getIdleConnNum() ) ;

   connPool.disable() ;
   connPool.close() ;
}
