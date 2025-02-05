查询快照可以列出数据库中正在进行的查询信息。当 [mongroupmask][configuration] 参数设置为“slowQuery:detail”或“all:detail”时，查询耗时超过 [monslowquerythreshold][configuration] 参数所规定阈值的历史查询信息会被缓存。用户可以通过指定 [viewHistory][SnapshotOption] 选项，查看历史查询信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个查询操作为一条记录。


## 标识

SDB_SNAP_QUERIES

## 协调节点字段信息

| 字段名                 | 类型     | 描述                                                |
| ---------------------- | -------- | --------------------------------------------------- |
| NodeName               | string   | 节点名，格式为\<hostname\>:\<servicename\>  |
| NodeID                 | bson array | 节点的 ID，格式为[<分区组 ID>,<节点 ID>]          |
| StartTimestamp         | string   | 查询开始时间                                        |
| EndTimestamp           | string   | 查询结束时间                                        |
| TID                    | int32    | 查询所属线程 ID                                     |
| OpType                 | string   | 操作类型                                            |
| Name                   | string   | 操作对象名                                          |
| QueryTimeSpent         | double   | 查询总共花费时间，单位为毫秒                        |
| DispatchTimeSpent      | double   | 消息分发花费时间，单位为毫秒                        |
| MsgSentTime            | double   | 消息发送花费时间，单位为毫秒                        |
| ReplyTimeSpent         | double   | 查询应答的网络耗时，单位为毫秒                      |
| QueryCataTime          | double   | 查询编目耗时，单位为毫秒                            |
| RemoteNodeWaitTime     | double   | 等待远程节点应答所花费时间，单位为毫秒              |
| BlockTime              | double   | 操作被阻塞的时间，如 SyncControl 等，详细阻塞事件在 BlockType 中 |
| SortTime               | double   | 数据排序花费时间，单位为毫秒                        |
| TotalMsgSent           | int32    | 发送到远程节点(不包含编目节点)的消息总次数          |
| TotalReplyCount        | int32    | 发送到源节点的消息应答总次数                        |
| QueryCataCount         | int32    | 查询编目的次数                                      |
| ReturnNum              | int32    | 操作返回记录数                                      |
| QueryID                | string   | 查询ID, 用于关联协调节点和数据节点的慢查询信息      |
| RelatedNID             | int32    | 查询接入节点ID，可用于查询和会话关联                |
| RelatedTID             | int32    | 查询接入节点的线程ID，可用于查询和会话关联          |
| SessionID              | int32    | 查询所属会话ID                                      |
| ClientInfo             | bson     | 连接到 SequoiaDB 引擎执行该查询的客户端信息         |
| RelatedNode            | bson array | 处理该查询时，经该协调节点发送到的远程节点集(如果为空则不显示该字段)   |
| BlockType              | bson array | 阻塞事件类型(如果为空则不显示该字段)：FreezingWindow, DMSBlock, WaitPrimary, WaitTransRollback, WaitReelect, SyncControl, WaitFusing, NoLogSpace   |
| LastOpInfo             | string   | 查询语句内容                                        |

**ClientInfo 字段中信息**

| 字段名                 | 类型     | 描述                                                               |
| ---------------------- | -------- | ------------------------------------------------------------------ |
| ClientTID              | int32    | 所连接的协调节点客户端线程 ID                                      |
| ClientHost             | int32    | 所连接的协调节点客户端主机 IP                                      |
| ClientPort             | int32    | 所连接的协调节点客户端主机端口，仅在连接客户端为 SQL 引擎时显示    |
| ClientQID              | int32    | 所连接的协调节点客户端程序查询 ID，仅在连接客户端为 SQL 引擎时显示 |

## 数据节点字段信息

