监控是一种监视当前系统状态的方式。在 SequoiaDB 中，用户可以使用快照（SNAPSHOT）与列表（LIST）命令进行系统监控。

##快照视图##

快照是一种得到系统当前状态的命令，主要分为以下类型：

| 快照标示 | 对应 sdbshell 接口标示 | 快照类型 | 描述 |
| -------- | -------- | -------- | ---- |
| [$SNAPSHOT_CONTEXT](reference/SQL_grammar/monitoring/SNAPSHOT_CONTEXT.md) | [SDB_SNAP_CONTEXTS](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS.md) | 上下文快照 | 上下文快照列出当前数据库节点中所有的会话所对应的上下文 |
| [$SNAPSHOT_CONTEXT_CUR](reference/SQL_grammar/monitoring/SNAPSHOT_CONTEXT_CUR.md) | [SDB_SNAP_CONTEXTS_CURRENT](database_management/monitoring/snapshot/SDB_SNAP_CONTEXTS_CURRENT.md) | 当前会话上下文快照 | 当前上下文快照列出当前数据库节点中当前会话所对应的上下文 |
| [$SNAPSHOT_SESSION](reference/SQL_grammar/monitoring/SNAPSHOT_SESSION.md) | [SDB_SNAP_SESSIONS](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS.md) | 会话快照 | 会话快照列出当前数据库节点中所有的会话 |
| [$SNAPSHOT_SESSION_CUR](reference/SQL_grammar/monitoring/SNAPSHOT_SESSION_CUR.md) | [SDB_SNAP_SESSIONS_CURRENT](database_management/monitoring/snapshot/SDB_SNAP_SESSIONS_CURRENT.md) | 当前会话快照 | 当前会话快照列出当前数据库节点中当前的会话 |
| [$SNAPSHOT_CL](reference/SQL_grammar/monitoring/SNAPSHOT_CL.md) | [SDB_SNAP_COLLECTIONS](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONS.md) | 集合快照 | 集合快照列出当前数据库节点或集群中所有非临时集合 |
| [$SNAPSHOT_CS](reference/SQL_grammar/monitoring/SNAPSHOT_CS.md) | [SDB_SNAP_COLLECTIONSPACES](database_management/monitoring/snapshot/SDB_SNAP_COLLECTIONSPACES.md) | 集合空间快照 | 集合空间快照列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [$SNAPSHOT_DB](reference/SQL_grammar/monitoring/SNAPSHOT_DB.md) | [SDB_SNAP_DATABASE](database_management/monitoring/snapshot/SDB_SNAP_DATABASE.md) | 数据库快照 | 数据库快照列出当前数据库节点的数据库监视信息 |
| [$SNAPSHOT_SYSTEM](reference/SQL_grammar/monitoring/SNAPSHOT_SYSTEM.md) | [SDB_SNAP_SYSTEM](database_management/monitoring/snapshot/SDB_SNAP_SYSTEM.md) | 系统快照 | 系统快照列出当前数据库节点的系统监视信息 |
| [$SNAPSHOT_CATA](reference/SQL_grammar/monitoring/SNAPSHOT_CATA.md) | [SDB_SNAP_CATALOG](database_management/monitoring/snapshot/SDB_SNAP_CATALOG.md) | 编目信息快照 | 用于查看编目信息 |
| [$SNAPSHOT_TRANS](reference/SQL_grammar/monitoring/SNAPSHOT_TRANS.md) | [SDB_SNAP_TRANSACTIONS](database_management/monitoring/snapshot/SDB_SNAP_TRANSACTIONS.md) | 事务快照 | 事务快照列出数据库中正在进行的事务信息 |
| [$SNAPSHOT_TRANS_CUR](reference/SQL_grammar/monitoring/SNAPSHOT_TRANS_CUR.md) | [SDB_SNAP_TRANSACTIONS_CURRENT](database_management/monitoring/snapshot/SDB_SNAP_TRANSACTIONS_CURRENT.md) | 当前事务快照 | 当前事务快照列出当前会话正在进行的事务信息 |
| [$SNAPSHOT_ACCESSPLANS](reference/SQL_grammar/monitoring/SNAPSHOT_ACCESSPLANS.md) | [SDB_SNAP_ACCESSPLANS](database_management/monitoring/snapshot/SDB_SNAP_ACCESSPLANS.md) | 访问计划缓存快照 | 访问计划缓存快照列出数据库中缓存的访问计划的信息 |
| [$SNAPSHOT_HEALTH](reference/SQL_grammar/monitoring/SNAPSHOT_HEALTH.md) | [SDB_SNAP_HEALTH](database_management/monitoring/snapshot/SDB_SNAP_HEALTH.md) | 节点健康检测快照 | 节点健康检测快照列出数据库中所有节点的健康信息 |
| [$SNAPSHOT_CONFIGS](reference/SQL_grammar/monitoring/SNAPSHOT_CONFIGS.md) | [SDB_SNAP_CONFIGS](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md) | 配置快照 | 配置快照列出数据库中指定节点的配置信息 |
| [$SNAPSHOT_SEQUENCES](reference/SQL_grammar/monitoring/SNAPSHOT_SEQUENCES.md) | [SDB_SNAP_SEQUENCES](database_management/monitoring/snapshot/SDB_SNAP_SEQUENCES.md) | 序列快照 | 序列快照列出当前数据库的全部序列信息 |
| [$SNAPSHOT_SVCTASKS](reference/SQL_grammar/monitoring/SNAPSHOT_SVCTASKS.md) | [SDB_SNAP_SVCTASKS](database_management/monitoring/snapshot/SDB_SNAP_SVCTASKS.md) | 服务任务快照 | 服务任务快照列出当前数据库节点中服务任务的统计信息 |
| [$SNAPSHOT_QUERIES](reference/SQL_grammar/monitoring/SNAPSHOT_QUERIES.md) | [SDB_SNAP_QUERIES](database_management/monitoring/snapshot/SDB_SNAP_QUERIES.md) | 查询快照 | 查询快照列出当前数据库节点中查询信息 |
| [$SNAPSHOT_LOCKWAITS](reference/SQL_grammar/monitoring/SNAPSHOT_LOCKWAITS.md) | [SDB_SNAP_LOCKWAITS](database_management/monitoring/snapshot/SDB_SNAP_LOCKWAITS.md) | 锁等待快照 | 等待锁快照列出当前数据库节点中锁等待信息 |
| [$SNAPSHOT_LATCHWAITS](reference/SQL_grammar/monitoring/SNAPSHOT_LATCHWAITS.md) | [SDB_SNAP_LATCHWAITS](database_management/monitoring/snapshot/SDB_SNAP_LATCHWAITS.md) | 闩锁等待快照 | 闩锁等待快照列出当前数据库节点中闩锁等待信息 |

