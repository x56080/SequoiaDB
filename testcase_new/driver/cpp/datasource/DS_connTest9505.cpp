/********************************************************************
 * @Description: testcase for datasource
 *               seqDB-9530:addCoord增加的url不符合格式要求
 *               seqDB-9531:addCoord增加的url已经存在
 *               seqDB-9532:removeCoord的url不符合格式要求
 *               seqDB-9533:removeCoord的url不存在
 *               seqDB-9505:申请到池满，再次申请连接
 *               seqDB-9515:disable后，获取连接
 *               seqDB-9516:disable后，addCoord
 *               seqDB-9506:禁用连接池后，空闲队列中的资源被回收
 *               seqDB-9507:禁用连接池后，所有队列中的资源被回收
 *               seqDB-9509:禁用连接池后，再次禁用连接池
 *               seqDB-9517:close后，获取连接
 *               seqDB-9518:close后，addCoord
 *               seqDB-9519:close后，enable连接池
 *               seqDB-9520:close后，disable连接池
 *               seqDB-9510:没有调用init，获取连接
 *               seqDB-9511:没有调用init,addCoord
 *               seqDB-9512:没有调用init，enable连接池
 *               seqDB-9513:没有调用init，disable连接池
 *               seqDB-9514:没有调用init，close连接池
 *               seqDB-9521:获取连接后，没有释放连接，disable连接池
 *               seqDB-9534:releaseConnection不属于连接池的连接
 * @Modify:      Liangxw
 *               2019-09-05
 ********************************************************************/
#include <gtest/gtest.h>
#include <sdbConnectionPool.hpp>
#include "DS_common.hpp"

using namespace sdbclient ;

class connTest9505 : public testBase
{
protected:
   sdbConnectionPoolConf conf ;
   sdbConnectionPool ds ;
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

// 启用连接池获取及释放连接,添加及删除节点( 9530-9533 )
TEST_F( connTest9505, enableConn9530 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
	sdb* conn = NULL ;

   // init enable datasource and get connection
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NOT_ENABLE, rc ) << "fail to test get connection before enable" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection after enable" ;
	ds.releaseConnection( conn ) ;

   // add normal coord
	ds.addCoord( url ) ;
	ASSERT_EQ( 1, ds.getNormalCoordNum() ) << "fail to test add coord " << url ;
	ds.addCoord( "localhost:11910" ) ;
	ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to test add coord localhost:11910" ;
	ds.addCoord( "1.2.3.4:000000" ) ;
	ASSERT_EQ( 3, ds.getNormalCoordNum() ) << "fail to test add coord 1.2.3.4:000000" ;

   // add abnormal coord
	ds.addCoord( "something:000000" ) ;
	ASSERT_EQ( 3, ds.getNormalCoordNum() ) << "fail to test add abnormal coord something:000000" ;

   // add existed coord
   ds.addCoord( "localhost:11910" ) ;
   ASSERT_EQ( 3, ds.getNormalCoordNum() ) << "fail to test add existed coord localhost:11910" ;
   
   // remove normal coord
   ds.removeCoord( "localhost:11910" ) ;
   ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to test remove coord localhost:11910" ;   

   // remove abnormal coord
	ds.removeCoord( "something:000000" ) ;
	ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to test remove abnormal coord something:000000" ;

   // remove not exist coord
   ds.removeCoord( "9.8.7.6:11810" ) ;
   ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to test remove not exist coord 9.8.7.6:11810" ;
}

// 启用连接池获取连接到连接池满后继续申请连接
TEST_F( connTest9505, fullConn9505 )
{
   INT32 rc = SDB_OK ;
   // init and enable datasource
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;

   // get connection until max count
	sdb* conn = NULL ;
	vector<sdb*> vec ;
	while( vec.size() < conf.getMaxCount() )
	{
		ASSERT_EQ( SDB_OK, ds.getConnection( conn ) ) ;
		vec.push_back( conn ) ;
	}
   
   // continue to get connection
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DRIVER_DS_RUNOUT, rc ) << "fail to test get connection after get max count" ;
   
   // release connection
   for( INT32 i = 0;i < vec.size();i++ )
   {
      ds.releaseConnection( vec[i] ) ;
   }
}

// 禁用连接池获取连接( 9515-9516 )
TEST_F( connTest9505, disableConn9515 )
{
   INT32 rc = SDB_OK ;
   // init and disable datasource
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;

   // get connection
	sdb* conn = NULL ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NOT_ENABLE, rc ) << "fail to test get connection when datasource disabled" ;

   // add/remove coord
	ds.addCoord( "localhost:11910" ) ;
   ASSERT_EQ( 2, ds.getNormalCoordNum() ) << "fail to test add coord after disable datasource" ;			
	ds.removeCoord( "localhost:11910" ) ;
   ASSERT_EQ( 1, ds.getNormalCoordNum() ) << "fail to test remove coord after disable datasource" ;
}

