[^_^]:
    $SNAPSHOT_SYSTEM

操作系统快照可以列出当前操作系统的状态和监控信息。

>**Note:**
>
> 通过协调节点查询快照，将分别返回所有节点的快照信息；通过非协调节点查询快照，则返回当前节点的快照信息。

##标识##

$SNAPSHOT_SYSTEM

##字段信息##

| 字段名               | 类型   |  描述                                                          |
| -------------------- | ------ | -------------------------------------------------------------- |
| NodeName             | string | 节点名，格式为`<主机名>:<服务名>`                              |
| HostName             | string | 数据库的主机名                                                 |
| ServiceName          | string | 数据库的服务名                                                 |
| GroupName            | string | 节点所属复制组的名称，standalone 模式下该字段为空字符串  |
| IsPrimary            | boolean| 是否为主节点，standalone 模式下该字段为 false          |
| ServiceStatus        | boolean| 是否为可提供服务状态<br>一些特殊状态，例如[全量同步][architecture]时，服务状态为 false |
| Status               | string | 数据库状态，如：Normal、Shutdown、Rebuilding、FullSync、OfflineBackup |
| BeginLSN.Offset      | int64  | 起始 LSN 的偏移                                                |
| BeginLSN.Version     | int32  | 起始 LSN 的版本号                                              |
| CurrentLSN.Offset    | int64  | 当前 LSN 的偏移                                                |
| CurrentLSN.Version   | int32  | 当前 LSN 的版本号                                              |
| CommittedLSN.Offset  | int64  | 已提交 LSN 的偏移                                              |
| CommittedLSN.Version | int32  | 已提交 LSN 的版本号                                            |
| CompleteLSN          | int64  | 已完成 LSN 的偏移                                              |
| LSNQueSize           | int32  | 等待同步的 LSN 队列长度                                          |
| TransInfo.TotalCount | int32  | 正在执行的事务数量                                             |
| TransInfo.BeginLSN   | int64  | 正在执行的事务的起始 LSN 的偏移                                |
| NodeID               | array  | 节点的 ID 信息，格式为`[<分区组 ID>, <节点 ID>]`<br>standalone 模式下该字段为[0, 0] |
| CPU.User             | double | 操作系统启动后累计的用户 CPU 时间，单位为秒                    |
| CPU.Sys              | double | 操作系统启动后累计的系统 CPU 时间，单位为秒                    |
| CPU.Idle             | double | 操作系统启动后累计的空闲时间（不包括 IO 等待时间），单位为秒   |
| CPU.IOWait           | double | 操作系统启动后累计的 IO 等待时间，单位为秒                     |
| CPU.Other            | double | 操作系统启动后软中断和硬中断的累计时间，单位为秒               |
| Memory.LoadPercent   | int32  | 操作系统的内存使用百分比（包括文件系统缓存）               |
| Memory.TotalRAM      | int64  | 操作系统的总内存空间，单位为字节                           |
| Memory.FreeRAM       | int64  | 操作系统的空闲内存空间，单位为字节                         |
| Memory.AvailableRAM  | int64  | 操作系统的可用内存空间，单位为字节                         |
| Memory.TotalSwap     | int64  | 操作系统的总交换空间，单位为字节                           |
| Memory.FreeSwap      | int64  | 操作系统的空闲交换空间，单位为字节                         |
| Memory.TotalVirtual  | int64  | 操作系统的总虚拟空间，单位为字节                           |
| Memory.FreeVirtual   | int64  | 操作系统的空闲虚拟空间，单位为字节                         |
| Disk.Name            | string | 节点数据文件所在磁盘的名称                                       |
| Disk.DatabasePath    | string | 节点数据文件所在路径                                                     |
| Disk.LoadPercent     | int32  | 节点数据文件所在文件系统的空间占用百分比                         |
| Disk.TotalSpace      | int64  | 节点数据文件所在磁盘的总存储空间，单位为字节                                   |
| Disk.FreeSpace       | int64  | 节点数据文件所在磁盘的空闲存储空间，单位为字节                                 |

##示例##

查看操作系统快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_SYSTEM" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:42000",
  "HostName": "hostname",
  "ServiceName": "42000",
  "GroupName": "db2",
  "IsPrimary": true,
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
    1003
  ],
  "CPU": {
    "User": 178552.74,
    "Sys": 58392.44,
    "Idle": 6400173.12,
    "IOWait": 22336.26,
    "Other": 7856.64
  },
  "Memory": {
    "LoadPercent": 66,
    "TotalRAM": 8370360320,
    "FreeRAM": 162598912,
    "AvailableRAM": 2795474944,
    "TotalSwap": 16383401984,
    "FreeSwap": 16046903296,
    "TotalVirtual": 24753762304,
    "FreeVirtual": 18842378240
  },
  "Disk": {
    "Name": "/dev/mapper/vgdata-lvdata1",
    "DatabasePath": "/opt/test/42000/",
    "LoadPercent": 34,
    "TotalSpace": 211139878912,
    "FreeSpace": 138432401408
  }
}
```



[^_^]:
    本文使用的所有引用及链接
[architecture]:manual/Distributed_Engine/Architecture/Replication/architecture.md#全量同步
[snapshot]:manual/Manual/Sequoiadb_Command/Sdb/snapshot.md
