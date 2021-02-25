/**********************************************************
 * C++驱动连接池并发测试线程函数头文件
 *
 **********************************************************/
#ifndef DS_THREAD_HPP
#define DS_THRAED_HPP

#include "impWorker.hpp"
#include "arguments.hpp"
#include <sdbConnectionPool.hpp>
#include <gtest/gtest.h>
#include <vector>

using namespace sdbclient ;
using namespace std ;
using namespace import ;

/*
DsArgs：包含sdbDataSource类的类，为了方便定义创建线程的函数, 需要继承WorkerArgs类
线程函数格式 void func( typename* args ) {} 返回void, 参数只有一个且为指针
线程类 Worker( WorkerRoutine routine, WorkerArgs* args, BOOLEAN managed = FALSE );
线程类构造 Worker worker( (WorkerRoutine)func, &args, false ) ; 
false表明线程结束后不删除参数指针，构造后调用start/waitstop方法
*/

class DsArgs : public WorkerArgs
{
private:
	sdbConnectionPool& _ds ;			// sdbDataSource成员，使用引用防止DsArgs构造函数调用sdbDataSource的私有构造函数
	vector<sdb*> conn_vec ;	// connection数组，存放getConnection后获得的连接，方便后续调用releaseConnection时传入
public:
	DsArgs( sdbConnectionPool& ds ):_ds( ds ) {}
	DsArgs( sdbConnectionPool& ds, vector<sdb*>& vec )
			:_ds( ds ), conn_vec( vec ) {}
	~DsArgs() {}

	sdbConnectionPool& getDs() { return _ds ; }
	vector<sdb*>& getConnVec() { return conn_vec ; }
} ;

INT32 getRand() ;
void init( DsArgs* arg ) ;
void init_enable( DsArgs* arg ) ;
void init_disable( DsArgs* arg ) ;
void init_close( DsArgs* arg ) ;
void init_conn( DsArgs* arg ) ;
void init_coord( DsArgs* arg ) ;
void enable( DsArgs* arg ) ;
void enable_disable( DsArgs* arg ) ;
void enable_close( DsArgs* arg ) ;
void enable_conn( DsArgs* arg ) ;
void enable_coord( DsArgs* arg ) ;
void disable( DsArgs* arg ) ;
void disable_close( DsArgs* arg ) ;
void disable_conn( DsArgs* arg ) ;
void disable_coord( DsArgs* arg ) ;
void dsclose( DsArgs* arg ) ;
void dsclose_conn( DsArgs* arg ) ;
void dsclose_coord( DsArgs* arg ) ;
void connection( DsArgs* arg ) ;
void connection_coord( DsArgs* arg ) ;
void releaseConn( DsArgs* arg ) ;
void releaseConn_coord( DsArgs* arg ) ;
void addCoord( DsArgs* arg ) ;
void addCoord_remove( DsArgs* arg ) ;
void removeCoord( DsArgs* arg ) ;

#endif
