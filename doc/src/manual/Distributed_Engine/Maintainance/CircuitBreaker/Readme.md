[^_^]:
    熔断机制

SequoiaDB 巨杉数据库在[连接池][connPool]和[复制组][replicaSet]上有一系列的熔断处理机制，以保证 SequoiaDB 集群在某些异常场景下，依然能通过相应的熔断机制（例如限速限流、剔除节点）确保集群可靠正常地工作。

本文档主要介绍了 SequoiaDB 在连接池和复制组两个方面的熔断处理情况，帮助用户深入了解 SequoiaDB 的可靠性保障机制。

连接池
----
SequoiaDB 服务端默认为每个连接创建会话线程以处理请求任务，随着会话线程越来越多，线程切换开销会越来越大，系统占用资源也会随着增加。SequoiaDB 支持使用连接池管理客户端连接，一方面能够提升连接获取效率，另一方面能灵活地指定获取连接策略，有效实现连接管理。

连接池主要从以下两方面来保证连接的可靠性：

+ 连接池最大连接数  

  SequoiaDB 支持连接池，服务端会为连接池的每个连接分配会话以处理各自连接的请求任务。每个连接在服务端都需占用系统资源，例如堆栈空间、socket 描述符和内存等资源。
  为了对服务器系统资源进行管控，SequoiaDB 会对连接池最大连接数 maxCount 进行相应限制。当客户端连接数达设置的最大连接数时，SequoiaDB 会限制进一步的连接请求。

+ 协调节点地址列表异常处理  

  在初始化连接池时，需要指定 SequoiaDB 集群协调节点地址列表，也可以通过配置决定是否向[编目节点][catalog]自动同步协调节点地址列表。如果地址列表中某个协调节点异常导致连接不上，连接池会自动剔除该节点，后续连接不再尝试连接该协调节点。

复制组
----
在 SequoiaDB 的[复制组][replicaSet]中，所有写入和更新操作均在主节点进行，各备节点通过日志文件与主节点进行数据同步。在节点磁盘空间满、网络异常或其他原因造成不能同步数据时，可能会导致主节点的写操作阻塞。为此，SequoiaDB 提供了容错与熔断机制，在发生上述异常的情况下，自动采用降级、限流或剔除节点的方式避免写操作阻塞，以保证业务的正常运行。


[^_^]:
    TODO:以下链接需要后续根据实际章节继续跳转

[connPool]:manual/Distributed_Engine/Maintainance/CircuitBreaker/connection_pool.md
[catalog]:manual/Distributed_Engine/Architecture/Node/catalog_node.md
[coord]:manual/Distributed_Engine/Architecture/Node/coord_node.md
[replicaSet]:manual/Distributed_Engine/Architecture/Replication/Readme.md
[beat]:manual/Distributed_Engine/Architecture/Replication/election.md

