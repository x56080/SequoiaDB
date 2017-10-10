/**************************************************************
 * @Description: testcase for datasource
 *               seqDB-9528:init时url不符合格式要求
 * @Modify:      Liangxw
 *               2019-09-05
 **************************************************************/
#include <sdbDataSource.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "DS_common.hpp"

using namespace std ;
using namespace sdbclient ;

class invalidArgTest9528 : public testing::Test
{
protected:
   sdbDataSource ds ;
   sdbDataSourceConf conf ;

   void SetUp()
   {
   }
   void TearDown()
   {
      INT32 rc = ds.disable() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
      ds.close() ;
   }
} ;

// 地址格式合法但不存在时，init/enable正常返回,getConnection报错
TEST_F( invalidArgTest9528, url9528 )
{
   INT32 rc = SDB_OK ;
	sdb* conn = NULL ;

	// init and enable datasource with not exist url
   string urlWrong = "something:00000" ;
   rc = ds.init( urlWrong, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   
   // get connection
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NO_REACHABLE_COORD, rc ) << "fail to test get connection with not exist url" ;
}
