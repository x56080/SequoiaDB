##描述##

查询快照可以列出数据库中正在进行的查询信息。当 [mongroupmask](database_management/database_configuration/configuration_parameters.md) 参数设置为“slowQuery:detail”或“all:detail”时，查询耗时超过 [monslowquerythreshold](database_management/database_configuration/configuration_parameters.md) 参数所规定阈值的历史查询信息会被缓存。用户可以通过指定 [viewHistory](reference/Sequoiadb_command/AuxiliaryObjects/SdbSnapshotOption.md) 选项，查看历史查询信息。

>**Note:**
>
> 每一个数据节点上正在进行的每一个查询操作为一条记录。


##标示##

SDB_SNAP_QUERIES

###字段信息###

协调节点

| 字段名                 | 类型     | 描述                                            |
| ---------------------- | -------- | ------------------------------------------------|
| NodeID                 | BSON数组 | 节点的 ID，为“[ <分区组 ID>, <节点 ID> ]”       |
| StartTimestamp         | 字符串   | 查询开始时间                                    |
| EndTimestamp           | 字符串   | 查询结束时间                                    |
| TID                    | 整型     | 内部线程 ID                                     |
| OpType                 | 字符串   | 操作类型                                        |
| Name                   | 字符串   | 操作对象名                                      |
| QueryTimeSpent         | 整型     | 查询总共花费时间，单位：毫秒                    |
| ReturnNum              | 整型     | 返回值                                          |
| TotalMsgSent           | 整型     | 发送到远程节点的消息总数                        |
| LastOpInfo             | 字符串   | 查询语句内容                                    |
| MsgSentTime            | 整型     | 消息发送花费时间，单位：毫秒                    |
| RemoteNodeWaitTime     | 整型     | 等待远程节点花费时间，单位：毫秒                |
| RelatedNode            | BSON数组 | 处理该查询时经由该协调节点发送到的远程数据节点集|

数据节点

| 字段名                 | 类型     | 描述                                                                                     |
| ---------------------- | -------- | ---------------------------------------------------------------------------------------- |
| NodeID                 | BSON数组 | 节点的 ID，为“[ <分区组 ID>, <节点 ID> ]”                                                |
| StartTimestamp         | 字符串   | 查询开始时间                                                                             |
| EndTimestamp           | 字符串   | 查询结束时间                                                                             |
| TID                    | 整型     | 内部线程 ID                                                                              |
| OpType                 | 字符串   | 操作类型                                                                                 |
| Name                   | 字符串   | 操作对象名                                                                               |
| QueryTimeSpent         | 整型     | 查询总共花费时间，单位：毫秒                                                             |
| ReturnNum              | 整型     | 返回值                                                                                   |
| RelatedNID             | 整型     | 将该查询请求发送到该数据节点的的相关协调节点ID                                           |
| RelatedTID             | 整型     | 发送查询的相关协调节点的线程ID, 结合RelatedNID可以将协调节点和数据节点的快照输出联系起来 |
| SessionID              | 整型     | 内部会话ID                                                                               |
| AccessPlanID           | 整型     | 访问计划 ID                                                                              |
| DataRead               | 整型     | 数据记录读                                                                               |
| DataWrite              | 整型     | 数据记录写                                                                               |
| IndexRead              | 整型     | 索引读                                                                                   |
| IndexWrite             | 整型     | 索引写                                                                                   |
| LobRead                | 整型     | 大对象数据读                                                                             |
| LobWrite               | 整型     | 大对象数据写                                                                             |
| TransLockWaitTime      | 整型     | 锁等待时间，单位：毫秒                                                                   |
| LatchWaitTime          | 整型     | 闩锁等待时间，单位：毫秒                                                                 |


##示例##

- 查看协调节点的查询信息
   ```lang-javascript
   > db.snapshot(SDB_SNAP_QUERIES)
   ```

   输出结果如下：

   ```lang-json

   {
     "NodeID": [
       2,
       4
     ],
     "StartTimestamp": "2020-06-12-11.33.14.019931",
     "EndTimestamp": "1970-01-01-08.00.00.000000",
     "TID": 10832,
     "OpType": "QUERY",
     "Name": "sbtest1.sbtest2",
     "QueryTimeSpent": 0,
     "ReturnNum": 0,
     "TotalMsgSent": 1,
     "LastOpInfo": "Collection:sbtest1.sbtest2, Matcher:{ \"id\": { \"$et\": 5015 } }, Selector:{}, OrderBy:{ \"id\": 1 }, Hint:{ \"\": \"PRIMARY\" }, Skip:0, Limit:-1, Flag:0x00000200(512)",
     "MsgSentTime": 0.034,
     "RemoteNodeWaitTime": 0,
     "RelatedNode": [
       1002
     ]
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
     "NodeID": [
       1000,
       1002
     ],
     "StartTimestamp": "2020-06-12-11.29.44.906939",
     "EndTimestamp": "1970-01-01-08.00.00.000000",
     "TID": 10850,
     "OpType": "QUERY",
     "Name": "$snapshot queries",
     "QueryTimeSpent": 0.118,
     "ReturnNum": 0,
     "RelatedNID": 0,
     "RelatedTID": 0,
     "SessionID": 47,
     "AccessPlanID": -1,
     "DataRead": 0,
     "DataWrite": 0,
     "IndexRead": 0,
     "IndexWrite": 0,
     "LobRead": 0,
     "LobWrite": 0,
     "TransLockWaitTime": 0,
     "LatchWaitTime": 0
   }
   ```

- 查看历史查询记录

   ```lang-javascript
   > db.snapshot(SDB_SNAP_QUERIES, new SdbSnapshotOption().options({"viewHistory":true}))
   ```

   输出结果如下：

   ```lang-json
   {
     "NodeID": [
       2,
       4
     ],
     "StartTimestamp": "2020-06-12-11.02.27.429347",
     "EndTimestamp": "1970-01-01-08.00.00.000000",
     "TID": 10107,
     "OpType": "QUERY",
     "Name": "sbtest1.sbtest6",
     "QueryTimeSpent": 0,
     "ReturnNum": 0,
     "TotalMsgSent": 1,
     "LastOpInfo": "Collection:sbtest1.sbtest6, Matcher:{ \"id\": { \"$et\": 5014 } }, Selector:{}, OrderBy:{ \"id\": 1 }, Hint:{ \"\": \"PRIMARY\" }, Skip:0, Limit:-1, Flag:0x00000200(512)",
     "MsgSentTime": 0.046,
     "RemoteNodeWaitTime": 0,
     "RelatedNode": [
       1002
     ]
   }
   {
     "NodeID": [
       2,
       4
     ],
     "StartTimestamp": "2020-06-12-11.02.27.515512",
     "EndTimestamp": "1970-01-01-08.00.00.000000",
     "TID": 10830,
     "OpType": "DELETE",
     "Name": "sbtest1.sbtest9",
     "QueryTimeSpent": 0,
     "ReturnNum": 0,
     "TotalMsgSent": 1,
     "LastOpInfo": "Collection:sbtest1.sbtest9, Deletor:{ \"$and\": [ { \"id\": { \"$et\": 5900 } }, { \"id\": { \"$et\": 5900 } } ] }, Hint:{}, Flag:0x00000004(4)",
     "MsgSentTime": 0.029,
     "RemoteNodeWaitTime": 0,
     "RelatedNode": [
       1002
     ]
   }
   ```

