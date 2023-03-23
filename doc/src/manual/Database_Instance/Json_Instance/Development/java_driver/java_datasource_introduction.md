Java 驱动连接池用于创建和管理连接。通过连接复用以减少创建连接的资源消耗、提升获取连接的效率。对于需要频繁创建连接的场景，建议使用连接池。

##连接池用法##

连接池的使用步骤主要如下：

1. 创建 SequoiadbDatasource 对象
2. 调用其 getConnection/releaseConnection 方法获取/释放连接
3. 关闭连接池

详情可查看 [Java API][api]。完整的连接池使用示例请参考 SequoiaDB 安装目录下的 samples/Java/com/sequoiadb/samples/Datasource.java

###创建连接池对象###

指定 SequoiaDB 集群的 coord 地址、鉴权信息，以及设置连接池配置参数，之后创建连接池对象。

 ```lang-java
 ArrayList<String> addrs = new ArrayList<String>();
 addrs.add("sdbserver1:11810");  // SequoiaDB 集群的协调节点地址
 addrs.add("sdbserver2:11810");
 addrs.add("sdbserver3:11810");
 
 String userName = "admin";
 String password = "admin";
 
 // 连接池参数配置
 DatasourceOptions dsOpt = new DatasourceOptions();
 dsOpt.setMaxCount(500);
 dsOpt.setMaxIdleCount(50);
 dsOpt.setMinIdleCount(20);

 // 连接参数配置
 ConfigOptions nwOpt = new ConfigOptions();
 nwOpt.setConnectTimeout(200);
 nwOpt.setMaxAutoConnectRetryTime(0);
 
 SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress(addrs)
                .userConfig(new UserConfig(userName, password))
                .datasourceOptions(dsOpt)
                .configOptions(nwOpt)
                .build();
 ```

> **Note:**
>
> * 在执行上述程序的机器上，需要配置 Sequoiadb 集群协调节点的主机名/IP地址映射关系。否则上述代码执行时将无法识别 sdbserver1、sdbserver2、sdbserver3。
>
> * 如果某一协调节点不可用，如  sdbserver3:11810 ，那么连接池将不会使用其创建新连接。

###使用连接池###

使用连接池获取、归还连接。

 ```lang-java
 Sequoiadb db = null;
 try {
     // 获取连接
     db = ds.getConnection();
 
     // 使用连接 db...
 } catch (BaseException | InterruptedException e) {
     // 异常处理
 } finally {
     // 归还连接
     if ( db != null ) {
         ds.releaseConnection( db );
     }
 }
 ```

###关闭连接池###

 ```lang-java
 ds.close();
 ```

##连接池配置##

连接池主要使用如下配置：

* DatasourceOptions 连接池配置

* ConfigOptions 连接配置

* Location 配置

> **Note:**
>
> 各种配置项详情可查看 [Java API][api]。

###连接池配置###

用于控制连接池的运行。常用配置如下：

* 设置连接池可管理的最大连接数量，默认值为 500。当连接数量达到上限时，连接池将不再创建新连接。
 ```lang-java
 dsOpt.setMaxCount(500);
 ```

* 设置连接池中闲置连接（可直接使用的连接）数量。连接池将动态维持池中的闲置连接数量在 [MinIdleCount, MaxIdleCount] 范围中。
 ```lang-java
 dsOpt.setMaxIdleCount(50);
 dsOpt.setMinIdleCount(20);
 ```

* 设置每次创建新连接的数量，默认值为 10。当连接池闲置连接数量小于 MinIdleCount 时，连接池就会触发连接创建任务，每次最多创建 DeltaIncCount 数量的新连接。
 ```lang-java
 dsOpt.setDeltaIncCount(10);
 ```

* 设置连接池内闲置连接的最大闲置时间，默认值为 0（表示最大闲置时间为无穷大），单位：毫秒。连接池会定期检查池内的闲置连接，并销毁闲置时间超时的连接。
 ```lang-java
 dsOpt.setKeepAliveTimeout(0);
 ```

* 设置闲置连接检查任务的执行周期，默认值为 60000（1 分钟），单位：毫秒。该任务会定期检查闲置连接的数量，以及它们的闲置时间是否超时。
 ```lang-java
 dsOpt.setCheckInterval(60 * 1000);
 ```

###连接配置###

连接池创建新连接时使用的网络参数，常用配置如下：

* 设置连接超时时间，单位：毫秒。
 ```lang-java
 nwOpt.setConnectTimeout(200);
 ```

* 设置连接失败的重试时间，单位：毫秒。建议设置为 0，表示不重试。
 ```lang-java
 nwOpt.setMaxAutoConnectRetryTime(0);
 ```

###Location 配置###

设置连接池所属 Location。连接池将根据 Location 对地址进行分级访问：
 1. 优先访问相同 Location 的地址
 2. 次级访问亲和 Location 的地址
 3. 最后访问剩余有效地址

示例：

1. 假设集群协调节点及 location 信息如下：
 ```lang-text
 sdbserver1:11810  "guangdong.guangzhou"
 sdbserver2:11810  "guangdong.shenzhen"
 sdbserver3:11810  "guangdong"
 sdbserver4:11810  "shanghai"
 sdbserver5:11810  "chongqing"
 ```
 可通过 [setLocation()][setLocation_link] 接口设置协调节点的 Location

2. 创建连接池，并指定其 Location 为 "guangdong.guangzhou"
 ```lang-java
 ArrayList<String> addrs = new ArrayList<String>();
 addrs.add("sdbserver1:11810");
 addrs.add("sdbserver2:11810");
 addrs.add("sdbserver3:11810");
 addrs.add("sdbserver4:11810");
 addrs.add("sdbserver5:11810");

 String location = "guangdong.guangzhou";
 SequoiadbDatasource ds = SequoiadbDatasource.builder()
                .serverAddress(addrs)
                .userConfig(new UserConfig(userName, password))
                .datasourceOptions(dsOpt)
                .configOptions(nwOpt)
                .location(location)
                .build();
 ```
 地址访问分级：
 1. 相同 Location 地址：sdbserver1:11810
 2. 亲和 Location 地址：sdbserver2:11810、sdbserver3:11810
 3. 剩余可用地址：sdbserver4:11810、sdbserver5:11810

 当 sdbserver1:11810 故障不可用时，连接池将自动切换为访问 sdbserver2:11810、sdbserver3:11810。  
 当 sdbserver2:11810、sdbserver3:11810 都故障不可用时，连接池将自动切换为访问 sdbserver4:11810、sdbserver5:11810。  
 当 sdbserver1:11810 恢复时，连接池将自动切换为访问 sdbserver1:11810。


[^_^]:
     本文使用的所有引用和链接
[api]:api/java/html/index.html
[setLocation_link]:manual/Manual/Sequoiadb_Command/SdbNode/setLocation.md