##描述##

数据库快照 SDB_SNAP_DATABASE 列出当前数据库节点中主要的状态与性能监控参数，输出一条记录。

##标示##

SDB_SNAP_DATABASE

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
| Version.Release       | 整型   | 数据库内部版本号                                                                |
| Version.GitVersion    | 字符串 | 数据库发行版本号                                                                |
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
| MemPoolSize           | 长整型 | Pool Memory 的大小（单位：字节）                                                |

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

##非协调节点示例##

```lang-javascript
> db.snapshot( SDB_SNAP_DATABASE )
{
  "NodeName": "hostname1:11810",
  "HostName": "hostname1",
  "ServiceName": "11810",
  "GroupName": "group1",
  "IsPrimary": false,
  "ServiceStatus": true,
  "Status": "Normal",
  "BeginLSN": {
    "Offset": 0,
    "Version": 1
  },
  "CurrentLSN": {
    "Offset": -1,
    "Version": 0
  },
  "CommittedLSN": {
    "Offset": -1,
    "Version": 0
  },
  "CompleteLSN": -1,
  "LSNQueSize": 0,
  "TransInfo": {
    "TotalCount": 0,
    "BeginLSN": -1
  },
  "NodeID": [
    1000,
    1000
  ],
  "Version": {
    "Major": 1,
    "Minor": 8,
    "Fix": 0,
    "Release": 13971,
    "GitVersion": "7b21adc4206894102682a621a4b49f17ed96a46f",
    "Build": "2014-08-07-11.04.12(Debug)"
  },
  "CurrentActiveSessions": 18,
  "CurrentIdleSessions": 0,
  "CurrentSystemSessions": 5,
  "CurrentContexts": 1,
  "ReceivedEvents": 0,
  "Role": "data",
  "Disk": {
    "DatabasePath": "/home/users/sdbadmin/sequoiadb",
    "LoadPercent": 46,
    "TotalSpace": 84543193088,
    "FreeSpace": 45332840448
  },
  "TotalNumConnects": 11,
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "ReplUpdate": 0,
  "ReplDelete": 0,
  "ReplInsert": 0,
  "TotalSelect": 0,
  "TotalRead": 0,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "ActivateTimestamp": "2014-08-07-13.04.16.248083",
  "ResetTimestamp": "2014-08-07-13.04.16.248083",
  "UserCPU": "7.980000",
  "SysCPU": "10.700000",
  "freeLogSpace": 1342177280,
  "vsize": 1745002496,
  "rss": 12929,
  "fault": 12,
  "TotalMapped": 918945792,
  "svcNetIn": 3051,
  "svcNetOut": 9245,
  "shardNetIn": 3054,
  "shardNetOut": 9265,
  "replNetIn": 0,
  "replNetOut": 0
  "MemPoolSize": 56868864
}
```

##协调节点示例##

```lang-javascript
> coord.snapshot( SDB_SNAP_DATABASE )
{
  "TotalNumConnects": 0,
  "TotalDataRead": 4,
  "TotalIndexRead": 0,
  "TotalDataWrite": 3,
  "TotalIndexWrite": 3,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 3,
  "ReplUpdate": 0,
  "ReplDelete": 0,
  "ReplInsert": 2,
  "TotalSelect": 606,
  "TotalRead": 4,
  "TotalReadTime": 0,
  "TotalWriteTime": 0,
  "freeLogSpace": 5368709120,
  "vsize": 5660057600,
  "rss": 44765,
  "fault": 25,
  "TotalMapped": 2144206848,
  "svcNetIn": 0,
  "svcNetOut": 0,
  "shardNetIn": 38228,
  "shardNetOut": 393997,
  "replNetIn": 40743956,
  "replNetOut": 40743956,
  "ErrNodes": [
    {
      "NodeName": "hostname1:11850",
      "GroupName": "group2",
      "Flag": -79,
      "ErrInfo": {}
    }
  ]
}
```
