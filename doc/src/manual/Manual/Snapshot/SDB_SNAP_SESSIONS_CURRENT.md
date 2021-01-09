[^_^]: 

    当前会话快照
    作者：何嘉文
    时间：20190307
    评审意见

    王涛：
    许建辉：
    市场部：



当前会话快照可以列出当前用户的会话。

> **Note:**  
>
> 如果连接协调节点查询快照，返回的是当前会话通过协调节点所连接的编目节点和数据节点的会话。

标识
----

SDB_SNAP_SESSIONS_CURRENT

字段信息
----

| 字段名            | 类型       | 描述                                               |
| ----------------- | ---------- | -------------------------------------------------- |
| NodeName          | string     | 节点名，格式为<主机名>:<服务名>                    |
| SessionID         | int64      | 会话 ID                                            |
| TID               | int32      | 该会话所对应的系统线程 ID                          |
| Status            | string     | 会话状态，取值如下：<br>"Creating"：创建状态 <br>"Running"：运行状态 <br>"Waiting"：等待状态  <br>"Idle"：线程池待机状态 <br>"Destroying"：销毁状态 |
| Type              | string     | [EDU 类型][edu_url]                                |
| IsBlocked         | boolean    | 会话当前是否处理阻塞状态                           |
| Name              | string     | EDU 名，一般系统 EDU 名为空                                             |
| Doing             | string     | 会话当前阻塞状态的详细描述信息                     |
| Source            | string     | 会话来源信息，该字段仅在与 SQL 实例相关的会话中有值
| QueueSize         | int32      | 等待处理请求的队列长度                             |
| ProcessEventCount | int64      | 已经处理请求的数量                                 |
| MemPoolSize       | 长整型        | Pool Memory 的大小，单位为字节                   |
| RelatedID         | string     | 会话的内部标识                                     |
| Contexts          | bson array | 该会话所有上下文的 ID                              |
| TotalDataRead     | int64      | 数据记录读                                         |
| TotalIndexRead    | int64      | 索引读                                             |
| TotalDataWrite    | int64      | 数据记录写                                         |
| TotalIndexWrite   | int64      | 索引写                                             |
| TotalUpdate       | int64      | 总更新记录数量                                     |
| TotalDelete       | int64      | 总删除记录数量                                     |
| TotalInsert       | int64      | 总插入记录数量                                     |
| TotalSelect       | int64      | 总选取记录数量                                     |
| TotalRead         | int64      | 总数据读                                           |
| TotalReadTime     | int64      | 总数据读时间，单位为毫秒                           |
| TotalWriteTime    | int64      | 总数据写时间，单位为毫秒                           |
| ReadTimeSpent     | int64      | 读取记录的时间，单位为毫秒                         |
| WriteTimeSpent    | int64      | 写入记录的时间，单位为毫秒                         |
| ConnectTimestamp  | timestamp  | 连接发起时间                                       |
| ResetTimestamp    | timestamp  | 重置快照的时间                                     |
| LastOpType        | string     | 最后一次操作的类型，如：INSERT、UPDATE、COMMAND、GETMORE |
| LastOpBegin       | string     | 最后一次操作的起始时间                             |
| LastOpEnd         | string     | 最后一次操作的结束时间                             |
| LastOpInfo        | string     | 最后一次操作的详细信息                             |
| UserCPU           | double     | 用户 CPU，单位为秒                                 |
| SysCPU            | double     | 系统 CPU，单位为秒                                 |

示例
----

查看当前会话快照

```lang-javascript
> db.snapshot( SDB_SNAP_CONTEXTS_CURRENT )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname1:11820",
  "SessionID": 28,
  "TID": 9430,
  "Status": "Running",
  "IsBlocked": false,
  "Type": "Agent",
  "Name": "127.0.0.1:60309",
  "Doing": "",
  "Source": "",
  "QueueSize": 0,
  "ProcessEventCount": 12,
  "MemPoolSize": 0,
  "RelatedID": "c0a81e442e7200008c8a",
  "Contexts": [
    15
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
  "ReadTimeSpent": 10,
  "WriteTimeSpent": 0,
  "ConnectTimestamp": "2013-09-27-18.06.25.961090",
  "ResetTimestamp": "2013-09-27-18.06.25.961090",
  "LastOpType": "UNKNOWN",
  "LastOpBegin": "2014-08-07-14.25.23.550216",
  "LastOpEnd": "--",
  "LastOpInfo": "",
  "UserCPU": "0.910000",
  "SysCPU": "2.060000"
}
```

[^_^]:
    本文使用到的所有链接及引用。
    
[edu_url]: manual/Distributed_Engine/Architecture/Thread_Model/edu.md