| 字段名                 | 类型     | 描述                                                               |
| ---------------------- | -------- | ------------------------------------------------------------------ |
| NodeName               | string   | 节点名，格式为\<hostname\>:\<servicename\>                         |
| NodeID                 | bson array | 节点的 ID，格式为[<分区组 ID>,<节点 ID>]                         |
| StartTimestamp         | string   | 查询开始时间                                                       |
| EndTimestamp           | string   | 查询结束时间                                                       |
| TID                    | int32    | 查询所属线程 ID                                                    |
| OpType                 | string   | 操作类型                                                           |
| Name                   | string   | 操作对象名                                                         |
| QueryTimeSpent         | double   | 查询总共花费时间，单位为毫秒                                       |
| DispatchTimeSpent      | double   | 消息分发花费时间，单位为毫秒                                       |
| MsgSentTime            | double   | 消息发送花费时间，单位为毫秒                                       |
| ReplyTimeSpent         | double   | 查询应答的网络耗时，单位为毫秒                                     |
| QueryCataTime          | double   | 查询编目耗时，单位为毫秒                                           |
| RemoteNodeWaitTime     | double   | 等待远程节点应答所花费时间，单位为毫秒                             |
| BlockTime              | double   | 操作被阻塞的时间，如 SyncControl 等，详细阻塞事件在 BlockType 中   |
| SortTime               | double   | 数据排序花费时间，单位为毫秒                                       |
| TotalMsgSent           | int32    | 发送到远程节点(不包含编目节点)的消息总次数                         |
| TotalReplyCount        | int32    | 发送到源节点的消息应答总次数                                       |
| QueryCataCount         | int32    | 查询编目的次数                                                     |
| ReturnNum              | int32    | 操作返回记录数                                                     |
| QueryID                | string   | 查询ID, 用于关联协调节点和数据节点的慢查询信息                     |
| RelatedNID             | int32    | 查询接入节点ID，可用于查询和会话关联                               |
| RelatedTID             | int32    | 查询接入节点的线程ID，可用于查询和会话关联                         |
| SessionID              | int32    | 查询所属会话ID                                                     |
| AccessPlanID           | int32    | 查询对应的访问计划 ID                                              |
| HashCode               | int32    | 查询语句的哈希标识，相同哈希标识对应同类型的查询语句               |
| DataRead               | int32    | 数据记录读                                                         |
| DataWrite              | int32    | 数据记录写                                                         |
| IndexRead              | int32    | 索引读                                                             |
| IndexWrite             | int32    | 索引写                                                             |
| LobRead                | int32    | 服务端中 LOB 分片的读次数                                          |
| LobWrite               | int32    | 服务端中 LOB 分片的写次数                                          |
| LobTruncate            | int64    | 服务端中 LOB 分片的截断次数（仅在 v3.6.1 及以上版本生效）          |
| LobAddressing          | int64    | 服务端中 LOB 分片的寻址总次数（仅在 v3.6.1 及以上版本生效）        |
| TransLockWaitTime      | double   | 锁等待时间，单位为毫秒                                             |
| LatchWaitTime          | double   | 闩锁等待时间，单位为毫秒                                           |
| SyncWaitTime           | double   | 等待备节点数据同步所花费时间，单位为毫秒                           |
| FileOPTime             | double   | 节点在文件层操作的耗时，单位为毫秒                                 |
| LogOPTime              | double   | 读写同步日志的耗时，单位为毫秒                                     |
| TransLockWaitCount     | int32    | 锁等待次数                                                         |
| LatchWaitCount         | int32    | 闩锁等待次数                                                       |
| RelatedNode            | bson array | 本节点操作执行过程中发送消息到的远程节点(如果为空则不显示该字段) |
| BlockType              | bson array | 阻塞事件类型(如果为空则不显示该字段)：FreezingWindow, DMSBlock, WaitPrimary, WaitTransRollback, WaitReelect, SyncControl, WaitFusing, NoLogSpace   |
| LastOpInfo             | string   | 查询语句内容                                                       |

## 示例

- 查看协调节点的查询信息

    ```lang-javascript
    > db.snapshot(SDB_SNAP_QUERIES)
    ```

    输出结果如下：

    ```lang-json
   {
     "NodeName": "sdbserver:11810",
     "NodeID": [
       2,
       4
     ],
     "StartTimestamp": "2024-03-19-20.53.23.574220",
     "EndTimestamp": "--",
     "TID": 52917,
     "OpType": "QUERY",
     "Name": "foo.bar",
     "QueryTimeSpent": 2.839,
     "DispatchTimeSpent": 0,
     "MsgSentTime": 0.167,
     "ReplyTimeSpent": 0.157,
     "QueryCataTime": 0,
     "RemoteNodeWaitTime": 2.197,
     "BlockTime": 0,
     "SortTime": 0,
     "TotalMsgSent": 5,
     "TotalReplyCount": 1,
     "QueryCataCount": 0,
     "ReturnNum": 1600,
     "QueryID": "0x0000ceb500049feb00000008",
     "RelatedNID": 4,
     "RelatedTID": 52917,
     "SessionID": 7352,
     "ClientInfo": {
       "ClientTID": 52911,
       "ClientHost": "192.168.30.64"
     },
     "RelatedNode": [
       1003
     ],
     "LastOpInfo": "Collection:foo.bar, Matcher:{ \"a\": { \"$type\": 2, \"$et\": \"double\" } }, Selector:{}, OrderBy:{ \"_id\": 1 }, Hint:{}, Skip:0, Limit:-1, Flag:0x00004200(16896)"
   }
    ```


