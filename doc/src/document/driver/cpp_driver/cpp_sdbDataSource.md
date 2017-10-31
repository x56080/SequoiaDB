C++ 驱动的连接池提供给用户一个快速获取连接实例的途径。

##连接池用法##

使用类 sdbDatasource 的 getConnection 方法从连接池中获取一个连接，使用 releaseConnection 方法把取出的连接放回连接池。当连接池使用的连接数到达连接上限时，下一个请求连接的操作将会等待一段时间，若在规定的时间内无空闲的连接可用，请求将失败。类 sdbDataSourceConf 可以设置连接池的各种参数。

详情请查看相关 [C++ API](api/cpp/html/index.html) 介绍。

##例子##

```lang-javascript
#include "common.hpp" // by default, this file is in /opt/sequoiadb/samples/CPP
#include "sdbDataSourceComm.hpp"
#include "sdbDataSource.hpp"
#include <vector>

using namespace std ;
using namespace sdbclient ;
using namespace bson ;

void queryTask( sdbDataSource &ds )
{
   INT32                rc = SDB_OK ;
   // 定义一个sdb对象用来连接数据库
   sdb*                 connection ;
   // 定义sdbCursor对象用来查询
   sdbCursor            cursor ;

   // 定义BSONObj对象
   BSONObj              obj ;

   // 向连接池添加一个协调节点
   ds.addCoord( "192.168.20.53:50000" ) ;
   
   // 从连接池获取一个连接
   rc = ds.getConnection( connection ) ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to get a connection, rc = " << rc << endl ;
      goto error ;
   }
   // 列出复制组
   rc = connection->listReplicaGroups( cursor ) ; 
   if ( SDB_OK != rc )
   {
      cout << "Fail to list replica groups, rc = " << rc << endl ;
      goto error ;
   }
   // 游标向前移动
   rc = cursor.next( obj ) ;
   if ( SDB_OK != rc )
   {
      cout << "sdbCursor fail to move forward, rc = " << rc << endl ;
      goto error ;
   }
   // 输出信息
   cout << obj.toString() << endl ;
   // 归还连接到连接池
   ds.releaseConnection( connection ) ;
done:  
   return ;
error:  
   goto done ; 
}


INT32 main( INT32 argc, CHAR **argv )
{
   INT32                rc = SDB_OK ;
   sdbDataSourceConf    conf ;
   sdbDataSource        ds ;

   // 设置连接池配置，userName="",passwd=""
   conf.setUserInfo( "", "" ) ;  
   // 初始化时预生成10个连接, 连接池中空闲连接不够用时每次生成10个连接
   // 最大空闲连接数为20，连接池最多维护500个连接
   conf.setConnCntInfo( 10, 10, 20, 500 ) ;        
   // 每隔60秒将连接池中多于最大空闲连接数限定的空闲连接关闭，
   // 并将存活时间过长（连接已停止收发超过keepAliveTimeout时间）的连接关闭。
   // 0表示不关心连接隔多长时间没有收发消息。       
   conf.setCheckIntervalInfo( 60, 0 ) ;
   // 每隔30s从编目节点同步协调节点信息（若为0，则表示不同步）
   conf.setSyncCoordInterval( 30 ) ;
   // 连接池采用负载均衡策略生成连接
   conf.setConnectStrategy( DS_STY_BALANCE ) ;
   // 当获取一个连接时，是否检查连接的有效性
   conf.setValidateConnection( TRUE ) ;
   // 是否启用SSL
   conf.setUseSSL( FALSE ) ;
   // 提供协调节点信息
   vector<string>        v ;
   v.push_back( "192.168.20.53:11810" );
   v.push_back( "192.168.20.53:11910" ) ;
   // 初始化连接池对象
   rc = ds.init( v, conf ) ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to init sdbDataSouce, rc = " << rc << endl ;
      goto error ;
   }

   // 启动连接池
   rc = ds.enable() ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to enable sdbDataSource, rc = " << rc << endl ;
      goto error ;
   }

   // 在单线程中或多线程中使用连接池对象
   queryTask( ds ) ;

   // 停止连接池
   rc = ds.disable() ;
   if ( SDB_OK != rc )
   {
      cout << "Fail to disable sdbDataSource, rc = " << rc << endl ;
      goto error ;
   }

   // 关闭连接池
   ds.close() ;
done:  
   return 0 ;
error:  
   goto done ;   
}
```