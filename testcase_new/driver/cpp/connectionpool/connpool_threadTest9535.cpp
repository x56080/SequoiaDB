/*********************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9535:init与init之间并发
 *               seqDB-9536:init与enable之间并发
 *               seqDB-9537:init与disable之间并发
 *               seqDB-9538:init与close之间并发
 *               seqDB-9539:init与getConnection之间并发
 *               seqDB-9540:init与releaseConnection之间并发
 *               seqDB-9541:init与addCoord之间并发
 *               seqDB-9542:init与removeCoord之间并发
 *               seqDB-9543:enable与enable之间并发 
 *               seqDB-9544:enable与disable之间并发 
 *               seqDB-9545:enable与close之间并发
 *               seqDB-9546:enable与getConnection之间并发
 *               seqDB-9547:enable与releaseConnection之间并发 
 *               seqDB-9548:enable与addCoord之间并发
 *               seqDB-9549:enable与removeCoord之间并发
 *               seqDB-9550:disable与disable之间并发
 *               seqDB-9551:disable与close之间并发
 *               seqDB-9552:disable与getConnection之间并发
 *               seqDB-9553:disable与releaseConnection之间并发
 *               seqDB-9554:disable与addCoord之间并发
 *               seqDB-9555:disable与removeCoord之间并发
 *               seqDB-9556:close与close之间并发
 *               seqDB-9557:close与getConnection之间并发
 *               seqDB-9558:close与releaseConnection之间并发
 *               seqDB-9559:close与addCoord之间并发
 *               seqDB-9560:close与removeCoord之间并发
 *               seqDB-9561:getConnection与getConnection之间并发
 *               seqDB-9562:getConnection与releaseConnection之间并发
 *               seqDB-9563:getConnection与addCoord之间并发
 *               seqDB-9564:getConnection与removeCoord之间并发
 *               seqDB-9565:releaseConnection与releaseConnection之间并发
 *               seqDB-9566:releaseConnection与addCoord之间并发
 *               seqDB-9567:releaseConnection与removeCoord之间并发
 *               seqDB-9568:addCoord与addCoord之间并发
 *               seqDB-9569:addCoord与removeCoord之间并发
 *               seqDB-9570:removeCoord与removeCoord之间并发         
 * @Modify:      Liangxw
 *               2019-09-05
 *********************************************************************/
#include <gtest/gtest.h>
#include <sdbConnectionPool.hpp>
#include <iostream>
#include "impWorker.hpp"
#include "connpool_thread.hpp"
#include "connpool_common.hpp"

using namespace sdbclient ;
using namespace import ;
using namespace std ;

// 定义线程数量
#define ThreadNum 5 

class threadTest9535 : public testBase
{
protected:
   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;
   Worker* workers[ ThreadNum ] ;
   string url ;

   void SetUp()
   {
      url = ARGS->coordUrl() ;
   }
   void TearDown()
   {
      ds.close() ;
   }
} ;

/* 问题单1946,init与其他操作并发,core,init close与其他操作不并发
// init与init之间并发  正常获取释放连接
TEST_F( threadTest9535, initInit9535 )
{
   INT32 rc = SDB_OK ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)init, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i]->waitStop() ;
		delete workers[i] ;
	}
   
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable connectionpool" ;
	sdb* conn = NULL ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;		
	ds.releaseConnection( conn ) ;					
}

// init与enable之间并发，正常获取释放连接
TEST_F( threadTest9535, initEnable9536 )
{
   INT32 rc = SDB_OK ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)init_enable, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i]->waitStop() ;
		delete workers[i] ;
	}

	sdb* conn = NULL ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to test get connection" ;	
	ds.releaseConnection( conn ) ;	
}

// init与disable之间并发，不出现死锁
TEST_F( threadTest9535, initDisable9537 )
{
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)init_disable, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
 	{
 		workers[i]->waitStop() ;
 		delete workers[i] ;
 	}			
}

// init与close之间并发，不出现死锁
TEST_F( threadTest9535, initClose9538 )
{
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)init_disable, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {   
      workers[i]->waitStop() ;
      delete workers[i] ;
   } 
}

// init与getConnection/releaseConnection之间并发，没有init时获取连接出错( 9539 9540 )
TEST_F( threadTest9535, initConn9539 )
{
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)init_conn, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// init与addCoord/removeCoord之间并发( 9541 9542 )
TEST_F( threadTest9535, initCoord9541 )
{
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new import::Worker( (WorkerRoutine)init_coord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
*/
#if 0
// enable与enable之间并发，正常获取释放连接
TEST_F( threadTest9535, enableEnable9543 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)enable, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

	sdb* conn = NULL ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   ds.releaseConnection( conn ) ;
}