- 查看数据节点的查询信息

    ```lang-javascript
    > var data = new Sdb("sdbserver", 11820)
    > data.snapshot(SDB_SNAP_QUERIES)
    ```

    输出结果下：

    ```lang-json
   {
     "NodeName": "sdbserver:11820",
     "NodeID": [
       1002,
       1003
     ],
     "StartTimestamp": "2024-03-19-20.57.51.001284",
     "EndTimestamp": "--",
     "TID": 57980,
     "OpType": "QUERY",
     "Name": "foo.bar",
     "QueryTimeSpent": 4.027,
     "DispatchTimeSpent": 0.102,
     "MsgSentTime": 0,
     "ReplyTimeSpent": 0.077,
     "QueryCataTime": 0,
     "RemoteNodeWaitTime": 0,
     "BlockTime": 0,
     "SortTime": 0,
     "TotalMsgSent": 0,
     "TotalReplyCount": 1,
     "ReturnNum": 100,
     "QueryID": "0x00010176000452c700000071",
     "RelatedNID": 4,
     "RelatedTID": 65910,
     "SessionID": 1444,
     "AccessPlanID": 2491,
     "HashCode": 6179654,
     "DataRead": 100,
     "DataWrite": 0,
     "IndexRead": 0,
     "IndexWrite": 0,
     "LobRead": 0,
     "LobWrite": 0,
     "LobTruncate": 0,
     "LobAddressing": 0,
     "TransLockWaitTime": 0,
     "LatchWaitTime": 0,
     "SyncWaitTime": 0,
     "FileOPTime": 3.01,
     "LogOPTime": 0,
     "TransLockWaitCount": 0,
     "LatchWaitCount": 0,
     "LastOpInfo": "Collection:foo.bar, Matcher:{ \"a\": { \"$type\": 2, \"$et\": \"double\" } }, Selector:{}, OrderBy:{ \"_id\": 1 }, Hint:{}, Skip:0, Limit:-1, Flag:0x00004200(16896)"
   }
    ```

- 查看历史查询记录

    ```lang-javascript
    > db.snapshot(SDB_SNAP_QUERIES, new SdbSnapshotOption().options({"viewHistory":true}))
    ```

    输出结果如下：

    ```lang-json
   {
     "NodeName": "sdbserver:11810",
     "NodeID": [
       2,
       4
     ],
     "StartTimestamp": "2024-03-19-21.05.04.882779",
     "EndTimestamp": "2024-03-19-21.05.05.981709",
     "TID": 70878,
     "OpType": "QUERY",
     "Name": "foo.bar",
     "QueryTimeSpent": 1061.201,
     "DispatchTimeSpent": 0,
     "MsgSentTime": 0.981,
     "ReplyTimeSpent": 0.393,
     "QueryCataTime": 0,
     "RemoteNodeWaitTime": 1058.26,
     "BlockTime": 0,
     "SortTime": 0,
     "TotalMsgSent": 22,
     "TotalReplyCount": 2,
     "QueryCataCount": 0,
     "ReturnNum": 21,
     "QueryID": "0x000114de000486fd00000005",
     "RelatedNID": 4,
     "RelatedTID": 70878,
     "SessionID": 9448,
     "ClientInfo": {
       "ClientTID": 70777,
       "ClientHost": "192.168.30.64"
     },
     "RelatedNode": [
       3,
       1003,
       1005,
       1008
     ],
     "LastOpInfo": "Collection:foo.bar, Matcher:{ \"a\": 10000 }, Selector:{}, OrderBy:{}, Hint:{}, Skip:0, Limit:-1, Flag:0x00004200(16896)"
   }
    ```


[^_^]:
    本文使用的所有引用及链接
[SnapshotOption]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbSnapshotOption.md
[configuration]:manual/Distributed_Engine/Maintainance/Database_Configuration/parameter_instructions.md
