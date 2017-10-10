/*************************************************************
 * @Desciption: testcase for datasource
 *              seqDB-9578:valgrind校验是否有内存泄露
 *              手工测试用例，不加入scons脚本
 * @Modify:     Liangxw
 *              2019-09-05
 *************************************************************/
#include <client.hpp>
#include <sdbDataSource.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "DS_common.hpp"

using namespace sdbclient ;
using namespace std ;

class memTest9578 : public testBase
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

// 手工测试用例，用valgrind测试内存泄露
TEST_F( memTest9578, memCheck9578 )
{
   INT32 rc = SDB_OK ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;

	sdb* conn ;
	CHAR ch ;
	for( INT32 i = 0;;i++ )
	{
      rc = ds.getConnection( conn ) ;
		ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
		ds.releaseConnection( conn ) ;
		ds.addCoord( "192.168.31.61:11910" ) ;
		ds.removeCoord( "192.168.31.61:11910" ) ;
		if( i % 10000000 == 0 )
		{
			cout << "continue??[y/n]" << endl ;
			cin >> ch ;
			if( ch == 'n' )
				break ;
		}
	}
}
