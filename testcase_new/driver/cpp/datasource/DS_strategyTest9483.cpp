/*****************************************************************************************
 * @Description: testcase for datasource
 *               seqDB-9483:同步coord的情况下，顺序策略申请连接
 *               seqDB-9484:同步coord的情况下，随机策略申请连接
 *               seqDB-9485:同步coord的情况下，本地策略申请连接
 *               seqDB-9486:同步coord的情况下，均衡策略申请连接
 *               seqDB-9487:初始化时指定多个coord，按顺序策略申请连接
 *               seqDB-9488:初始化时指定多个coord，按随机策略申请连接
 *               seqDB-9489:初始化时指定多个coord，按本地策略申请连接
 *               seqDB-9490:初始化时指定多个coord，按均衡策略申请连接
 *               seqDB-9491:初始化时指定一个coord，过程中增加coord，按顺序策略申请连接
 *               seqDB-9492:初始化时指定一个coord，过程中增加coord,按随机策略申请连接
 *               seqDB-9493:初始化时指定一个coord，过程中增加coord，按本地策略申请连接
 *               seqDB-9494:初始化时指定一个coord，过程中增加coord，按均衡策略申请连接
 *               seqDB-9495:初始化时指定多个coord，过程中删除coord，按顺序策略申请连接
 *               seqDB-9496:初始化时指定多个coord，过程中删除coord,按随机策略申请连接
 *               seqDB-9497:初始化时指定多个coord，过程中删除coord，按本地策略申请连接
 *               seqDB-9498:初始化时指定多个coord，过程中删除coord，按均衡策略申请连接
 *               seqDB-9508:禁用连接池后，启用连接池
 *               手工测试用例，不加入scons脚本
 * @Modify:      Liangxw
 *               2019-09-05
 *****************************************************************************************/
#include <gtest/gtest.h>
#include <sdbDataSource.hpp>
#include <iostream>
#include "DS_common.hpp"

using namespace std ;
using namespace sdbclient ;

// 手工测试用例,通过控制台输出检查分配策略是否生效
// 需要开发修改代码显示新连接的连接地址测试
class strategyTest9483 : public testBase
{
protected:
   sdbDataSource ds ;
   sdbDataSourceConf conf ;
   string url ;
   string url1 ;
   string url2 ;
   INT32 coordNum ;  // 协调组中协调节点数

   void SetUp()
   {
      url = "192.168.31.61:11810" ;   // 协调节点：11810 11910 11920
      url1 = "192.168.31.61:11910" ;
      url2 = "192.168.31.61:11920" ;
      coordNum = 3 ;
   }
   void TearDown()
   {
      INT32 rc = ds.disable() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
      ds.close() ;
   }
} ;

INT32 checkStartegy( sdbDataSource& ds )
{
   INT32 rc = SDB_OK ;
   sdb* conn = NULL ;
   INT32 cnt = 0 ;
   vector<sdb*> vec ;
   while( cnt < 10 )
   {
      rc = ds.getConnection( conn ) ;
      CHECK_RC( SDB_OK, rc, "fail to get connection" ) ;
      vec.push_back( conn ) ;
      ++cnt ;
   }
done:
   for( INT32 i = 0;i < vec.size();i++ )
   {
      ds.releaseConnection( vec[i] ) ; 
   }
   return rc ;
error:
   goto done ;
}

// 同步情况下测试顺序分配策略
TEST_F( strategyTest9483, syncSerial9483 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_SERIAL ) ;
	conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ossSleep( 2*1000 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
   
   checkStartegy( ds ) ;
}

// 同步情况下测试随机分配策略
TEST_F( strategyTest9483, syncRandom9484 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_RANDOM ) ;
	conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ossSleep( 2*1000 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
   checkStartegy( ds ) ;
}

// 同步情况下测试本地分配策略
TEST_F( strategyTest9483, syncLocal9485 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_LOCAL ) ;
	conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ossSleep( 2*1000 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;

	checkStartegy( ds ) ;
}

