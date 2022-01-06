
## 描述

数据库快照 $SNAPSHOT_DB 列出当前数据库节点中主要的状态与性能监控参数。

通过协调节点查询快照，将返回所有节点的快照信息，每个数据节点或编目节点产生一条记录；通过数据节点或编目节点查询快照，将返回当前节点的快照信息。

## 标识

$SNAPSHOT_DB

## 字段信息

| 字段名                | 类型   | 描述                                                                            |
| --------------------- | ------ | ------------------------------------------------------------------------------- |
| NodeName              | string | 节点名，格式为< HostName >:< ServiceName >                                      |
| HostName              | string | 数据库节点所在物理节点的主机名                                                  |
| ServiceName           | string | svcname 所指定的服务名，与 HostName 共同作为一个逻辑节点的标识                  |
| GroupName             | string | 该逻辑节点所属的分区组名，standalone 模式下该字段为空字符串                     |
| IsPrimary             | boolean| 该节点是否为主节点，standalone 模式下该字段为 false                             |
| ServiceStatus         | boolean| 是否为可提供服务状态。<br>一些特殊状态，例如[全量同步][architecture]会使该状态为 false |
| Status                | string | 节点状态，取值如下：<br/>             "Normal"：正常工作状态<br/>             "Shutdown"：正在关闭状态，表示节点正在被关闭<br/>             "Rebuilding"：重新构建状态，如节点异常重启后，无法与其他节点进行数据同步时，节点会进入该状态，重新构建数据<br/>             "FullSync"：[全量同步][architecture]状态<br/>             "OfflineBackup"：[数据备份][regular_bar]状态  |
| FTStatus              | string | 容错状态，取值如下：<br> "NOSPC"：磁盘空间不足 <br>"DEADSYNC"：节点数据不同步 <br> "SLOWNODE"：节点数据同步过慢 <br> "TRANSERR"：节点事务异常 |
| BeginLSN.Offset       | int64  | 起始 LSN 的偏移                                                                 |
| BeginLSN.Version      | int32  | 起始 LSN 的版本号                                                               |
| CurrentLSN.Offset     | int64  | 当前 LSN 的偏移                                                                 |
| CurrentLSN.Version    | int32  | 当前 LSN 的版本号                                                               |
| CommittedLSN.Offset   | int64  | 已提交 LSN 的偏移                                                               |
| CommittedLSN.Version  | int32  | 已提交 LSN 的版本号                                                             |
| CompleteLSN           | int64  | 已完成 LSN 的偏移                                                               |
| LSNQueSize            | int32  | 等待同步的 LSN 队列长度                                                           |
| TransInfo.TotalCount  | int32  | 正在执行的事务数量                                                              |
| TransInfo.BeginLSN    | int64  | 正在执行的事务的起始 LSN 的偏移                                                 |
| NodeID                | array  | 节点的 ID，格式为[ <分区组 ID>, <节点 ID> ]<br>在 standalone 模式下，该字段为[ 0，0 ] |
| Version.Major         | int32  | 数据库主版本号                                                                  |
| Version.Minor         | int32  | 数据库子版本号                                                                  |
| Version.Fix           | int32  | 数据库修复版本号                                                                |
| Version.Release       | int32  | 数据库内部版本号                                                                |
| Version.GitVersion    | string | 数据库发行版本号                                                                |
| Version.Build         | string | 数据库编译时间                                                                  |
| Editon                | string | “Enterprise”表示企业版（社区版中无该字段）                                |
| CurrentActiveSessions | int32  | 当前活动会话                                                                |
| CurrentIdleSessions   | int32  | 当前非活动会话，一般来说非活动会话意味着 EDU 存在线程池中等待分配               |
| CurrentSystemSessions | int32  | 当前系统会话，为当前活动用户 EDU 数量 |
| CurrentTaskSessions   | int32  | 后台任务会话数量                                                                |
| CurrentContexts       | int32  | 当前上下文数量                                                                  |
| ReceivedEvents        | int32  | 当前分区接收到的事件请求总数                                                    |
| Role                  | string | 当前节点角色                                                                    |
| Disk.DatabasePath     | string | 数据库所在路径                                                                  |
| Disk.LoadPercent      | int32  | 数据库路径磁盘占用率百分比                                                      |
| Disk.TotalSpace       | int64  | 数据库路径总空间，单位为字节                                                  |
| Disk.FreeSpace        | int64  | 数据库路径空闲空间，单位为字节                                                |
| TotalNumConnects      | int32  | 数据库连接请求数量                                                              |
| TotalDataRead         | int64  | 总数据读请求                                                                    |
| TotalIndexRead        | int64  | 总索引读请求                                                                    |
| TotalDataWrite        | int64  | 总数据写请求                                                                    |
| TotalIndexWrite       | int64  | 总索引写请求                                                                    |
| TotalUpdate           | int64  | 总更新记录数量                                                                  |
| TotalDelete           | int64  | 总删除记录数量                                                                  |
| TotalInsert           | int64  | 总插入记录数量                                                                  |
| ReplUpdate            | int64  | 复制更新记录数量                                                                |
| ReplDelete            | int64  | 复制删除记录数量                                                                |
| ReplInsert            | int64  | 复制插入记录数量                                                                |
| TotalSelect           | int64  | 总选择记录数量                                                                  |
| TotalRead             | int64  | 总读取记录数量                                                                  |
| TotalReadTime         | int64  | 总读取时间，单位为毫秒                                                        |
| TotalWriteTime        | int64  | 总写入时间，单位为毫秒                                                        |
| ActivateTimestamp     | timestamp | 数据库节点启动时间                                                         |
| ResetTimestamp        | timestamp | 重置快照的时间                               |
| UserCPU               | double | 用户 CPU，单位为秒                                                            |
| SysCPU                | double | 系统 CPU，单位为秒                                                            |
| freeLogSpace          | int64  | 空闲日志空间，单位为字节                                                      |
| vsize                 | int64  | 虚拟内存使用量，单位为字节                                                    |
| rss                   | int64  | 物理内存使用量，单位为字节                                                    |
| fault                 | int64  | 每秒访问失败数（仅支持 Linux），数据被交换出物理内存，放到 swap                 |
| TotalMapped           | int64  | mmap 的总数据量，单位为字节                                                   |
| svcNetIn              | int64  | 本地服务端口收到的网络流量，单位为字节                                        |
| svcNetOut             | int64  | 本地服务端口发送的网络流量，单位为字节                                        |
| shardNetIn            | int64  | shard 平面端口收到的网络流量，单位为字节                                       |
| shardNetOut           | int64  | shard 平面端口发送的网络流量，单位为字节                                       |
| replNetIn             | int64  | 数据同步平面端口收到的网络流量，单位为字节                                     |
| replNetOut            | int64  | 数据同步平面端口发送的网络流量，单位为字节                                     |
| SchdlrType            | int32  | 资源调度类型，取值如下：<br>0：没有开启资源调度<br>1：开启了 FIFO资 源调度 <br>2：开启了优先级资源调度<br>3：开启了基于容器的优先级资源调度                                     |
| SchdlrTypeDesp        | string | 资源调度类型描述，取值：NONE、FIFO、PRIORITY、CONTAINER                  |
| Run                   | int32  | 当前正在运行的任务数量                                                          |
| Wait                  | int32  | 当前处于等待队列的任务数量（包含未分发的任务）                                  |
| SchdlrMgrEvtNum       | int32  | 当前未分发的任务数量                                                            |
| SchdlrTimes           | int64  | 统计时间范围内总的任务执行次数                                                  |
| MemPoolSize           | int64  | Pool Memory 的大小，单位为字节                                                |

