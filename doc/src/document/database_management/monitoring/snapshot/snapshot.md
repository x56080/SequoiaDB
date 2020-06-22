在 SequoiaDB 中，快照是一种得到系统当前状态的命令，主要分为以下类型：

| 快照标示 | 快照类型 | 描述 |
| -------- | -------- | ---- |
| [SDB_SNAP_CONTEXTS](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS.md) | 上下文快照 | 上下文快照列出当前数据库节点中所有的会话所对应的上下文 |
| [SDB_SNAP_CONTEXTS_CURRENT](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS_CURRENT.md) | 当前会话上下文快照 | 当前上下文快照列出当前数据库节点中当前会话所对应的上下文 |
| [SDB_SNAP_SESSIONS](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS.md) | 会话快照 | 会话快照列出当前数据库节点中所有的会话 |
| [SDB_SNAP_SESSIONS_CURRENT](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS_CURRENT.md) | 当前会话快照 | 当前会话快照列出当前数据库节点中当前的会话 |
| [SDB_SNAP_COLLECTIONS](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONS.md) | 集合快照 | 集合快照列出当前数据库节点或集群中所有非临时集合 |
| [SDB_SNAP_COLLECTIONSPACES](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONSPACES.md) | 集合空间快照 | 集合空间快照列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [SDB_SNAP_DATABASE](database_management/monitoring/snapshot/SDB_SNAP_DATABASE.md) | 数据库快照 | 数据库快照列出当前数据库节点的数据库监视信息 |
| [SDB_SNAP_SYSTEM](database_management/monitoring/snapshot/SDB_SNAP_SYSTEM.md) | 系统快照 | 系统快照列出当前数据库节点的系统监视信息 |
| [SDB_SNAP_CATALOG](database_management/monitoring/snapshot/SDB_SNAP_CATALOG.md) | 编目信息快照 | 用于查看编目信息 |
| [SDB_SNAP_TRANSACTIONS](database_management/monitoring/snapshot/SDB_SNAP_TRANSACTIONS.md) | 事务快照 | 事务快照列出数据库中正在进行的事务信息 |
| [SDB_SNAP_TRANSACTIONS_CURRENT](database_management/monitoring/snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md) | 当前事务快照 | 当前事务快照列出当前会话正在进行的事务信息 |
| [SDB_SNAP_ACCESSPLANS](database_management/monitoring/snapshot/SDB_SNAP_ACCESSPLANS.md) | 访问计划缓存快照 | 访问计划缓存快照列出数据库中缓存的访问计划的信息 |
| [SDB_SNAP_HEALTH](database_management/monitoring/snapshot/SDB_SNAP_HEALTH.md) | 节点健康检测快照 | 节点健康检测快照列出数据库中所有节点的健康信息 |
| [SDB_SNAP_CONFIGS](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md) | 配置快照 | 配置快照列出数据库中指定节点的配置信息 |
| [SDB_SNAP_SVCTASKS](database_management/monitoring/snapshot/SDB_SNAP_SVCTASKS.md) | 服务任务快照 | 服务任务快照列出当前数据库节点中服务任务的统计信息 |
| [SDB_SNAP_SEQUENCES](database_management/monitoring/snapshot/SDB_SNAP_SEQUENCES.md) | 序列快照 | 序列快照列出当前数据库的全部序列信息 |
| [SDB_SNAP_QUERIES](database_management/monitoring/snapshot/SDB_SNAP_QUERIES.md) | 查询快照 | 查询快照列出当前数据库节点中查询信息 |
| [SDB_SNAP_LOCKWAITS](database_management/monitoring/snapshot/SDB_SNAP_LOCKWAITS.md) | 锁等待快照 | 锁快照列出当前数据库节点中锁等待信息 |
| [SDB_SNAP_LATCHWAITS](database_management/monitoring/snapshot/SDB_SNAP_LATCHWAITS.md) | 闩锁等待快照 | 闩锁快照列出当前数据库节点中闩锁等待信息 |



>   **Note:**
>
>   用户可以通过调用 ```Sdb.snapshot()``` 来获取快照，请参见：[Sdb.snapshot()](reference/Sequoiadb_command/Sdb/snapshot.md)。
