在 SequoiaDB 中，列表是一种轻量级的得到系统当前状态的命令，主要分为以下类型：

| 列表标示 | 列表类型 | 描述 |
| -------- | -------- | ---- |
| [SDB_LIST_CONTEXTS](database_management/monitoring/list/SDB_LIST_CONTEXTS.md) | 上下文列表 | 上下文列表列出当前数据库节点中所有的会话所对应的上下文 |
| [SDB_LIST_CONTEXTS_CURRENT](database_management/monitoring/list/SDB_LIST_CONTEXTS_CURRENT.md) | 当前会话上下文列表 | 当前上下文列表列出当前数据库节点中当前会话所对应的上下文 |
| [SDB_LIST_SESSIONS](database_management/monitoring/list/SDB_LIST_SESSIONS.md) | 会话列表 | 会话列表列出当前数据库节点中所有的会话 |
| [SDB_LIST_SESSIONS_CURRENT](database_management/monitoring/list/SDB_LIST_SESSIONS_CURRENT.md) | 当前会话列表 | 当前会话列表列出当前数据库节点中当前的会话 |
| [SDB_LIST_COLLECTIONS](database_management/monitoring/list/SDB_LIST_COLLECTIONS.md) | 集合列表 | 集合列表列出当前数据库节点或集群中所有非临时集合 |
| [SDB_LIST_COLLECTIONSPACES](database_management/monitoring/list/SDB_LIST_COLLECTIONSPACES.md) | 集合空间列表 | 集合空间列表列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [SDB_LIST_STORAGEUNITS](database_management/monitoring/list/SDB_LIST_STORAGEUNITS.md) | 存储单元列表 | 存储单元列表列出当前数据库节点的全部存储单元信息 |
| [SDB_LIST_GROUPS](database_management/monitoring/list/SDB_LIST_GROUPS.md) | 分区组列表 | 分区组列表列出当前集群中的所有分区信息 |
| [SDB_LIST_TRANSACTIONS](database_management/monitoring/list/SDB_LIST_TRANSACTIONS.md) | 事务列表 | 事务列表列出数据库中正在进行的事务信息 |
| [SDB_LIST_TRANSACTIONS_CURRENT](database_management/monitoring/list/SDB_LIST_TRANSACTIONS_CURRENT.md) | 当前事务列表 | 当前事务列表列出当前会话正在进行的事务信息 |

>   **Note:**
>
>   用户可以通过调用 ```Sdb.list()``` 来获取列表，请参见：[Sdb.list()](reference/Sequoiadb_command/Sdb/list.md)。
