##描述##

数据库快照 $SNAPSHOT_DB 列出当前数据库节点中主要的状态与性能监控参数，输出一条记录。

##标示##

$SNAPSHOT_DB

##非协调节点字段信息##

| 字段名                | 类型   | 描述                                                                            |
| --------------------- | ------ | ------------------------------------------------------------------------------- |
| NodeName              | 字符串 | 节点名，为“< HostName > : < ServiceName >”                                      |
| HostName              | 字符串 | 数据库节点所在物理节点的主机名                                                  |
| ServiceName           | 字符串 | svcname 所指定的服务名，与 HostName 共同作为一个逻辑节点的标示                  |
| GroupName             | 字符串 | 该逻辑节点所属的分区组名，standalone 模式下该字段为空字符串                     |
| IsPrimary             | 布尔   | 该节点是否为主节点，standalone 模式下该字段为 false                             |
| ServiceStatus         | 布尔   | 是否为可提供服务状态。<br>一些特殊状态，例如[全量同步](infrastructure/replication/replicate.md#全量同步)会使该状态为 false |
| Status                | 字符串 | 节点状态：<br/>             1."Normal"：正常工作状态。<br/>             2."Shutdown"：正在关闭状态，表示节点正在被关闭。<br/>             3."Rebuilding"：重新构建状态，如节点异常重启后，无法与其他节点进行数据同步时，节点会进入该状态，重新构建数据。<br/>             4."FullSync"：[全量同步](infrastructure/replication/replicate.md#全量同步)状态。<br/>             5."OfflineBackup"：[数据备份](database_management/backup_and_recovery/data_backup.md)状态。  |
| BeginLSN.Offset       | 长整型 | 起始 LSN 的偏移                                                                 |
| BeginLSN.Version      | 整型   | 起始 LSN 的版本号                                                               |
| CurrentLSN.Offset     | 长整型 | 当前 LSN 的偏移                                                                 |
| CurrentLSN.Version    | 整型   | 当前 LSN 的版本号                                                               |
| CommittedLSN.Offset   | 长整型 | 已提交 LSN 的偏移                                                               |
| CommittedLSN.Version  | 整型   | 已提交 LSN 的版本号                                                             |
| CompleteLSN           | 长整型 | 已完成 LSN 的偏移                                                               |
| LSNQueSize            | 整型   | 等待同步的LSN队列长度                                                           |
| TransInfo.TotalCount  | 整型  | 正在执行的事务数量                                                              |
| TransInfo.BeginLSN    | 长整型 | 正在执行的事务的起始 LSN 的偏移                                                 |
| NodeID                | 数组   | 节点的 ID，为“[ <分区组 ID>, <节点 ID> ]”<br>在 standalone 模式下，该字段为“[ 0，0 ]” |
| Version.Major         | 整型   | 数据库主版本号                                                                  |
| Version.Minor         | 整型   | 数据库子版本号                                                                  |
| Version.Fix           | 整型   | 数据库修复版本号                                                                |
| Version.Release       | 整型   | 数据库发行版本号                                                                |
| Version.Build         | 字符串 | 数据库编译时间                                                                  |
| Editon                | 字符串 | “Enterprise”表示企业版（备注：社区版中无该字段）                                |
| CurrentActiveSessions | 整型   | 当前活动会话                                                                |
| CurrentIdleSessions   | 整型   | 当前非活动会话，一般来说非活动会话意味着 EDU 存在线程池中等待分配               |
| CurrentSystemSessions | 整型   | 当前系统会话，为当前活动用户 EDU 数量 |
| CurrentTaskSessions   | 整型   | 后台任务会话数量                                                                |
| CurrentContexts       | 整型   | 当前上下文数量                                                                  |
| ReceivedEvents        | 整型   | 当前分区接收到的事件请求总数                                                    |
| Role                  | 字符串 | 当前节点角色                                                                    |
| Disk.DatabasePath     | 字符串 | 数据库所在路径                                                                  |
| Disk.LoadPercent      | 整型   | 数据库路径磁盘占用率百分比                                                      |
| Disk.TotalSpace       | 长整型 | 数据库路径总空间（单位：字节）                                                  |
| Disk.FreeSpace        | 长整型 | 数据库路径空闲空间（单位：字节）                                                |
| TotalNumConnects      | 整型   | 数据库连接请求数量                                                              |
| TotalDataRead         | 长整型 | 总数据读请求                                                                    |
| TotalIndexRead        | 长整型 | 总索引读请求                                                                    |
| TotalDataWrite        | 长整型 | 总数据写请求                                                                    |
| TotalIndexWrite       | 长整型 | 总索引写请求                                                                    |
| TotalUpdate           | 长整型 | 总更新记录数量                                                                  |
| TotalDelete           | 长整型 | 总删除记录数量                                                                  |
| TotalInsert           | 长整型 | 总插入记录数量                                                                  |
| ReplUpdate            | 长整型 | 复制更新记录数量                                                                |
| ReplDelete            | 长整型 | 复制删除记录数量                                                                |
| ReplInsert            | 长整型 | 复制插入记录数量                                                                |
| TotalSelect           | 长整型 | 总选择记录数量                                                                  |
| TotalRead             | 长整型 | 总读取记录数量                                                                  |
| TotalReadTime         | 长整型 | 总读取时间（单位：毫秒）                                                        |
| TotalWriteTime        | 长整型 | 总写入时间（单位：毫秒）                                                        |
| ActivateTimestamp     | 时间戳 | 数据库节点启动时间                                                              |
| ResetTimestamp        | 时间戳 | 重置快照的时间                               |
| UserCPU               | 浮点数 | 用户 CPU（单位：秒）                                                            |
| SysCPU                | 浮点数 | 系统 CPU（单位：秒）                                                            |
| freeLogSpace          | 长整型 | 空闲日志空间（单位：字节）                                                      |
| vsize                 | 长整型 | 虚拟内存使用量（单位：字节）                                                    |
| rss                   | 长整型 | 物理内存使用量（单位：字节）                                                    |
| fault                 | 长整型 | 每秒访问失败数（仅支持 Linux），数据被交换出物理内存，放到 swap                 |
| TotalMapped           | 长整型 | mmap 的总数据量（单位：字节）                                                   |
| svcNetIn              | 长整型 | 本地服务端口收到的网络流量（单位：字节）                                        |
| svcNetOut             | 长整型 | 本地服务端口发送的网络流量（单位：字节）                                        |
| shardNetIn            | 长整型 | shard 平面端口收到的网络流量（单位：字节）                                      |
| shardNetOut           | 长整型 | shard 平面端口发送的网络流量（单位：字节）                                      |
| replNetIn             | 长整型 | 数据同步平面端口收到的网络流量（单位：字节）                                    |
| replNetOut            | 长整型 | 数据同步平面端口发送的网络流量（单位：字节）                                    |
| SchdlrType            | 整型   | 资源调度类型。0 表示没有开启资源调度，1 表示开启了FIFO资源调度，2 表示开启了优先级资源调度，3 表示开启了基于容器的优先级资源调度                                     |
| SchdlrTypeDesp        | 字符串 | 资源调度类型描述，取值：NONE、FIFO、PRIORITY、CONTAINER                  |
| Run                   | 整型   | 当前正在运行的任务数量                                                          |
| Wait                  | 整型   | 当前处于等待队列的任务数量（包含未分发的任务）                                  |
| SchdlrMgrEvtNum       | 整型   | 当前未分发的任务数量                                                            |
| SchdlrTimes           | 长整型 | 统计时间范围内总的任务执行次数                                                  |

##协调节点字段信息##

| 字段名            | 类型   | 描述                                          |
| ----------------- | ------ | --------------------------------------------- |
| TotalNumConnects  | 整型   | 数据库连接请求数量                            |
| TotalDataRead     | 长整型 | 总数据读请求                                  |
| TotalIndexRead    | 长整型 | 总索引读请求                                  |
| TotalDataWrite    | 长整型 | 总数据写请求                                  |
| TotalIndexWrite   | 长整型 | 总索引写请求                                  |
| TotalUpdate       | 长整型 | 总更新记录数量                                |
| TotalDelete       | 长整型 | 总删除记录数量                                |
| TotalInsert       | 长整型 | 总插入记录数量                                |
| ReplUpdate        | 长整型 | 复制更新记录数量                              |
| ReplDelete        | 长整型 | 复制删除记录数量                              |
| ReplInsert        | 长整型 | 复制插入记录数量                              |
| TotalSelect       | 长整型 | 总选择记录数量                                |
| TotalRead         | 长整型 | 总读取记录数量                                |
| TotalReadTime     | 长整型 | 总读取时间（单位：毫秒）                      |
| TotalWriteTime    | 长整型 | 总写入时间（单位：毫秒）                      |
| freeLogSpace      | 长整型 | 空闲日志空间（单位：字节）                    |
| vsize             | 长整型 | 虚拟内存使用量（单位：字节）                  |
| rss               | 长整型 | 物理内存使用量（单位：字节）                  |
| fault             | 长整型 | 每秒访问失败数（仅支持 Linux），数据被交换出物理内存，放到 swap |
| TotalMapped       | 长整型 | mmap 的总数据量（单位：字节）                 |
| svcNetIn          | 长整型 | 本地服务端口收到的网络流量（单位：字节）      |
| svcNetOut         | 长整型 | 本地服务端口发送的网络流量（单位：字节）      |
| shardNetIn        | 长整型 | shard 平面端口收到的网络流量（单位：字节）    |
| shardNetOut       | 长整型 | shard 平面端口发送的网络流量（单位：字节）    |
| replNetIn         | 长整型 | 数据同步平面端口收到的网络流量（单位：字节）  |
| replNetOut        | 长整型 | 数据同步平面端口发送的网络流量（单位：字节）  |
| ErrNodes.NodeName | 字符串 | 返回异常节点名（主机名 + 端口）               |
| ErrNodes.GroupName| 字符串 | 返回异常节点所属分区组名                      |
| ErrNodes.Flag     | 整型   | 错误码，详细请参见：[错误码](reference/Sequoiadb_error_code.md) |
| ErrNodes.ErrInfo  | 字符串 | 返回节点出错信息                              |

> Note:
>
> 存在异常节点时才显示ErrNodes字段。

##示例##

```lang-javascript
> db.exec( "select * from $SNAPSHOT_DB" )
{
  "NodeName": "hostname:41000",
  "HostName": "hostname",
  "ServiceName": "41000",
  "GroupName": "db2",
  "IsPrimary": false,
  "ServiceStatus": true,
  "Status": "Normal",
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
}
...
```