##列表视图##

列表是一种轻量级的得到系统当前状态的命令，主要分为以下类型：

| 列表标示 | 对应 sdbshell 接口标示 | 列表类型 | 描述 |
| -------- | -------- | -------- | ---- |
| [$LIST_CONTEXT](reference/SQL_grammar/monitoring/LIST_CONTEXT.md) | [SDB_LIST_CONTEXTS](database_management/monitoring/list/SDB_LIST_CONTEXTS.md) | 上下文列表 | 上下文列表列出当前数据库节点中所有的会话所对应的上下文 |
| [$LIST_CONTEXT_CUR](reference/SQL_grammar/monitoring/LIST_CONTEXT_CUR.md) | [SDB_LIST_CONTEXTS_CURRENT](database_management/monitoring/list/SDB_LIST_CONTEXTS_CURRENT.md) | 当前会话上下文列表 | 当前上下文列表列出当前数据库节点中当前会话所对应的上下文 |
| [$LIST_SESSION](reference/SQL_grammar/monitoring/LIST_SESSION.md) | [SDB_LIST_SESSIONS](database_management/monitoring/list/SDB_LIST_SESSIONS.md) | 会话列表 | 会话列表列出当前数据库节点中所有的会话 |
| [$LIST_SESSION_CUR](reference/SQL_grammar/monitoring/LIST_SESSION_CUR.md) | [SDB_LIST_SESSIONS_CURRENT](database_management/monitoring/list/SDB_LIST_SESSIONS_CURRENT.md) | 当前会话列表 | 当前会话列表列出当前数据库节点中当前的会话 |
| [$LIST_CL](reference/SQL_grammar/monitoring/LIST_CL.md) | [SDB_LIST_COLLECTIONS](database_management/monitoring/list/SDB_LIST_COLLECTIONS.md) | 集合列表 | 集合列表列出当前数据库节点或集群中所有非临时集合 |
| [$LIST_CS](reference/SQL_grammar/monitoring/LIST_CS.md) | [SDB_LIST_COLLECTIONSPACES](database_management/monitoring/list/SDB_LIST_COLLECTIONSPACES.md) | 集合空间列表 | 集合空间列表列出当前数据库节点或集群中所有集合空间（编目集合空间除外） |
| [$LIST_SU](reference/SQL_grammar/monitoring/LIST_SU.md) | [SDB_LIST_STORAGEUNITS](database_management/monitoring/list/SDB_LIST_STORAGEUNITS.md) | 存储单元列表 | 存储单元列表列出当前数据库节点的全部存储单元信息 |
| [$LIST_GROUP](reference/SQL_grammar/monitoring/LIST_GROUP.md) | [SDB_LIST_GROUPS](database_management/monitoring/list/SDB_LIST_GROUPS.md) | 分区组列表 | 分区组列表列出当前集群中的所有分区信息 |
| [$LIST_TRANS](reference/SQL_grammar/monitoring/LIST_TRANS.md) | [SDB_LIST_TRANSACTIONS](database_management/monitoring/list/SDB_LIST_TRANSACTIONS.md) | 事务列表 | 事务列表列出数据库中正在进行的事务信息 |
| [$LIST_TRANS_CUR](reference/SQL_grammar/monitoring/LIST_TRANS_CUR.md) | [SDB_LIST_TRANSACTIONS_CURRENT](database_management/monitoring/list/SDB_LIST_TRANSACTIONS_CURRENT.md) | 当前事务列表 | 当前事务列表列出当前会话正在进行的事务信息 |
| [$LIST_SEQUENCES](reference/SQL_grammar/monitoring/LIST_SEQUENCES.md) | [SDB_LIST_SEQUENCES](database_management/monitoring/list/SDB_LIST_SEQUENCES.md) | 序列列表 | 序列列表列出当前数据库中所有的序列信息 |
| [$LIST_BACKUP](reference/SQL_grammar/monitoring/LIST_BACKUP.md) | [SDB_LIST_BACKUPS](database_management/monitoring/list/SDB_LIST_BACKUPS.md) | 备份列表 | 备份列表列出当前数据库的备份信息 |
| [$LIST_SVCTASKS](reference/SQL_grammar/monitoring/LIST_SVCTASKS.md) | [SDB_LIST_SVCTASKS](database_management/monitoring/list/SDB_LIST_SVCTASKS.md) | 服务任务列表 | 服务任务列表列出当前数据库节点中所有的服务任务 |
| [$LIST_USER](reference/SQL_grammar/monitoring/LIST_USER.md) | [SDB_LIST_USERS](database_management/monitoring/list/SDB_LIST_USERS.md) | 用户列表 | 用户列表列出当前集群中的所有用户信息 |

