
在 SequoiaDB 巨杉数据库中，快照是一种得到系统当前状态的命令，主要分为以下类型：

| 快照标识 | 快照类型 | 描述 |
| -------- | -------- | ---- |
| [SDB_SNAP_CONTEXTS][CONTEXTS]  | 上下文快照 | 上下文快照列出当前数据库节点中所有的会话所对应的上下文 |
| [SDB_SNAP_CONTEXTS_CURRENT][CONTEXTS_CURRENT]  | 当前会话上下文快照 | 当前上下文快照列出当前数据库节点中当前会话所对应的上下文 |
| [SDB_SNAP_SESSIONS][SESSIONS] | 会话快照 | 会话快照列出当前数据库节点中所有的会话 |
| [SDB_SNAP_SESSIONS_CURRENT][SESSIONS_CURRENT] | 当前会话快照 | 当前会话快照列出当前数据库节点中当前的会话 |
| [SDB_SNAP_COLLECTIONS][SDB_SNAP_COLLECTIONS]  | 集合快照 | 集合快照列出当前数据库节点或集群中所有非临时集合 |
| [SDB_SNAP_COLLECTIONSPACES][COLLECTIONSPACES] | 集合空间快照 | 集合空间快照列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [SDB_SNAP_DATABASE][DATABASE] | 数据库快照 | 数据库快照列出当前数据库节点的数据库监视信息 |
| [SDB_SNAP_SYSTEM][SYSTEM] | 系统快照 | 系统快照列出当前数据库节点的系统监视信息 |
| [SDB_SNAP_CATALOG][CATALOG] | 编目信息快照 | 用于查看编目信息 |
| [SDB_SNAP_TRANSACTIONS][TRANSACTIONS]  | 事务快照 | 事务快照列出数据库中正在进行的事务信息 |
| [SDB_SNAP_TRANSACTIONS_CURRENT][TRANSACTIONS_CURRENT] | 当前事务快照 | 当前事务快照列出当前会话正在进行的事务信息 |
| [SDB_SNAP_ACCESSPLANS][ACCESSPLANS] | 访问计划缓存快照 | 访问计划缓存快照列出数据库中缓存的访问计划的信息 |
| [SDB_SNAP_HEALTH][HEALTH] | 节点健康检测快照 | 节点健康检测快照列出数据库中所有节点的健康信息 |
| [SDB_SNAP_CONFIGS][CONFIGS] | 配置快照 | 配置快照列出数据库中指定节点的配置信息 |
| [SDB_SNAP_SVCTASKS][SVCTASKS] | 服务任务快照 | 服务任务快照列出当前数据库节点中服务任务的统计信息 |
| [SDB_SNAP_SEQUENCES][SEQUENCES] | 序列快照 | 序列快照列出当前数据库的全部序列信息 |
| [SDB_SNAP_QUERIES][SNAP_QUERIES] | 查询快照 | 查询快照列出当前数据库节点中查询信息 |
| [SDB_SNAP_LOCKWAITS][SNAP_LOCKWAITS] | 锁等待快照 | 锁快照列出当前数据库节点中锁等待信息 |
| [SDB_SNAP_LATCHWAITS][SNAP_LATCHWAITS] | 闩锁等待快照 | 闩锁快照列出当前数据库节点中闩锁等待信息 |
| [SDB_SNAP_INDEXSTATS][SNAP_INDEXSTATS] | 索引统计信息快照 | 索引统计信息快照列出当前数据库的全部索引统计信息 |



>   **Note:**
>
>   用户可以通过 [snapshot()][snapshot] 获取快照。


[^_^]:
    本文使用的所有引用和链接
[CONTEXTS]:manual/Manual/Snapshot/SDB_SNAP_CONTEXTS.md
[CONTEXTS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_CONTEXTS_CURRENT.md
[SESSIONS]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS.md
[SESSIONS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_SESSIONS_CURRENT.md
[SDB_SNAP_COLLECTIONS]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONS.md
[COLLECTIONSPACES]:manual/Manual/Snapshot/SDB_SNAP_COLLECTIONSPACES.md
[DATABASE]:manual/Manual/Snapshot/SDB_SNAP_DATABASE.md
[SYSTEM]:manual/Manual/Snapshot/SDB_SNAP_SYSTEM.md
[CATALOG]:manual/Manual/Snapshot/SDB_SNAP_CATALOG.md
[TRANSACTIONS]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS.md
[TRANSACTIONS_CURRENT]:manual/Manual/Snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md
[ACCESSPLANS]:manual/Manual/Snapshot/SDB_SNAP_ACCESSPLANS.md
[HEALTH]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
[CONFIGS]:manual/Manual/Snapshot/SDB_SNAP_CONFIGS.md
[SVCTASKS]:manual/Manual/Snapshot/SDB_SNAP_SVCTASKS.md
[SEQUENCES]:manual/Manual/Snapshot/SDB_SNAP_SEQUENCES.md
[SNAP_QUERIES]:manual/Manual/Snapshot/SDB_SNAP_QUERIES.md
[SNAP_LOCKWAITS]:manual/Manual/Snapshot/SDB_SNAP_LOCKWAITS.md
[SNAP_LATCHWAITS]:manual/Manual/Snapshot/SDB_SNAP_LATCHWAITS.md
[SNAP_INDEXSTATS]:manual/Manual/Snapshot/SDB_SNAP_INDEXSTATS.md
[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