## 示例

查看数据库快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_DB" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:41000",
  "HostName": "hostname",
  "ServiceName": "41000",
  "GroupName": "db2",
  "IsPrimary": false,
  "ServiceStatus": true,
  "Status": "Normal",
  "FTStatus": "",
  "BeginLSN": {
    "Offset": 2013265920,
    "Version": 1
  },
  "CurrentLSN": {
    "Offset": 3314225876,
    "Version": 1
  },
  "CommittedLSN": {
    "Offset": 3314225876,
    "Version": 1
  },
  "CompleteLSN": 3314226020,
  "LSNQueSize": 0,
  "TransInfo": {
    "TotalCount": 1,
    "BeginLSN": 3314225744
  },
  "NodeID": [
    1001,
    1002
  ],
  "Version": {
    "Major": 3,
    "Minor": 2,
    "Fix": 1,
    "Release": 41325,
    "GitVersion": "7b21adc4206894102682a621a4b49f17ed96a46f",
    "Build": "2019-05-30-15.48.53(Debug)"
  },
  "CurrentActiveSessions": 19,
  "CurrentIdleSessions": 12,
  "CurrentSystemSessions": 12,
  "CurrentTaskSessions": 5,
  "CurrentContexts": 1,
  "ReceivedEvents": 1122566,
  "Role": "data",
  "Disk": {
    "DatabasePath": "/opt/test/41000/",
    "LoadPercent": 34,
    "TotalSpace": 211139878912,
    "FreeSpace": 138432405504
  },
  "TotalNumConnects": 0,
  "TotalDataRead": 182083,
  "TotalIndexRead": 445888,
  "TotalDataWrite": 611764,
  "TotalIndexWrite": 943444,
  "TotalUpdate": 14502,
  "TotalDelete": 95970,
  "TotalInsert": 501098,
  "ReplUpdate": 14502,
  "ReplDelete": 95968,
  "ReplInsert": 501098,
  "TotalSelect": 2164,
  "TotalRead": 181154,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "ActivateTimestamp": "2019-05-31-09.38.06.394521",
  "ResetTimestamp": "2019-05-31-09.38.06.394521",
  "UserCPU": "274.790000",
  "SysCPU": "430.920000",
  "freeLogSpace": 956610128,
  "vsize": 2412945408,
  "rss": 88946,
  "fault": 116,
  "TotalMapped": 956825600,
  "svcNetIn": 1054928,
  "svcNetOut": 6801875,
  "shardNetIn": 3580367,
  "shardNetOut": 2284882,
  "replNetIn": 3413945536,
  "replNetOut": 99794796,
  "SchdlrType": 0,
  "SchdlrTypeDesp": "NONE",
  "Run": 1,
  "Wait": 0,
  "SchdlrMgrEvtNum": 0,
  "SchdlrTimes": 0
  "MemPoolSize": 56868864
}
...
```



[^_^]:
    本文使用的所有引用及链接
[architecture]:manual/Distributed_Engine/Architecture/Replication/architecture.md#全量同步
[regular_bar]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_backup.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md