// 同步情况下测试均衡分配策略
TEST_F( strategyTest9483, syncBalance9486 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_BALANCE ) ;
	conf.setSyncCoordInterval( 1 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ossSleep( 2*1000 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
   
	checkStartegy( ds ) ;
}

// 初始化多个节点情况下测试顺序分配策略
TEST_F( strategyTest9483, multiCoordSerial9487 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_SERIAL ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;
   coordNum = urlList.size() ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;

	checkStartegy( ds ) ;
}

// 初始化多个节点情况下测试随机分配策略
TEST_F( strategyTest9483, multiCoordRandom9488 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_RANDOM ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;
   coordNum = urlList.size() ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
   checkStartegy( ds ) ;
}

// 初始化多个节点情况下测试本地分配策略
TEST_F( strategyTest9483, multiCoordLocal9489 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_LOCAL ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;
	coordNum = urlList.size() ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;

	checkStartegy( ds ) ;
}

// 初始化多个节点情况下测试均衡分配策略
TEST_F( strategyTest9483, multiCoordBalance9490 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_BALANCE ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
   checkStartegy( ds ) ;
}

// 初始化一个节点，过程中添加节点情况下测试顺序分配策略
TEST_F( strategyTest9483, addCoordSerial9491 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_SERIAL ) ;
	conf.setSyncCoordInterval( 0 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.addCoord( url1 ) ;
	ds.addCoord( url2 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;

   checkStartegy( ds ) ;
}

// 初始化一个节点，过程中添加节点情况下测试随机分配策略
TEST_F( strategyTest9483, addCoordRandom9492 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_RANDOM ) ;
	conf.setSyncCoordInterval( 0 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.addCoord( url1 ) ;
	ds.addCoord( url2 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
	checkStartegy( ds ) ;
}

// 初始化一个节点，过程中添加节点情况下测试本地分配策略
TEST_F( strategyTest9483, addCoordLocal9493 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_LOCAL ) ;
	conf.setSyncCoordInterval( 0 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.addCoord( url1 ) ;
	ds.addCoord( url2 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
	checkStartegy( ds ) ;
}

// 初始化一个节点，过程中添加节点情况下测试均衡分配策略
TEST_F( strategyTest9483, addCoordBalance9494 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_BALANCE ) ;
	conf.setSyncCoordInterval( 0 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.addCoord( url1 ) ;
	ds.addCoord( url2 ) ;
	ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
	checkStartegy( ds ) ;
}

// 初始化多个节点过程中删除节点情况下测试顺序分配策略
TEST_F( strategyTest9483, removeCoordSerial9495 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_SERIAL ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;
	urlList.push_back( url2 ) ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ; 
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.removeCoord( url2 ) ;
	ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
	checkStartegy( ds ) ;
}

// 初始化多个节点过程中删除节点情况下测试随机分配策略
TEST_F( strategyTest9483, removeCoordRandom9496 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_RANDOM ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;
	urlList.push_back( url2 ) ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.removeCoord( url2 ) ;
	ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
	checkStartegy( ds ) ;
}

// 初始化多个节点过程中删除节点情况下测试本地分配策略
TEST_F( strategyTest9483, removeCoordLocal9497 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_LOCAL ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;
	urlList.push_back( url2 ) ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.removeCoord( url2 ) ;
	ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
	checkStartegy( ds ) ;
}

// 初始化多个节点过程中删除节点情况下测试均衡分配策略
TEST_F( strategyTest9483, removeCoordBalance9498 )
{
   INT32 rc = SDB_OK ;
	conf.setConnectStrategy( DS_STY_BALANCE ) ;
	conf.setSyncCoordInterval( 0 ) ;
	vector<string> urlList ;
	urlList.push_back( url ) ;
	urlList.push_back( url1 ) ;
	urlList.push_back( url2 ) ;

   rc = ds.init( urlList, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	ds.removeCoord( url2 ) ;
	ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to check coord num" ;
	
	checkStartegy( ds ) ;
}

// 禁用连接池再启用情况下测试均衡分配策略( 默认分配策略 )
TEST_F( strategyTest9483, disThenEnable9508 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;

   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   ds.addCoord( url1 ) ;
   ds.addCoord( url2 ) ;
   ASSERT_EQ( coordNum, ds.getNormalCoordNum() ) << "fail to check coord num" ;
   rc = ds.enable() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = ds.disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
	rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource again" ;
	
	checkStartegy( ds ) ;
}