// enable与disable之间并发，disable时获取连接出错
TEST_F( threadTest9535, enableDisable9544 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)enable_disable, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

	sdb *conn = NULL ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_CLIENT_CONNPOOL_NOT_ENABLE, rc ) << "fail to test get connection" ;
}

/* 问题单1945,enable与close并发,core
// enable与close之间并发，close时获取连接出错
TEST_F( threadTest9535, enableClose9545 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)enable_close, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }

	sdb *conn = NULL ;
   rc = ds.getConnection( conn ) ;
	ASSERT_EQ( SDB_CONNPOOL_NOT_ENABLE, rc ) << "fail to test get connection" ;
}
*/

// enable与getConnection/releaseConnection之间并发，enable之前获取连接出错( 9546 9547 )
TEST_F( threadTest9535, enableConn9546 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)enable_conn, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// enable与addCoord/removeCoord之间并发, 
// init之后能够添加删除节点，添加删除节点正常( 9548 9549 )
TEST_F( threadTest9535, enableCoord9548 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)enable_coord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// disable与disable之间并发，不出错不死锁
TEST_F( threadTest9535, disableDisable9550 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)disable, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

/*
// disable与close之间并发，close后正常disable
TEST_F( threadTest9535, disableClose9551 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)disable_close, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
*/
#endif
// disable与getConnection/releaseConnection之间并发，disable后获取连接出错( 9552 9553 )
TEST_F( threadTest9535, disableConn9552 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)disable_conn, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// disable与addCoord/removeCoord之间并发( 9554 9555 )
TEST_F( threadTest9535, disableCoord9554 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)disable_coord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

/* 
// close与close之间并发，无死锁
TEST_F( threadTest9535, closeClose9556 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)dsclose, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// close与getConnection/releaseConnection之间并发，close后获取连接出错( 9557 9558 )
TEST_F( threadTest9535, closeConn9557 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)dsclose_conn, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// close与addCoord/removeCoord之间并发( 9559 9560 )
TEST_F( threadTest9535, closeCoord9559 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   rc = ds.enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)dsclose_coord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
*/

// getConnection与getConnection/releaseConnection之间并发，正常获取释放连接( 9561 9562 )
TEST_F( threadTest9535, getConnConn9561 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)connection, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// getConnection与addCoord/removeCoord之间并发，正常获取释放连接( 9563 9564 )
TEST_F( threadTest9535, getConnCoord9563 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)connection_coord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// releaseConnection与releaseConnection之间并发，正常获取连接
TEST_F( threadTest9535, releaseConnConn9565 )
{
   INT32 rc = SDB_OK ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

	vector<sdb*> vec ;
	INT32 cnt = 0 ;
	while( cnt < 10 )
	{
		sdb* conn = NULL ;
      rc = ds.getConnection( conn ) ;
		ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
		vec.push_back( conn ) ;
		++cnt ;
	}

	DsArgs args( ds, vec ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)releaseConn, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// releaseConnection与addCoord/removeCoord之间并发，正常获取释放连接( 9566 9567 )
TEST_F( threadTest9535, releaseConnCoord9566 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

	vector<sdb*> vec ;
	INT32 cnt = 0 ;
	while( cnt < 10 )
	{
		sdb* conn = NULL ;
      rc = ds.getConnection( conn ) ;
		ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
		vec.push_back( conn ) ;
		++cnt ;
	}

	DsArgs args( ds, vec ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)releaseConn_coord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// addCoord与addCoord之间并发，正常添加节点
TEST_F( threadTest9535, addCoordAddCoord9568 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)addCoord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// addCoord与removeCoord之间并发，正常添加删除节点
TEST_F( threadTest9535, addCoordRemoveCoord9569 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)addCoord_remove, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}

// removeCoord与removeCoord之间并发，正常删除节点
TEST_F( threadTest9535, removeCoordRemoveCoord9570 )
{
   INT32 rc = SDB_OK ;
	conf.setSyncCoordInterval( 0 ) ;
   rc = ds.init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

	string url1 = url ;

	DsArgs args( ds ) ;
	for( INT32 i = 0;i < ThreadNum;++i )
	{
		workers[i] = new Worker( (WorkerRoutine)removeCoord, &args, false ) ;
		workers[i]->start() ;
	}
	for( INT32 i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
