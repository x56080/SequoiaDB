##描述##

节点健康检测快照 $SNAPSHOT_HEALTH 列出数据库中所有节点的健康信息。

每一个节点上的健康检测信息为一条记录。

##标示##

$SNAPSHOT_HEALTH

###字段信息###

| 字段名               | 类型   | 描述                                           |
| -------------------- | ------ | ---------------------------------------------- |
| NodeName             | 字符串 | 节点名，为“< HostName > : < ServiceName >” |
| IsPrimary            | 布尔   | 是否为主节点 |
| ServiceStatus        | 布尔   | 是否为可提供服务状态<br>一些特殊状态，例如[全量同步](manual/infrastructure/Replication/architecture.md#数据复制)会使该状态为 false |
| Status               | 字符串 | 节点状态：<br/>1."Normal"：正常工作状态。<br/>2."Shutdown"：正在关闭状态，表示节点正在被关闭。<br/>3."Rebuilding"：重新构建状态，如节点异常重启后，无法与其他节点进行数据同步，则节点会进入该状态，重新构建数据。<br/>4."FullSync"：[全量同步](manual/infrastructure/Replication/architecture.md#数据复制)状态。<br/>5."OfflineBackup"：[数据备份](manual/Maintainance/Backup_Recovery/regular_bar.md)状态。|
| BeginLSN.Offset      | 长整型 | 起始 LSN 的偏移 |
| BeginLSN.Version     | 整型   | 起始 LSN 的版本号 |
| CurrentLSN.Offset    | 长整型 | 当前 LSN 的偏移 |
| CurrentLSN.Version   | 整型   | 当前 LSN 的版本号 |
| CommittedLSN.Offset  | 长整型 | 已提交 LSN 的偏移 |
| CommittedLSN.Version | 整型   | 已提交 LSN 的版本号 |
| CompleteLSN          | 长整型 | 已完成 LSN 的偏移 |
| LSNQueSize           | 整型   | 等待同步的LSN队列长度 |
| NodeID               | 数组   | 节点的 ID，为“[ <分区组 ID>, <节点 ID> ]”<br>在 standalone 模式下，该字段为“[ 0，0 ]” |
| DataStatus           | 字符串 | 数据状态:<br/>1."Normal": 正常状态。<br/>2."Repairing"：修复状态，当节点状态为 "Rebuilding" 或 "FullSync" 时，数据状态为 "Repairing"。 <br/>3."Fault"：错误状态，当节点异常启动，且节点状态不为"Rebuilding" 或 "FullSync" 时，数据状态为 "Fault"。 |
| SyncControl          | 布尔   | 节点是否处于同步控制 |
| Ulimit.CoreFileSize  | 长整型 | 节点进程的core文件大小限制（-1表示unlimited） |
| Ulimit.VirtualMemory | 长整型 | 节点进程的虚拟内存限制（-1表示unlimited） |
| Ulimit.OpenFiles     | 长整型 | 节点进程的文件句柄数限制 |
| Ulimit.NumProc       | 长整型 | 节点进程的线程数限制（-1表示unlimited） |
| Ulimit.FileSize      | 长整型 | 节点进程的文件大小限制（-1表示unlimited） |
| ResetTimestamp       | 时间戳 | 重置快照的时间 |
| ErrNum.SDB_OOM       | 长整型 | 节点发生错误 SDB_OOM 的次数 |
| ErrNum.SDB_NOSPC     | 长整型 | 节点发生错误 SDB_NOSPC 的次数 |
| ErrNum.SDB_TOO_MANY_OPEN_FD | 长整型 | 节点发生错误 SDB_TOO_MANY_OPEN_FD 的次数 |
| Memory.LoadPercent   | 整型   | 节点进程占用 RAM 的百分比 |
| Memory.TotalRAM      | 长整型 | 节点所在操作系统的总 RAM 大小（单位：字节） |
| Memory.RssSize       | 长整型 | 节点进程占用的 RAM 大小（单位：字节） |
| Memory.LoadPercentVM | 整型   | 节点进程占用虚拟空间的百分比 |
| Memory.VMLimit       | 长整型 | 节点进程虚拟空间限制（单位：字节） |
| Memory.VMSize        | 长整型 | 节点进程占用的虚拟空间（单位：字节） |
| Disk.Name            | 字符串 | 节点路径所在的磁盘名称 |
| Disk.LoadPercent     | 整型   | 节点路径占用磁盘的百分比 |
| Disk.TotalSpace      | 长整型 | 节点路径所在的磁盘空间大小（单位：字节） |
| Disk.FreeSpace       | 长整型 | 节点路径所在的磁盘剩余空间大小（单位：字节） |
| FileDesp.LoadPercent | 整型   | 节点进程占用的文件句柄的百分比 |
| FileDesp.TotalNum    | 长整型 | 节点进程文件句柄限制 |
| FileDesp.FreeNum     | 长整型 | 节点进程剩余的文件句柄个数 |
| StartHistory         | 数组   | 节点启动历史（只取最新的十条记录） |
| AbnormalHistory      | 数组   | 节点异常后启动历史（只取最新的十条记录） |
| DiffLSNWithPrimary   | 长整型 | 与主节点的 LSN 差异 |

> Note:
>
> * 协调节点的快照返回所有节点的信息。非协调节点返回自身节点的信息。
> * 备节点在计算与主节点的 LSN 差异时，所取的主节点 LSN 可能是2秒钟前的，因此 DiffLSNWithPrimary 可能与实际值存在一定偏差。（2秒是一个心跳间隔）

##示例##

查看数据节点 20000 上的健康检测信息

```lang-javascript
> db.exec( "select * from $SNAPSHOT_HEALTH" )
{
  "NodeName": "hostname:20000",
  "IsPrimary": true,
  "ServiceStatus": true,
  "Status": "Normal",
  "BeginLSN": {
    "Offset": 0,
    "Version": 1
  },
  "CurrentLSN": {
    "Offset": 1050290908,
    "Version": 1
  },
  "CommittedLSN": {
    "Offset": 1050290908,
    "Version": 1
  },
  "CompleteLSN": 1050290984,
  "LSNQueSize": 0,
  "NodeID": [
    1000,
    1000
  ],
  "DataStatus": "Normal",
  "SyncControl": false,
  "Ulimit": {
    "CoreFileSize": 0,
    "VirtualMemory": -1,
    "OpenFiles": 1024,
    "NumProc": 23711,
    "FileSize": -1
  },
  "ResetTimestamp": "2019-05-31-09.37.59.316262",
  "ErrNum": {
    "SDB_OOM": 0,
    "SDB_NOSPC": 0,
    "SDB_TOO_MANY_OPEN_FD": 0
  },
  "Memory": {
    "LoadPercent": 11,
    "TotalRAM": 6257471488,
    "RssSize": 701349888,
    "LoadPercentVM": 0,
    "VMLimit": -1,
    "VMSize": 2491211776
  },
  "Disk": {
    "Name": "/dev/mapper/vgdata-lvdata1",
    "LoadPercent": 34,
    "TotalSpace": 211139878912,
    "FreeSpace": 138432405504
  },
  "FileDesp": {
    "LoadPercent": 4,
    "TotalNum": 1024,
    "FreeNum": 973
  },
  "StartHistory": [
    "2019-05-31-09.37.59.581769"
  ],
  "AbnormalHistory": [],
  "DiffLSNWithPrimary": 0
}
```
