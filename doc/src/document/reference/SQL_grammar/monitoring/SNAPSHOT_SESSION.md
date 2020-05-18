##描述##

会话快照 $SNAPSHOT_SESSIONS 列出当前数据库节点中所有的用户与系统会话，每一个会话为一条记录。

##标示##

$SNAPSHOT_SESSIONS

##字段信息##

| 字段名            | 类型          | 描述                                               |
| ----------------- | ------------- | -------------------------------------------------- |
| NodeName          | 字符串        | 节点名（主机名：端口号）                           |
| SessionID         | 长整型        | 会话 ID                                            |
| TID               | 整型          | 该会话所对应的系统线程 ID                          |
| Status            | 字符串        | 会话状态<br>- Creating：创建状态<br>- Running：运行状态<br>- Waiting：等待状态<br>- Idle：线程池待机状态<br>- Destroying：销毁状态 |
| IsBlocked         | 布尔型        | 会话当前是否处理阻塞状态                           |
| Type              | 字符串        | [EDU 类型](database_management/EDU.md)             |
| Name              | 字符串        | EDU 名，一般系统 EDU 名为空                        |
| Doing             | 字符串        | 会话当前阻塞状态的详细描述信息                     |
| QueueSize         | 整型          | 等待处理请求的队列长度                             |
| ProcessEventCount | 长整型        | 已经处理请求的数量                                 |
| RelatedID         | 字符串        | 会话的内部标识                                     |
| Contexts          | 长整型数组    | 上下文 ID 数组，为该会话所包含的所有上下文列表     |
| TotalDataRead     | 长整型        | 数据记录读                                         |
| TotalIndexRead    | 长整型        | 索引读                                             |
| TotalDataWrite    | 长整型        | 数据记录写                                         |
| TotalIndexWrite   | 长整型        | 索引写                                             |
| TotalUpdate       | 长整型        | 总更新记录数量                                     |
| TotalDelete       | 长整型        | 总删除记录数量                                     |
| TotalInsert       | 长整型        | 总插入记录数量                                     |
| TotalSelect       | 长整型        | 总选取记录数量                                     |
| TotalRead         | 长整型        | 总数据读                                           |
| TotalReadTime     | 长整型        | 总数据读时间（单位：毫秒）                         |
| TotalWriteTime    | 长整型        | 总数据写时间（单位：毫秒）                         |
| ReadTimeSpent     | 长整型        | 读取记录的时间（单位：毫秒）                       |
| WriteTimeSpent    | 长整型        | 写入记录的时间（单位：毫秒）                       |
| ConnectTimestamp  | 时间戳        | 连接发起时间                                       |
| ResetTimestamp    | 时间戳        | 重置快照的时间                                     |
| LastOpType        | 字符串        | 最后一次操作的类型，如：insert，update             |
| LastOpBegin       | 字符串        | 最后一次操作的起始时间                             |
| LastOpEnd         | 字符串        | 最后一次操作的结束时间                             |
| LastOpInfo        | 字符串        | 最后一次操作的详细信息                             |
| UserCPU           | 浮点数        | 用户 CPU（单位：秒）                               |
| SysCPU            | 浮点数        | 系统 CPU（单位：秒）                               |

##示例##

```lang-javascript
> db.exec( "select * from $SNAPSHOT_SESSION" )
{
  "NodeName": "hostname:42000",
  "SessionID": 183,
  "TID": 4024,
  "Status": "Idle",
  "IsBlocked": false,
  "Type": "Unknow",
  "Name": "",
  "Doing": "",
  "Source": "",
  "QueueSize": 0,
  "ProcessEventCount": 0,
  "RelatedID": "c0a8143ea41000000fb8",
  "Contexts": [],
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
  "ReadTimeSpent": 0,
  "WriteTimeSpent": 0,
  "ConnectTimestamp": "1970-01-01-08.00.00.000000",
  "ResetTimestamp": "2019-06-03-14.34.57.118778",
  "LastOpType": "UNKNOW",
  "LastOpBegin": "--",
  "LastOpEnd": "--",
  "LastOpInfo": "",
  "UserCPU": 0.01,
  "SysCPU": 0
}
...
```
