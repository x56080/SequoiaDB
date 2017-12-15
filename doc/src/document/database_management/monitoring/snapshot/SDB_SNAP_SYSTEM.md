##描述##

操作系统快照 SDB_SNAP_SYSTEM 列出当前数据库节点所在操作系统中主要的状态与性能监控参数，输出一条记录。

##标示##

SDB_SNAP_SYSTEM

##非协调节点字段信息##

| 字段名               | 类型   |  描述                                                          |
| -------------------- | ------ | -------------------------------------------------------------- |
| HostName             | 字符串 | 数据库节点所在物理节点的主机名                                 |
| ServiceName          | 字符串 | svcname 所指定的服务名，与 HostName 共同作为一个逻辑节点的标示 |
| NodeName             | 字符串 | 节点名，为“< HostName > : < ServiceName >”                     |
| GroupName            | 字符串 | 该逻辑节点所属的分区组名，standalone 模式下，该字段为空字符串  |
| IsPrimary            | 布尔   | 该节点是否为主节点，standalone 模式下，该字段为 false          |
| ServiceStatus        | 布尔   | 是否为可提供服务状态。<br>一些特殊状态，例如 [全量同步](infrastructure/replication/replicate.md) 会使该状态为 false |
| BeginLSN.Offset      | 长整型 | 起始 LSN 的偏移                                                |
| BeginLSN.Version     | 整型   | 起始 LSN 的版本号                                              |
| CurrentLSN.Offset    | 整型   | 当前 LSN 的偏移                                                |
| CurrentLSN.Version   | 整型   | 当前 LSN 的版本号                                              |
| CommittedLSN.Offset  | 整型   | 已提交 LSN 的偏移                                              |
| CommittedLSN.Version | 整型   | 已提交 LSN 的版本号                                            |
| CompleteLSN          | 整型   | 已完成 LSN 的偏移                                              |
| LSNQueSize           | 整型   | 等待同步的LSN队列长度                                          |
| TransInfo.TotalCount | 长整型 | 正在执行的事务数量                                             |
| TransInfo.BeginLSN   | 长整型 | 正在执行的事务的起始 LSN 的偏移                                |
| NodeID               | 数组   | 节点的 ID，为“[ <分区组 ID>, <节点 ID> ]”<br>在 standalone 模式下，该字段为“[ 0，0 ]” |
| CPU.User             | 浮点数 | 操作系统启动后所消耗的总用户 CPU 时间（单位：秒）              |
| CPU.Sys              | 浮点数 | 操作系统启动后所消耗的总系统 CPU 时间（单位：秒）              |
| CPU.Idle             | 浮点数 | 操作系统启动后所消耗的总空闲 CPU 时间（单位：秒）              |
| CPU.Other            | 浮点数 | 操作系统启动后所消耗的总其它 CPU 时间（单位：秒）              |
| Memory.LoadPercent   | 整型   | 当前操作系统的内存使用百分比（包括文件系统缓存）<br><b>重要：该字段仅在数据节点和编目节点显示，协调节点不显示</b> |
| Memory.TotalRAM      | 长整型 | 当前操作系统的总内存空间（单位：字节）                         |
| Memory.FreeRAM       | 长整型 | 当前操作系统的空闲内存空间（单位：字节）                       |
| Memory.TotalSwap     | 长整型 | 当前操作系统的总交换空间（单位：字节）                         |
| Memory.FreeSwap      | 长整型 | 当前操作系统的空闲交换空间（单位：字节）                       |
| Memory.TotalVirtual  | 长整型 | 当前操作系统的总虚拟空间（单位：字节）                         |
| Memory.FreeVirtual   | 长整型 | 当前操作系统的空闲虚拟空间（单位：字节）                       |
| Disk.Name            | 字符串 | 数据库路径所在的磁盘名称<br>                                   |
| Disk.DatabasePath    | 字符串 | 数据库路径<br><b>重要：该字段及以上字段仅在数据节点和编目节点显示，协调节点不显示</b> |
| Disk.LoadPercent     | 整型   | 数据库路径所在文件系统的空间占用百分比<br><b>重要：该字段及以上字段仅在数据节点和编目节点显示，协调节点不显示</b> |
| Disk.TotalSpace      | 长整型 | 数据库路径总空间（单位：字节）                                 |
| Disk.FreeSpace       | 长整型 | 数据库路径空闲空间（单位：字节）                               |

##协调节点字段信息##

