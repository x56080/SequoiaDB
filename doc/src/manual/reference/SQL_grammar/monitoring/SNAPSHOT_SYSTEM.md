##描述##

操作系统快照 $SNAPSHOT_SYSTEM 列出当前数据库节点所在操作系统中主要的状态与性能监控参数，输出一条记录。

##标示##

$SNAPSHOT_SYSTEM

##字段信息##

| 字段名               | 类型   |  描述                                                          |
| -------------------- | ------ | -------------------------------------------------------------- |
| NodeName             | 字符串 | 节点名，为“< HostName > : < ServiceName >”                     |
| HostName             | 字符串 | 数据库节点所在物理节点的主机名                                 |
| ServiceName          | 字符串 | svcname 所指定的服务名，与 HostName 共同作为一个逻辑节点的标示 |
| GroupName            | 字符串 | 该逻辑节点所属的分区组名，standalone 模式下，该字段为空字符串  |
| IsPrimary            | 布尔   | 该节点是否为主节点，standalone 模式下，该字段为 false          |
| ServiceStatus        | 布尔   | 是否为可提供服务状态。<br>一些特殊状态，例如[全量同步](manual/infrastructure/Replication/architecture.md#数据复制)会使该状态为 false |
| Status               | 字符串 | 数据库状态：包括"Normal"、 "Shutdown"、"Rebuilding"、"FullSync"、"OfflineBackup"状态 |
| BeginLSN.Offset      | 长整型 | 起始 LSN 的偏移                                                |
| BeginLSN.Version     | 整型   | 起始 LSN 的版本号                                              |
| CurrentLSN.Offset    | 长整型 | 当前 LSN 的偏移                                                |
| CurrentLSN.Version   | 整型   | 当前 LSN 的版本号                                              |
| CommittedLSN.Offset  | 长整型 | 已提交 LSN 的偏移                                              |
| CommittedLSN.Version | 整型   | 已提交 LSN 的版本号                                            |
| CompleteLSN          | 长整型 | 已完成 LSN 的偏移                                              |
| LSNQueSize           | 整型   | 等待同步的LSN队列长度                                          |
| TransInfo.TotalCount | 整型   | 正在执行的事务数量                                             |
| TransInfo.BeginLSN   | 长整型 | 正在执行的事务的起始 LSN 的偏移                                |
| NodeID               | 数组   | 节点的 ID，为“[ <分区组 ID>, <节点 ID> ]”<br>在 standalone 模式下，该字段为“[ 0，0 ]” |
| CPU.User             | 浮点数 | 操作系统启动后所消耗的总用户 CPU 时间（单位：秒）              |
| CPU.Sys              | 浮点数 | 操作系统启动后所消耗的总系统 CPU 时间（单位：秒）              |
| CPU.Idle             | 浮点数 | 操作系统启动后所消耗的总空闲 CPU 时间（单位：秒）              |
| CPU.Other            | 浮点数 | 操作系统启动后所消耗的总其它 CPU 时间（单位：秒）              |
| Memory.LoadPercent   | 整型   | 当前操作系统的内存使用百分比（包括文件系统缓存）               |
| Memory.TotalRAM      | 长整型 | 当前操作系统的总内存空间（单位：字节）                         |
| Memory.FreeRAM       | 长整型 | 当前操作系统的空闲内存空间（单位：字节）                       |
| Memory.TotalSwap     | 长整型 | 当前操作系统的总交换空间（单位：字节）                         |
| Memory.FreeSwap      | 长整型 | 当前操作系统的空闲交换空间（单位：字节）                       |
| Memory.TotalVirtual  | 长整型 | 当前操作系统的总虚拟空间（单位：字节）                         |
| Memory.FreeVirtual   | 长整型 | 当前操作系统的空闲虚拟空间（单位：字节）                       |
| Disk.Name            | 字符串 | 数据库路径所在的磁盘名称<br>                                   |
| Disk.DatabasePath    | 字符串 | 数据库路径                                                     |
| Disk.LoadPercent     | 整型   | 数据库路径所在文件系统的空间占用百分比                         |
| Disk.TotalSpace      | 长整型 | 数据库路径总空间（单位：字节）                                 |
| Disk.FreeSpace       | 长整型 | 数据库路径空闲空间（单位：字节）                               |

##示例##

```lang-javascript
> db.exec( "select * from $SNAPSHOT_SYSTEM" )
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
    "User": 53323.86,
    "Sys": 55738.17,
    "Idle": 3999129.94,
    "Other": 3124.02
  },
  "Memory": {
    "LoadPercent": 94,
    "TotalRAM": 6257471488,
    "FreeRAM": 340115456,
    "TotalSwap": 1022357504,
    "FreeSwap": 732004352,
    "TotalVirtual": 7279828992,
    "FreeVirtual": 1072119808
  },
  "Disk": {
    "Name": "/dev/mapper/vgdata-lvdata1",
    "DatabasePath": "/opt/test/42000/",
    "LoadPercent": 34,
    "TotalSpace": 211139878912,
    "FreeSpace": 138432401408
  }
}
...
```

