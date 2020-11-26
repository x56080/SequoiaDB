[^_^]:
    熔断机制
    作者：余卫星
    时间：20190326
    评审意见
    余卫星：初稿完成；时间：20190326
    王涛：  时间：
    许建辉：时间：
    市场部：时间：


SequoiaDB 巨杉数据库服务端默认为每个连接创建会话线程以处理请求任务，随着会话线程越来越多，线程切换开销会越来越大，系统占用资源也会随着增加。连接池通过有效管理客户端大量连接，能够有效减少开销，并且还可以通过指定不同的连接获取策略，从而达到提升性能效果。总体而言，连接池有两大优势：

- 快速获取连接请求实例
- 灵活地指定获取连接策略，有效实现连接管理

连接会话线程过多会增加系统资源的占用，为了管控系统资源，保证 SequoiaDB 服务端的连接请求控制在一定范围内，通常需要对连接池的最大连接数进行控制。

另外一方面，为了实现更好的连接请求分流，SequoiaDB 连接池会用地址列表保存集群中所有[协调节点][coord]的地址，然后根据不同的分配策略获取连接，在地址列表中的协调节点异常时，通常需要移除异常节点，以实现连接的获取效率。

本文档从这两方面进行介绍说明，帮助用户深入了解连接池相关原理。

最大连接数  
---

在初始化时，可以通过连接池的 sdbDataSourceConf 类进行配置相关参数，该类中的方法 setConnCntInfo 可以指定最大连接数，配置的更多使用方法可参见 [Java驱动->连接池][javaDriver]。

```lang-java
public class Datasource {
    public static void main(String[] args) throws InterruptedException {
        ArrayList<String> addrs = new ArrayList<String>();
        String user = "";
        String password = "";
        ConfigOptions nwOpt = new ConfigOptions();
        DatasourceOptions dsOpt = new DatasourceOptions();
        SequoiadbDatasource ds = null;
        // 设置连接池参数
        dsOpt.setMaxCount(500);                            // 连接池最多能提供 500 个连接
        ……
    }
    ……
```

dsOpt.setMaxCount(500) 方法的参数 500 即为最大连接数。

当客户端连接请求数量达到了指定的最大连接数时，后续的新连接获取则需要进行等待，具体的等待时间可以在获取连接时进行指定（默认等待时间 5s），还可以设置失败连接后的重试时间，更多使用方法可参见 [Java驱动->连接池][javaDriver]。
```lang-java
   Sequoiadb db = null;
   // 设置网络参数
   nwOpt.setConnectTimeout(500);                      // 建连超时时间为 500ms
   nwOpt.setMaxAutoConnectRetryTime(0);               // 建连失败后重试时间为 0ms
```
nwOpt.setConnectTimeout 方法的参数 timeOut 即为等待超时时间，单位为 ms。当等待时间达 timeOut 时，如果连接池中有以被获取的连接被释放成为了空闲连接，那么可以获取成功，否则获取失败。

nwOpt.setMaxAutoConnectRetryTime 可设置失败重试时间，以在获取失败后多少时间后进行重试。

连接池参数配置和连接获取的完整代码可参考 [Java驱动->连接池][javaDriver]。

协调节点异常处理  
---

连接池的连接策略 sdbDataSourceStrategy 类中，有个类成员变量地址列表，以保存 SequoiaDB 集群的所有协调节点的地址。

在初始化时，用户可以通过指定所有的协调节点地址，也可以指定其中一个。然后开启自动同步协调节点地址功能，连接池会自动进行同步保存所有协调节点地址。在后续操作中，还可以不断通过 addCoord 方法新添协调节点地址。下面为指定所有协调节点的地址的方式，更多使用可参考 [Java驱动->连接池][javaDriver]。  

```lang-java
   // 提供 coord 节点地址  
   addrs.add("192.168.10.1:11810");
   addrs.add("192.168.10.2:11810");
   addrs.add("sdbserver01:11810");
```

当连接池地址列表中其中一个协调节点异常，导致从该节点上获取新的连接请求失败时，连接池为了保持连接请求获取效率，会及时将其地址从地址列表中移除，然后后续不再尝试从该节点获取新连接。

SequoiaDB 连接池有效地提升了连接获取效率，并提供大量连接时不同策略的连接管理，从而在整体上提升了客户端和服务端性能。限制最大连接数范围，能够很好的实现对系统资源进行管控；在协调节点异常时，及时移除异常节点，很好的保证了连接获取效率。



[^_^]:
[javaDriver]:manual/Database_Instance/Json_Instance/Development/java_driver/java_driver/java_datasource_introduction.md
[coord]:manual/infrastructure/Node/coord_node.md
