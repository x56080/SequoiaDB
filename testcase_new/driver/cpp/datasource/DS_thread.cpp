/*****************************************************
 * c++驱动连接池并发测试的线程函数
 *
 *****************************************************/
#include "DS_thread.hpp"
#include "DS_common.hpp"
#include <ctime>

using namespace std ;

INT32 getRand()
{
	static BOOLEAN inited = TRUE ;
	if( inited )
	{
		srand( time( NULL ) ) ;
		inited = FALSE ;
	}
	return rand()%10 ;
}

void init( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdbDataSourceConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
}

void init_enable( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdbDataSourceConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
}

void init_disable( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdbDataSourceConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = arg->getDs().disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
}

void init_close( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdbDataSourceConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
	arg->getDs().close() ;
}

void init_conn( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdbDataSourceConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
	arg->getDs().releaseConnection( conn ) ;
}

void init_coord( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdbDataSourceConf conf ;
   string url = ARGS->coordUrl() ;
   rc = arg->getDs().init( url, conf ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to init datasource" ;
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().removeCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
}

void enable( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
}

void enable_disable( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
   rc = arg->getDs().disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
}

void enable_close( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	arg->getDs().close() ;
}

void enable_conn( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
	arg->getDs().releaseConnection( conn ) ;
}

void enable_coord( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
   ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().removeCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
}

void disable( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().enable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to enable datasource" ;
}

void disable_close( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
	arg->getDs().close() ;
}

void disable_conn( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
	sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
	ASSERT_EQ( SDB_DS_NOT_ENABLE, rc ) << "fail to test get connection" ;
	arg->getDs().releaseConnection( conn ) ;
}

void disable_coord( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
   rc = arg->getDs().disable() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to disable datasource" ;
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().removeCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
}

void dsclose( DsArgs* arg )
{
	ossSleep( getRand() * 100 ) ;
	arg->getDs().close() ;
}

void dsclose_conn( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
	arg->getDs().releaseConnection( conn ) ;
	arg->getDs().close() ;
}

void dsclose_coord( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().removeCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().close() ;
}

void connection( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
	arg->getDs().releaseConnection( conn ) ;
}

void connection_coord( DsArgs* arg )
{
   INT32 rc = SDB_OK ;
	ossSleep( getRand() * 100 ) ;
	sdb* conn = NULL ;
   rc = arg->getDs().getConnection( conn ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
	arg->getDs().releaseConnection( conn ) ;
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().removeCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
}

void releaseConn( DsArgs* arg )
{
	ossSleep( getRand() * 100 ) ;
	vector<sdb*> vec = arg->getConnVec() ;
	for( INT32 i = 0;i != vec.size();++i )
   {
		arg->getDs().releaseConnection( vec[i] ) ;
   }
}

void releaseConn_coord( DsArgs* arg )
{
	ossSleep( getRand() * 100 ) ;
	vector<sdb*> vec = arg->getConnVec() ;
	for( INT32 i = 0;i != vec.size();++i )
   {
		arg->getDs().releaseConnection( vec[i] ) ;
   }
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().removeCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
}

void addCoord( DsArgs* arg )
{
	ossSleep( getRand() * 100 ) ;
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_EQ( 2, arg->getDs().getNormalCoordNum() ) ;
}

void addCoord_remove( DsArgs* arg )
{
	string url1 = "localhost:11910" ;
	arg->getDs().addCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
	arg->getDs().removeCoord( url1 ) ;
	ASSERT_GE( 2, arg->getDs().getNormalCoordNum() ) ;
}

void removeCoord( DsArgs* arg )
{
	ossSleep( getRand() * 100 ) ;
	string url = ARGS->coordUrl() ;
	arg->getDs().removeCoord( url ) ;
	ASSERT_EQ( 0, arg->getDs().getNormalCoordNum() ) ;
}