| 字段名              | 类型   | 描述                                              |
| ------------------- | ------ | ------------------------------------------------- |
| CPU.User            | 浮点数 | 操作系统启动后所消耗的总用户 CPU 时间（单位：秒） |
| CPU.Sys             | 浮点数 | 操作系统启动后所消耗的总系统 CPU 时间（单位：秒） |
| CPU.Idle            | 浮点数 | 操作系统启动后所消耗的总空闲 CPU 时间（单位：秒） |
| CPU.Other           | 浮点数 | 操作系统启动后所消耗的总其它 CPU 时间（单位：秒） |
| Memory.LoadPercent  | 整型   | 当前操作系统的内存使用百分比（包括文件系统缓存）<br><b>重要：该字段仅在数据节点和编目节点显示，协调节点不显示</b> |
| Memory.TotalRAM     | 长整型 | 当前操作系统的总内存空间（单位：字节）            |
| Memory.FreeRAM      | 长整型 | 当前操作系统的空闲内存空间（单位：字节）          |
| Memory.TotalSwap    | 长整型 | 当前操作系统的总交换空间（单位：字节）            |
| Memory.FreeSwap     | 长整型 | 当前操作系统的空闲交换空间（单位：字节）          |
| Memory.TotalVirtual | 长整型 | 当前操作系统的总虚拟空间（单位：字节）            |
| Memory.FreeVirtual  | 长整型 | 当前操作系统的空闲虚拟空间（单位：字节）          |
| Disk.DatabasePath   | 字符串 | 数据库路径<br><b>重要：该字段及以上字段仅在数据节点和编目节点显示，协调节点不显示</b> |
| Disk.LoadPercent    | 整型   | 数据库路径所在文件系统的空间占用百分比<br><b>重要：该字段及以上字段仅在数据节点和编目节点显示，协调节点不显示</b> |
| Disk.TotalSpace     | 长整型 | 数据库路径总空间（单位：字节）                    |
| Disk.FreeSpace      | 长整型 | 数据库路径空闲空间（单位：字节）                  |
| ErrNodes.NodeName   | 字符串 | 返回异常节点名（主机名 + 端口）<br><b>重要：此字段仅在协调节点上显示，并且有异常节点时才显示</b> |
| ErrNodes.Flag       | 整型   | 错误码，详细请参见：[错误码](reference/Sequoiadb_error_code.md) <br><b>重要：此字段仅在协调节点上显示，并且有异常节点时才显示</b> |

##非协调节点示例##

```lang-javascript
> db.snapshot( SDB_SNAP_SYSTEM )
{
  "NodeName": "hostname1:11820",
  "HostName": "hostname1",
  "ServiceName": "11820",
  "GroupName": "group1",
  "IsPrimary": false,
  "ServiceStatus": true,
  "BeginLSN": {
    "Offset": 0,
    "Version": 1
  },
  "CurrentLSN": {
    "Offset": 3764,
    "Version": 1
  },
  "CommittedLSN": {
    "Offset": 3764,
    "Version": 1
  },
  "NodeID": [
    1000,
    1000
  ],
  "TransInfo": {
    "TotalCount": 0,
    "BeginLSN": -1
    },
  "NodeID": [
    0,
    0
    ],
  "CPU": {
    "User": 3947.31,
    "Sys": 715.11,
    "Idle": 331196.41,
    "Other": 771.14
  },
  "Memory": {
    "LoadPercent": 95,
    "TotalRAM": 4155072512,
    "FreeRAM": 202219520,
    "TotalSwap": 2153771008,
    "FreeSwap": 2137071616,
    "TotalVirtual": 6308843520,
    "FreeVirtual": 2339291136
  },
  "Disk": {
    "Name":"/dev/sda1",
    "DatabasePath": "/opt/sequoiadb/database/data/11820",
    "LoadPercent": 78,
    "TotalSpace": 40704466944,
    "FreeSpace": 8615747584
  }
}
```

##协调节点示例##

```lang-javascript
> coord.snapshot( SDB_SNAP_SYSTEM )
{
  "CPU": {
    "User": 36280.72,
    "Sys": 5046.23,
    "Idle": 7560242.4,
    "Other": 5887.24
  },
  "Memory": {
    "TotalRAM": 8403730432,
    "FreeRAM": 3075035136,
    "TotalSwap": 25757204480,
    "FreeSwap": 25663799296,
    "TotalVirtual": 34160934912,
    "FreeVirtual": 28738834432
  },
  "Disk": {
    "TotalSpace": 338172772352,
    "FreeSpace": 181331296256
  },
  "ErrNodes": []
}
```
