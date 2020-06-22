监控是一种监视当前系统状态的方式。在 SequoiaDB 中，用户可以使用快照（SNAPSHOT）与列表（LIST）命令进行系统监控。

>   **Note:** 
>
>   如果在集群环境下查询快照，连接协调节点就可以获取。

连接协调节点，默认是获取整个集群的快照信息，如：

```lang-javascript
> db.snapshot( SDB_SNAP_SYSTEM )
```

要获取指定分区组的快照信息，使用条件查询，如：

```lang-javascript
> db.snapshot( SDB_SNAP_SYSTEM, { GroupName: "group1" } )
```

要获取指定节点的快照信息，如：

```lang-javascript
> db.snapshot( SDB_SNAP_SYSTEM, { HostName: "hostname1", svcname: "11810" } )
```

##快照##

[快照](database_management/monitoring/snapshot/snapshot.md) 是一种得到系统当前状态的命令，主要分为以下类型：

*   [上下文快照](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS.md)
*   [当前会话上下文快照](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS_CURRENT.md)
*   [会话快照](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS.md)
*   [当前会话快照](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS_CURRENT.md)
*   [集合快照](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONS.md)
*   [集合空间快照](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONSPACES.md)
*   [数据库快照](database_management/monitoring/snapshot/SDB_SNAP_DATABASE.md)
*   [系统快照](database_management/monitoring/snapshot/SDB_SNAP_SYSTEM.md)
*   [编目信息快照](database_management/monitoring/snapshot/SDB_SNAP_CATALOG.md)
*   [事务快照](database_management/monitoring/snapshot/SDB_SNAP_TRANSACTIONS.md)
*   [当前事务快照](database_management/monitoring/snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md)
*   [访问计划缓存快照](database_management/monitoring/snapshot/SDB_SNAP_ACCESSPLANS.md)
*   [节点健康检测快照](database_management/monitoring/snapshot/SDB_SNAP_HEALTH.md)
*   [配置快照](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md)
*   [序列快照](database_management/monitoring/snapshot/SDB_SNAP_SEQUENCES.md)
*   [服务任务快照](database_management/monitoring/snapshot/SDB_SNAP_SVCTASKS.md)
*   [查询快照](database_management/monitoring/snapshot/SDB_SNAP_QUERIES.md)
*   [锁等待快照](database_management/monitoring/snapshot/SDB_SNAP_LOCKWAITS.md)
*   [闩锁等待快照](database_management/monitoring/snapshot/SDB_SNAP_LATCHWAITS.md)


##列表##

[列表](database_management/monitoring/list/list.md) 是一种轻量级的得到系统当前状态的命令，主要分为以下类型：

*   [上下文列表](database_management/monitoring/list/SDB_LIST_CONTEXTS.md)
*   [当前会话上下文列表](database_management/monitoring/list/SDB_LIST_CONTEXTS_CURRENT.md)
*   [会话列表](database_management/monitoring/list/SDB_LIST_SESSIONS.md)
*   [当前会话列表](database_management/monitoring/list/SDB_LIST_SESSIONS_CURRENT.md)
*   [集合列表](database_management/monitoring/list/SDB_LIST_COLLECTIONS.md)
*   [集合空间列表](database_management/monitoring/list/SDB_LIST_COLLECTIONSPACES.md)
*   [存储单元列表](database_management/monitoring/list/SDB_LIST_STORAGEUNITS.md)
*   [分区组列表](database_management/monitoring/list/SDB_LIST_GROUPS.md)
*   [事务列表](database_management/monitoring/list/SDB_LIST_TRANSACTIONS.md)
*   [当前事务列表](database_management/monitoring/list/SDB_LIST_TRANSACTIONS_CURRENT.md)
*   [序列列表](database_management/monitoring/list/SDB_LIST_SEQUENCES.md)
*   [备份列表](database_management/monitoring/list/SDB_LIST_BACKUPS.md)
*   [服务任务列表](database_management/monitoring/list/SDB_LIST_SVCTASKS.md)
*   [用户列表](database_management/monitoring/list/SDB_LIST_USERS.md)

> **Note:**
>
> 单位说明：
>
> - 描述中没有标志单位时：
>   1. 监控信息中关于存储的单位为字节。
>   2. 监控信息中关于数量的单位为条数。
>   3. 监控信息中关于网络的单位为字节。
>
> - 描述中有标志单位时，以描述为准。
