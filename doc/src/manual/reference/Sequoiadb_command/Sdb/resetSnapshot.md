##语法##
***db.resetSnapshot( [options] )***

重置快照。主要针对一些统计信息，比如TotalDataRead、TotalDataWrite等。它的作用是清空之前的统计，重新开始统计。

##参数描述##

| 参数名  | 参数类型 | 描述   | 是否必填 |
| ------- | -------- | ------ | -------- |
| options | Json 对象| 设定快照类型、会话号、[命令位置参数](reference/Sequoiadb_command/location.md) | 否 |

1. **options 格式**

 | 属性名 | 描述   | 默认值 | 格式 |
 | ------ | ------ | -------| ---- |
 | Type   | 指定重置的[快照类型](manual/Distributed_Engine/Maintainance/Monitoring/snapshot/snapshot.md)。取值：<br/>"sessions"<br/>"sessions current"<br/>"database"<br/>"health"<br/>"collections"<br/>"all" | "all" | Type: "sessions" |
 | SessionID | 指定重置的会话ID。 | 所有会话 | SessionID: 1 |
 | CollectionSpace | 指定需要重置快照统计信息的集合空间名称，字符串类型。 | 空 | CollectionSpace : "foo" |
 | Collection | 指定需要重置快照统计信息的集合名称，字符串类型。需要为集合全名。 | 空 | Collection : "foo.bar" |
 | Location Elements | [命令位置参数](reference/Sequoiadb_command/location.md) | 所有节点 | GroupName:"db1" |

 > **Note:**
 >
 > * Type: "all" 表示重置所有快照。
 > * SessionID 字段只在 Type: "sessions" 才生效。
 > * CollectionSpace 字段和 Collection 字段只在 Type : "collections" 时生效；CollectionSpace 字段和 Collection 字段不能同时指定；Collection 字段和 CollectionSpace 字段不指定时为清空所有所有集合的快照统计信息。

2. **重置项**

 | 快照类型 | 重置项  |
 | ------ | ------ |
 | [sessions](manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_SESSIONS.md) | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"，"TotalIndexWrite"<br/>"WriteTimeSpent"，"ResetTimestamp"，"LastOpType"，"LastOpBegin"<br/>"TotalRead"，"TotalReadTime"，"TotalWriteTime"，"ReadTimeSpent"<br/>"LastOpEnd"，"LastOpInfo"，"ReadTimeSpent"，"WriteTimeSpent"<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect" |
 | [sessions current](manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_SESSIONS_CURRENT.md) | 与"sessions"重置项相同 |
 | [database](manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_DATABASE.md) | "totalDataRead"，"totalIndexRead"，"totalLobRead"，"TotalDataWrite"<br/>"svcNetOut"，"totalReadTime"，"totalWriteTime"，"resetTimestamp"<br/>"TotalIndexWrite"，"totalLobWrite"，"totalUpdate"，"totalDelete"<br/>"totalInsert"，"totalSelect"，"totalRead"，"receiveNum"<br/>"replUpdate"，"replInsert"，"replDelete"，"svcNetIn" |
 | [health](manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_HEALTH.md) | "ErrNum":{"SDB_OOM"，"SDB_NOSPC"，"SDB_TOO_MANY_OPEN_FD"} |
 | [collections](manual/Distributed_Engine/Maintainance/Monitoring/snapshot/SDB_SNAP_COLLECTIONS.md) | "TotalDataRead"，"TotalIndexRead"，"TotalDataWrite"，"TotalIndexWrite"<br/>"TotalUpdate"，"TotalDelete"，"TotalInsert"，"TotalSelect"<br/>"TotalRead"，"TotalWrite"，"TotalTbScan"，"TotalIxScan"<br/>"ResetTimestamp" |
 | all | 除了重置上述所有的项，还包括：<br/>"totalTime"，"totalContexts" |

##返回值##
无返回值，出错抛异常，并输出错误信息。可以通过 [getLastErrObj()](reference/Sequoiadb_command/Global/getLastErrObj.md)  或 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。

关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##示例##

* 重置 SessionID 为22的快照。

  重置前：

  ```lang-javascript
  >db.snapshot(SDB_SNAP_CONTEXTS,{"SessionID":22})
  {
    "NodeName": "u1604-nzb:31820",
    "SessionID": 22,
    "TID": 11076,
    "Status": "Waiting",
    "Type": "ShardAgent",
    "Name": "Type:Shard,NetID:1,R-TID:12930,R-IP:192.168.20.53,R-Port:11810",
    "QueueSize": 0,
    "ProcessEventCount": 32,
    "RelatedID": "c0a814352e2200003282",
    "Contexts": [
        200
    ],
    "TotalDataRead": 27577,
    "TotalIndexRead": 0,
    "TotalDataWrite": 0,
    "TotalIndexWrite": 0,
    "TotalUpdate": 0,
    "TotalDelete": 0,
    "TotalInsert": 0,
    "TotalSelect": 27577,
    "TotalRead": 27577,
    "TotalReadTime": 0,
    "TotalWriteTime": 0,
    "ReadTimeSpent": 0,
    "WriteTimeSpent": 0,
    "ConnectTimestamp": "2019-06-20-13.55.52.646730",
    "ResetTimestamp": "2019-06-20-13.55.52.646730",
    "LastOpType": "GETMORE",
    "LastOpBegin": "--",
    "LastOpEnd": "2019-06-20-14.20.22.223637",
    "LastOpInfo": "ContextID:200, NumToRead:-1",
    "UserCPU": 0.38,
    "SysCPU": 0.29
  }
  ```
  
  重置快照：

  ```lang-javascript
  > db.resetSnapshot({Type : "session", SessionID: 22})
  Takes 0.001436s.
  ```

  重置后：
  
  ```lang-javascript
  >db.snapshot(SDB_SNAP_CONTEXTS,{"SessionID":22})
  {
    "NodeName": "u1604-nzb:31820",
    "SessionID": 22,
    "TID": 11076,
    "Status": "Waiting",
    "Type": "ShardAgent",
    "Name": "Type:Shard,NetID:1,R-TID:12930,R-IP:192.168.20.53,R-Port:11810",
    "QueueSize": 0,
    "ProcessEventCount": 32,
    "RelatedID": "c0a814352e2200003282",
    "Contexts": [
        200
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
    "ReadTimeSpent": 0,
    "WriteTimeSpent": 0,
    "ConnectTimestamp": "2019-06-20-13.55.52.646730",
    "ResetTimestamp": "2019-06-20-14.23.42.059988",
    "LastOpType": "UNKNOW",
    "LastOpBegin": "--",
    "LastOpEnd": "--",
    "LastOpInfo": "",
    "UserCPU": 0.38,
    "SysCPU": 0.3
  }
  ```