// 禁用连接池后资源回收情况,禁用连接池后,连接队列被清空( 9506-9507 )
TEST_F( connTest9505, disableResource9506 )
{
   INT32 rc = SDB_OK ;
   // init enable datasource
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;

   // get connection and check idle/used conn num
	sdb* conn ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
	ASSERT_EQ( 9, ds.getIdleConnNum() ) << "fail to check idle conn num" ;
	ASSERT_EQ( 1, ds.getUsedConnNum() ) << "fail to check used conn num" ;
   ds.releaseConnection( conn ) ; 
   
   // disable and check idle/used conn num
   rc = ds.disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
   ASSERT_EQ( 0, ds.getIdleConnNum() ) << "fail to check idle conn num after disable datasource" ;
	ASSERT_EQ( 0, ds.getUsedConnNum() ) << "fail to check used conn num after disable datasource" ;
}

// 重复禁用连接池后资源回收情况,禁用连接池后,连接队列被清空
TEST_F( connTest9505, disableResourceAgain9509 )
{
   INT32 rc = SDB_OK ;
   // init enable datasource
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;

   // getConnection
	sdb* conn ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
	ASSERT_EQ( 9, ds.getIdleConnNum() ) << "fail to check idle conn num" ;
	ASSERT_EQ( 1, ds.getUsedConnNum() ) << "fail to check used conn num" ;
   ds.releaseConnection( conn ) ;

   // disable datasource
   rc = ds.disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
	ASSERT_EQ( 0, ds.getIdleConnNum() ) << "fail to check idle conn num after disable datasource" ;
	ASSERT_EQ( 0, ds.getUsedConnNum() ) << "fail to check used conn num after disable datasource" ;

   // disable datasource again
   rc = ds.disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource again" ;
   ASSERT_EQ( 0, ds.getIdleConnNum() ) << "fail to check idle conn num again" ;
	ASSERT_EQ( 0, ds.getUsedConnNum() ) << "fail to check used conn num again" ;
}

// 调用close后继续执行相关操作( 9517-9520 )
TEST_F( connTest9505, close9517 )
{
   INT32 rc = SDB_OK ;
   // init enable datasource   
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;

   // close datasource
	ds.close() ;
	
   // operation after close
	sdb* conn ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NOT_ENABLE, rc ) << "fail to test get connection after close" ;	
	ds.addCoord( "localhost:11910" ) ;  
   // ASSERT_EQ( 1, ds.getNormalCoordNum() ) << "fail to test add coord after close" ;  core
   rc = ds.enable() ;
	ASSERT_EQ( SDB_DS_NOT_INIT, rc ) << "fail to test enable after close" ;
   rc = ds.disable() ; 
	ASSERT_EQ( SDB_OK, rc ) << "fail to test disable after close" ;
}


// 没有调用init就执行相关操作( 9510-9514 )
TEST_F( connTest9505, withoutInit9510 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
	sdb* conn = NULL ;
   
   // operation before init
   rc = ds.enable() ;
   ASSERT_EQ( SDB_DS_NOT_INIT, rc ) << "fail to test enable before init" ;
   rc = ds.getConnection( conn ) ;	
	ASSERT_EQ( SDB_DS_NOT_ENABLE, rc ) << "fail to test get connection before init" ;	
	ds.addCoord( url ) ;	
   // ASSERT_EQ( 0, ds.getNormalCoordNum() ) << "fail to test add coord before init" ; core
   rc = ds.disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to test disable before init" ;		
	ds.close() ;		
}

//获取连接后没有释放,直接disable 正常返回
TEST_F( connTest9505, disableWithoutRelease9521 )
{
   INT32 rc = SDB_OK ;
   // init enable datasource
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;

   // get connection
	sdb* conn = NULL ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;

   // disable datasource
   rc = ds.disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;

   // 此处不应该如此使用，请参考SEQUOIADBMAINSTREAM-2854以了解更多信息。
   // ASSERT_EQ( 1, conn->isValid() ) << "fail to check connection valid" ;  connection invalid
   // conn->disconnect() ;  core
}

// 释放不属于连接池的连接
TEST_F( connTest9505, releaseNormalConnection9534 )
{
   INT32 rc = SDB_OK ;
   // init enable datasource
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = ds.enable() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;

   // get a normal connection
   sdb tmp ;
   rc = tmp.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   
   // release normal connection
   ds.releaseConnection( &tmp ) ;
   
   ASSERT_EQ( 1, tmp.isValid() ) << "fail to check connection valid" ;
   tmp.disconnect() ;
}
