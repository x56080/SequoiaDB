##概念##

- 分区组又被称为复制组，一个复制组内可以包含一个或多个数据节点（或编目节点），节点之间的数据使用异步日志复制机制，保持最终一致。

- 分区组中所有的节点之间使用复制服务端口（replname参数）进行通讯，定期相互发送心跳信息以相互验证状态。

- 分区组结构如下图：

  ![分区组结构](infrastructure/replication/sequoiadb_infrastructure_shard1.jpg)

- 每个分区组内的节点有两种状态：

  **主节点:** 主节点可做读写操作。所有写入的数据会同步写入日志文件，日志文件中的日志信息会异步写入从节点。  
  **从节点:** 从节点可做只读操作。所有主节点写入的数据会异步写入从节点。因此从节点与主节点之间可能存在暂时的数据不一致，但是复制机制可以保证数据的最终一致性。

- 分区组通过 [数据复制](infrastructure/replication/replicate.md) ，[读写分离](infrastructure/replication/read_write.md) 与 [选举机制](infrastructure/replication/vote.md) 实现高可用。
