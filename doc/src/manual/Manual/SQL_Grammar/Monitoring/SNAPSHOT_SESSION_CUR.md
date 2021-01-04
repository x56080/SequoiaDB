
##描述##

当前会话快照 $SNAPSHOT_SESSIONS_CUR 列出数据库节点中的当前用户会话记录。

如果当前连接在协调节点上，将会返回当前会话通过协调节点连接各个数据节点或者编目节点的会话，每个数据节点或者编目节点连接产生一条记录；如果当前连接在数据节点或者编目节点上，将会返回一条记录。

##标识##

$SNAPSHOT_SESSIONS_CUR

##字段信息##
| 字段名            | 类型          | 描述                                               |
| ----------------- | ------------- | -------------------------------------------------- |
| NodeName          | string        | 节点名，格式为<主机名>:<端口号>                    |
| SessionID         | int64         | 会话 ID                                            |
| TID               | int32         | 该会话所对应的系统线程 ID                          |
| Status            | string        | 会话状态，取值如下：<br> Creating：创建状态<br> Running：运行状态<br> Waiting：等待状态<br> Idle：线程池待机状态<br> Destroying：销毁状态 |
| IsBlocked         | boolean       | 会话当前是否处理阻塞状态                           |
| Type              | string        | [EDU 类型][edu] |
| Name              | string        | EDU 名，一般系统 EDU 名为空                        |
| Doing             | string        | 会话当前阻塞状态的详细描述信息                     |
| Source            | string        | 会话来源信息，该字段仅在与 SQL 实例相关的会话中有值 |
| QueueSize         | int32         | 等待处理请求的队列长度                             |
| ProcessEventCount | int64         | 已经处理请求的数量                                 |
| RelatedID         | string        | 会话的内部标识                                     |
| Contexts          | array         | 上下文 ID 数组，为该会话所包含的所有上下文列表     |
| TotalDataRead     | int64         | 数据记录读                                         |
| TotalIndexRead    | int64         | 索引读                                             |
| TotalDataWrite    | int64         | 数据记录写                                         |
| TotalIndexWrite   | int64         | 索引写                                             |
| TotalUpdate       | int64         | 总更新记录数量                                     |
| TotalDelete       | int64         | 总删除记录数量                                     |
| TotalInsert       | int64         | 总插入记录数量                                     |
| TotalSelect       | int64         | 总选取记录数量                                     |
| TotalRead         | int64         | 总数据读                                           |
| TotalReadTime     | int64         | 总数据读时间，单位为毫秒                           |
| TotalWriteTime    | int64         | 总数据写时间，单位为毫秒                           |
| ReadTimeSpent     | int64         | 读取记录的时间，单位为毫秒                         |
| WriteTimeSpent    | int64         | 写入记录的时间，单位为毫秒                         |
| ConnectTimestamp  | timestamp     | 连接发起时间                                       |
| ResetTimestamp    | timestamp     | 重置快照的时间                                     |
| LastOpType        | string        | 最后一次操作的类型，如 insert、update              |
| LastOpBegin       | string        | 最后一次操作的起始时间                             |
| LastOpEnd         | string        | 最后一次操作的结束时间                             |
| LastOpInfo        | string        | 最后一次操作的详细信息                             |
| UserCPU           | double        | 用户 CPU，单位为秒                                 |
| SysCPU            | double        | 系统 CPU，单位为秒                                 |

##示例##

查看当前会话快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_SESSION_CUR" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:41000",
  "SessionID": 28,
  "TID": 23512,
  "Status": "Running",
  "IsBlocked": false,
  "Type": "ShardAgent",
  "Name": "Type:Shard,NetID:1,R-TID:24371,R-IP:192.168.20.62,R-Port:50000",
  "Doing": "",
  "Source": "",
  "QueueSize": 0,
  "ProcessEventCount": 27,
  "RelatedID": "c0a8143ec35000005f33",
  "Contexts": [
    13579
  ],
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "TotalSelect": 0,
  "TotalRead": 0,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "ReadTimeSpent": 60,
  "WriteTimeSpent": 0,
  "ConnectTimestamp": "2019-06-03-14.36.25.093610",
  "ResetTimestamp": "2019-06-03-14.36.25.093609",
  "LastOpType": "COMMAND",
  "LastOpBegin": "2019-06-03-14.50.01.460452",
  "LastOpEnd": "--",
  "LastOpInfo": "Command:$SNAPSHOT_SESSION_CUR, Collection:, Match:{}, Selector:{}, OrderBy:{}, Hint:{}, Skip:0, Limit:-1, Flag:0x00000200(512)",
  "UserCPU": 9.24,
  "SysCPU": 2
}
```


[^_^]:
    本文使用的所有引用及链接
[edu]:manual/Distributed_Engine/Architecture/Thread_Model/edu.md