##SQL到SequoiaDB映射表##

下表列出了 SQL 快照查询语句的操作在 API 中对应的[快照操作](reference/Sequoiadb_command/Sdb/snapshot.md)：

| SQL 语句 | API 语句        |
| -------- | -------------- |
|  select \<sel\> from $\<snapshot\> where \<cond\> order by \<sort\>  |   db.snapshot( <snapType>, [cond], [sel], [sort] ) |
| db.exec( "select * from $SNAPSHOT_CONTEXT where SessionID = 20" ) | 过滤指定条件的记录。db.snapshot(SDB_SNAP_CONTEXTS, { SessionID: 20 } ) |
| db.exec( " select NodeName from $SNAPSHOT_CONTEXT " ) | 只显示记录的指定字段。db.snapshot(SDB_SNAP_CONTEXTS, {}, { NodeName:""} ) |
| db.exec( " select * from $SNAPSHOT_CONTEXT order by SessionID" ) | 根据指定字段进行排序。db.snapshot(SDB_SNAP_CONTEXTS, {}, {}, { "SessionID": 1 } ) |

下面列出了 SQL 快照查询语句的操作在 API 中使用指定[快照查询参数](reference/Sequoiadb_command/AuxiliaryObjects/SdbSnapshotOption.md)的对应快照操作：

