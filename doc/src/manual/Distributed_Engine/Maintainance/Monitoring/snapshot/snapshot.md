在 SequoiaDB 中，快照是一种得到系统当前状态的命令，主要分为以下类型：

| 快照标识 | 快照类型 | 描述 |
| -------- | -------- | ---- |
| [SDB_SNAP_CONTEXTS][SNAP_CONTEXTS] | 上下文快照 | 上下文快照列出当前数据库节点中所有的会话所对应的上下文 |
| [SDB_SNAP_CONTEXTS_CURRENT][SNAP_CONTEXTS_CURRENT] | 当前会话上下文快照 | 当前上下文快照列出当前数据库节点中当前会话所对应的上下文 |
| [SDB_SNAP_SESSIONS][SNAP_SESSIONS] | 会话快照 | 会话快照列出当前数据库节点中所有的会话 |
| [SDB_SNAP_SESSIONS_CURRENT][SNAP_SESSIONS_CURRENT] | 当前会话快照 | 当前会话快照列出当前数据库节点中当前的会话 |
| [SDB_SNAP_COLLECTIONS][SNAP_COLLECTIONS] | 集合快照 | 集合快照列出当前数据库节点或集群中所有非临时集合 |
| [SDB_SNAP_COLLECTIONSPACES][SNAP_COLLECTIONSPACES] | 集合空间快照 | 集合空间快照列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [SDB_SNAP_DATABASE][SNAP_DATABASE] | 数据库快照 | 数据库快照列出当前数据库节点的数据库监视信息 |
| [SDB_SNAP_SYSTEM][SNAP_SYSTEM] | 系统快照 | 系统快照列出当前数据库节点的系统监视信息 |
| [SDB_SNAP_CATALOG][SNAP_CATALOG] | 编目信息快照 | 用于查看编目信息 |
| [SDB_SNAP_TRANSACTIONS][SNAP_TRANSACTIONS] | 事务快照 | 事务快照列出数据库中正在进行的事务信息 |
| [SDB_SNAP_TRANSACTIONS_CURRENT][SNAP_TRANSACTIONS_CURRENT] | 当前事务快照 | 当前事务快照列出当前会话正在进行的事务信息 |
| [SDB_SNAP_ACCESSPLANS][SNAP_ACCESSPLANS] | 访问计划缓存快照 | 访问计划缓存快照列出数据库中缓存的访问计划的信息 |
| [SDB_SNAP_HEALTH][SNAP_HEALTH]| 节点健康检测快照 | 节点健康检测快照列出数据库中所有节点的健康信息 |
| [SDB_SNAP_CONFIGS][SNAP_CONFIGS] | 配置快照 | 配置快照列出数据库中指定节点的配置信息 |
| [SDB_SNAP_SVCTASKS][SNAP_SVCTASKS] | 服务任务快照 | 服务任务快照列出当前数据库节点中服务任务的统计信息 |
| [SDB_SNAP_SEQUENCES][SNAP_SEQUENCES] | 序列快照 | 序列快照列出当前数据库的全部序列信息 |
| [SDB_SNAP_QUERIES][SNAP_QUERIES] | 查询快照 | 查询快照列出当前数据库节点中查询信息 |
| [SDB_SNAP_LOCKWAITS][SNAP_LOCKWAITS] | 锁等待快照 | 锁快照列出当前数据库节点中锁等待信息 |
| [SDB_SNAP_LATCHWAITS][SNAP_LATCHWAITS] | 闩锁等待快照 | 闩锁快照列出当前数据库节点中闩锁等待信息 |
| [SDB_SNAP_INDEXSTATS][SNAP_INDEXSTATS] | 索引统计信息快照 | 索引统计信息快照列出当前数据库的全部索引统计信息 |



>   **Note:**
>
>   用户可以通过调用 ```Sdb.snapshot()``` 来获取快照，请参见：[Sdb.snapshot()][snapshot]。


[^_^]:
    本文使用的所有引用及链接
[SNAP_CONTEXTS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_CONTEXTS.md
[SNAP_CONTEXTS_CURRENT]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_CONTEXTS_CURRENT.md
[SNAP_SESSIONS_CURRENT]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_SESSIONS_CURRENT.md
[SNAP_COLLECTIONSPACES]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_COLLECTIONSPACES.md
[SNAP_COLLECTIONS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_COLLECTIONS.md
[SNAP_SESSIONS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_SESSIONS.md
[SNAP_DATABASE]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_DATABASE.md
[SNAP_SYSTEM]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_SYSTEM.md
[SNAP_CATALOG]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_CATALOG.md
[SNAP_TRANSACTIONS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_TRANSACTIONS.md
[SNAP_TRANSACTIONS_CURRENT]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md
[SNAP_ACCESSPLANS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_ACCESSPLANS.md
[SNAP_HEALTH]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_HEALTH.md
[SNAP_CONFIGS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_CONFIGS.md
[SNAP_SVCTASKS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_SVCTASKS.md
[SNAP_SEQUENCES]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_SEQUENCES.md
[SNAP_QUERIES]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_QUERIES.md
[SNAP_LOCKWAITS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_LOCKWAITS.md
[SNAP_LATCHWAITS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_LATCHWAITS.md
[SNAP_INDEXSTATS]:manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_INDEXSTATS.md
[snapshot]:reference/Sequoiadb_command/Sdb/snapshot.md