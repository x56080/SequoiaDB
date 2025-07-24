##描述##

操作系统快照 SDB_SNAP_SYSTEM 列出当前数据库节点所在操作系统中主要的状态与性能监控参数，输出一条记录。

##标示##

SDB_SNAP_SYSTEM

##非协调节点字段信息##

| 字段名               | 类型   |  描述                                                          |
| -------------------- | ------ | -------------------------------------------------------------- |
| NodeName             | 字符串 | 节点名，为“< HostName > : < ServiceName >”                     |
| HostName             | 字符串 | 数据库节点所在物理节点的主机名                                 |
| ServiceName          | 字符串 | svcname 所指定的服务名，与 HostName 共同作为一个逻辑节点的标示 |
| GroupName            | 字符串 | 该逻辑节点所属的分区组名，standalone 模式下，该字段为空字符串  |
| IsPrimary            | 布尔   | 该节点是否为主节点，standalone 模式下，该字段为 false          |
| ServiceStatus        | 布尔   | 是否为可提供服务状态。<br>一些特殊状态，例如 [全量同步][syn]会使该状态为 false |
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
| TransInfo.GlobLowTran| 字符串 | 集群中正在执行的最小的全局事务                                 |
| TransInfo.GlobExpireTran | 字符串 | 集群中已经过期最大的事务，用于清理过期的 MVCC 老版本       |
| TransInfo.LowTran    | 字符串 | 本节点中正在执行的最小的全局事务                               |
| TransInfo.ExpireTran | 字符串 | 本节点中已经过期最大的事务，用于清理过期的 MVCC 老版本         |
| TransInfo.IdxTreeLowTran | 字符串 | MVCC 老版本索引树上最小的全局事务版本                      |
| NodeID               | 数组   | 节点的 ID，为“[ <分区组 ID>, <节点 ID> ]”<br>在 standalone 模式下，该字段为“[ 0，0 ]” |
| CPU.User             | 浮点数 | 操作系统启动后累计的用户 CPU 时间，单位为秒                    |
| CPU.Sys              | 浮点数 | 操作系统启动后累计的系统 CPU 时间，单位为秒                    |
| CPU.Idle             | 浮点数 | 操作系统启动后累计的空闲时间（不包括 IO 等待时间），单位为秒   |
| CPU.IOWait           | 浮点数 | 操作系统启动后累计的 IO 等待时间，单位为秒                     |
| CPU.Other            | 浮点数 | 操作系统启动后软中断和硬中断的累计时间，单位为秒               |
| Memory.LoadPercent   | 整型   | 当前操作系统的内存使用百分比（包括文件系统缓存）               |
| Memory.TotalRAM      | 长整型 | 当前操作系统的总内存空间（单位：字节）                         |
| Memory.FreeRAM       | 长整型 | 当前操作系统的空闲内存空间，单位为字节                         |
| Memory.AvailableRAM  | 长整型 | 当前操作系统可用的内存空间，单位为字节                         |
| Memory.TotalSwap     | 长整型 | 当前操作系统的总交换空间（单位：字节）                         |
| Memory.FreeSwap      | 长整型 | 当前操作系统的空闲交换空间（单位：字节）                       |
| Memory.TotalVirtual  | 长整型 | 当前操作系统的总虚拟空间（单位：字节）                         |
| Memory.FreeVirtual   | 长整型 | 当前操作系统的空闲虚拟空间（单位：字节）                       |
| Disk.Name            | 字符串 | 数据库路径所在的磁盘名称<br>                                   |
| Disk.DatabasePath    | 字符串 | 数据库路径                                                     |
| Disk.LoadPercent     | 整型   | 数据库路径所在文件系统的空间占用百分比                         |
| Disk.TotalSpace      | 长整型 | 数据库路径总空间（单位：字节）                                 |
| Disk.FreeSpace       | 长整型 | 数据库路径空闲空间（单位：字节）                               |

##协调节点字段信息##

| 字段名              | 类型   | 描述                                              |
| ------------------- | ------ | ------------------------------------------------- |
| CPU.User            | 浮点数 | 操作系统启动后累计的用户 CPU 时间，单位为秒       |
| CPU.Sys             | 浮点数 | 操作系统启动后累计的系统 CPU 时间，单位为秒       |
| CPU.Idle            | 浮点数 | 操作系统启动后累计的空闲时间（不包括 IO 等待时间），单位为秒   |
| CPU.IOWait          | 浮点数 | 操作系统启动后累计的 IO 等待时间，单位为秒        |
| CPU.Other           | 浮点数 | 操作系统启动后软中断和硬中断的累计时间，单位为秒  |
| Memory.TotalRAM     | 长整型 | 当前操作系统的总内存空间（单位：字节）            |
| Memory.FreeRAM      | 长整型 | 当前操作系统的空闲内存空间，单位为字节            |
| Memory.AvailableRAM | 长整型 | 当前操作系统可用的内存空间，单位为字节            |
| Memory.TotalSwap    | 长整型 | 当前操作系统的总交换空间（单位：字节）            |
| Memory.FreeSwap     | 长整型 | 当前操作系统的空闲交换空间（单位：字节）          |
| Memory.TotalVirtual | 长整型 | 当前操作系统的总虚拟空间（单位：字节）            |
| Memory.FreeVirtual  | 长整型 | 当前操作系统的空闲虚拟空间（单位：字节）          |
| Disk.TotalSpace     | 长整型 | 数据库路径总空间（单位：字节）                    |
| Disk.FreeSpace      | 长整型 | 数据库路径空闲空间（单位：字节）                  |
| ErrNodes.NodeName   | 字符串 | 返回异常节点名（主机名 + 端口）                   |
| ErrNodes.GroupName  | 字符串 | 返回异常节点所属分区组名                          |
| ErrNodes.Flag       | 整型   | 错误码，详细请参见：[错误码][Sequoiadb_error_code]|
| ErrNodes.ErrInfo    | 字符串 | 返回节点出错信息                                  |

> Note:
>
> 存在异常节点时才显示ErrNodes字段。

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
  "Status": "Normal",
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
  "CompleteLSN": 3865,
  "LSNQueSize": 0,
  "TransInfo": {
    "TotalCount": 0,
    "BeginLSN": -1
    "GlobLowTran": "0x1059d7d5bf71a05",
    "GlobExpireTran": "0x1059d7d5bf656b5",
    "LowTran": "0x1059d7d5bf71a05",
    "ExpireTran": "0x1059d7d5bf656b5",
    "IdxTreeLowTran": "0x1059d7d5861410f"
    },
  "NodeID": [
    1000,
    1000
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
    "User": 178552.74,
    "Sys": 58392.44,
    "Idle": 6400173.12,
    "IOWait": 22336.26,
    "Other": 7856.64
  },
  "Memory": {
    "TotalRAM": 8370360320,
    "FreeRAM": 162349056,
    "AvailableRAM": 2795397120,
    "TotalSwap": 16383401984,
    "FreeSwap": 16046911488,
    "TotalVirtual": 24753762304,
    "FreeVirtual": 18842308608
  },
  "Disk": {
    "TotalSpace": 338172772352,
    "FreeSpace": 181331296256
  },
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


[^_^]:
    本文使用的所有引用和链接
[syn]:manual/Distributed_Engine/Architecture/Replication/architecture.md#数据复制
[Sequoiadb_error_code]:reference/Sequoiadb_error_code.md