```
select <sel> from $<snapshot>
               where <cond>
               order by <sort>
               limit <limit>
               offset <skip> /*+use_option(<options>)*/
```

对应

```
SdbSnapshotOption[.cond(<cond>)]
                 [.sel(<sel>)]
                 [.sort(<sort>)]
                 [.options(<options>)]
                 [.skip(<skip>)]
                 [.limit(<limit>)]
```

###cond(\<cond\>)###

对应 SQL 语法[ where 子句](reference/SQL_grammar/clause/where.md)

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( "select * from $SNAPSHOT_CONTEXT where SessionID = 22" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( { SessionID: 22 } ) ) |

###sel(\<sel\>)###

对应 SQL 语法的字段名

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( "select SessionID from $SNAPSHOT_CONTEXT" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).sel( { SessionID: "" } ) ) |

###sort(\<sort\>)###

对应 SQL 语法[ order by 子句](reference/SQL_grammar/clause/order_by.md)

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( " select * from $SNAPSHOT_CONTEXT order by SessionID" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).sort( { SessionID: 1 } ) ) |

###options(\<options\>)###

对应 SQL 语法[ hint 子句](reference/SQL_grammar/clause/hint.md)中的 use_option 部分。

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec('select * from $SNAPSHOT_CONFIGS where GroupName = "db1" and ServiceName = "20000" /*+use_option(Mode, local) use_option(Expand, false)*/') | db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().cond( { GroupName:'db1', ServiceName:'20000' } ).options( { "Mode": "local", "Expand": false } ) ) |

###skip(\<skip\>)###

对应 SQL 语法[ offset 子句](reference/SQL_grammar/clause/offset.md)

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( " select * from $SNAPSHOT_CONTEXT offset 2" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).skip( 2 ) ) |

###limit(\<limit\>)###

对应 SQL 语法[ limit 子句](reference/SQL_grammar/clause/limit.md)

| SQL 语句 | API 语句        |
| -------- | -------------- |
| db.exec( "select * from $SNAPSHOT_CONTEXT limit 1" ) | db.snapshot( SDB_SNAP_CONTEXTS, new SdbSnapshotOption().cond( {} ).limit( 1 ) ) |

##SQL使用命令位置参数##

[命令位置参数](reference/Sequoiadb_command/location.md) 是用于控制命令执行的位置信息。SQL 可以用 where 语句来使用命令位置参数。

###示例###

* 控制快照在指定节点运行：

```lang-javascript
> db.exec("select * from $SNAPSHOT_CONTEXT where Role = 'catalog'")
{
  "NodeName": "hostname:30000",
  "SessionID": 21,
  "Contexts": [
    {
      "ContextID": 7764,
      "Type": "DUMP",
      "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2019-06-26-17.55.42.355666"
    }
  ]
}
...
```

* 控制快照不在全局执行：

```lang-javascript
> db.exec("select * from $SNAPSHOT_CONTEXT where Global = false")
{
  "NodeName": "hostname:50000",
  "SessionID": 10,
  "Contexts": [
    {
      "ContextID": 70,
      "Type": "DUMP",
      "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2019-06-26-17.56.55.916040"
    }
  ]
}
```
