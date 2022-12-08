[^_^]:
    分布式事务
    作者：何国明
    时间：20190817
    评审意见

SequoiaDB 巨杉数据库中的事务日志记录了事务对数据库的所有更改，是备份和恢复的重要组件，也在事务操作中被用于回滚数据。因此事务日志中通常包含 REDO 和 UNDO 两部分，其中 REDO 部分用于数据恢复和复制组节点间数据进行增量同步，UNDO 部分用于事务回滚操作恢复数据到事务操作前的状态。

如执行更新操作的事务日志中，将分别记录新值（New）和旧值（Orig）：

```lang-text
 Version: 0x00000001(1)
 LSN    : 0x0000000058b90740(236)
 PreLSN : 0x0000000058b906d0(156)
 Length : 228
 Type   : UPDATE(2)
 FullName : sample.employee
 Orig id : { "_id": { "$oid": "5c88afe31a3f5822754040d0" } }
 Orig    : { "$set" : { "balance" : 10000 } }
 New id  : { "_id": { "$oid": "5c88afe31a3f5822754040d0" } }
 New     : { "$set" : { "balance" : 8000 } }
 TransID : 0x00040105e5eac5b72953
 TransPreLSN : 0x0000000058b906d0
```

如果事务日志的记录中有事务 ID（TransID） 表示该日志记录是某个事务的事务日志，同一个事务的事务日志有相同的事务 ID。可以通过 TransPreLSN 查找同一个事务的前一条事务日志。

事务开启日志
----

事务日志中事务开启日志和事务的第一个操作合并，事务日志的事务 ID（IDAttr）带有 Start 标签的事务日志为事务的开启日志，即事务的第一个操作。

对于全局事务的日志信息，事务 ID 将同时带有 Start 和 Global 标签，同时增加事务开始的[全局逻辑时间戳][stp]（TransTime 和 TransTimeError）。

```lang-text
 Version: 0x00000001(1)
 LSN    : 0x00000000000000ec(236)
 PreLSN : 0x000000000000009c(156)
 Length : 228
 ...
 TransID : 0x00040105e5eac5b72953
 IDAttr  ：Start | Global
 TransTime : 1660171385776467
 TransTimeError : 1000000
```

事务预提交日志
----

事务的预提交日志表示事务将进入 WAIT-COMMIT 状态：

```lang-text
 Version: 0x00000001(1)
 LSN    : 0x0000000058b90740(1488521024)
 PreLSN : 0x0000000058b906d0(1488520912)
 Length : 100
 Type   : COMMIT(12)
 FirstLSN : 0x0000000058b90670
 Attr     : 1(Pre-Commit)
 NodeNum  : 1
 Nodes    : [ (1001,1003) ]
 TransID  : 0x00040105e5eac5b72953
 IDAttr   : Global
 TransTime : 1660171406974875
 TransPreLSN : 0x0000000058b906d0
```

其中：

- 事务日志中 Nodes 将标明参与事务的数据节点，用于二阶段协议出错时节点间进行协商。
- 事务日志中 TransTime 是事务进行预提交时的全局逻辑时间戳。

> **Note:**
>
> SequoiaDB 分布式事务的二阶段提交协议可参考[二阶段提交][2pc]。

事务提交日志
----

事务的提交日志表示事务已经提交：

```lang-text
 Version: 0x00000001(1)
 LSN    : 0x0000000058b907a4(1488521124)
 PreLSN : 0x0000000058b90740(1488521024)
 Length : 80
 Type   : COMMIT(12)
 FirstLSN : 0x0000000058b90670
 Attr     : 2(Commit)
 TransID  : 0x00040105e5eac5b72953
 IDAttr   : Global
 TransTime : 1660171406974875
 TransPreLSN : 0x0000000058b90740
```

其中，事务日志中的 TransTime 是事务进行提交时的全局逻辑时间戳。

事务回滚日志
----

事务回滚日志与事务之前的操作日志一一对应，事务 ID（IDAttr）带有 Rollback 标签：

- 对于更新操作，新值（New）和旧值（Orig）与原事务日志互换
- 对于插入操作，将产生删除操作的事务日志
- 对于删除操作，将产生插入操作的事务日志

```lang-text
 Version: 0x00000001(1)
 LSN    : 0x0000000058b90740(236)
 PreLSN : 0x0000000058b906d0(156)
 Length : 228
 Type   : UPDATE(2)
 FullName :sample.employee
 Orig id : { "_id": { "$oid": "5c88afe31a3f5822754040d0" } }
 Orig    : { "$set" : { "balance" : 8000 } }
 New id  : { "_id": { "$oid": "5c88afe31a3f5822754040d0" } }
 New     : { "$set" : { "balance" : 10000 } }
 TransID : 0x00040105e5eac5b72953
 IDAttr  : Rollback | Global
 TransPreLSN : 0x0000000058b906d0
```

[^_^]:
    本文使用到的所有链接

[2pc]:manual/Distributed_Engine/Architecture/Transactions/2pc.md
[stp]:manual/Distributed_Engine/Architecture/Stp/logicaltime.md
