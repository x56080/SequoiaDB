[^_^]:
    操作系统快照

操作系统快照可以列出当前操作系统的状态和监控信息。

>**Note:**
>
> 协调节点通过聚合所有节点的数据（非协调节点字段信息）得到协调节点字段信息。用户可以通过 `coord.snapshot(SDB_SNAP_SYSTEM, {RawData:true})` 获取聚合前的数据。

##标识##

SDB_SNAP_SYSTEM

##非协调节点字段信息##

| 字段名               | 类型      |  描述                                                          |
| -------------------- | --------- | -------------------------------------------------------------- |
| NodeName             | string    | 节点名，格式为`<主机名>:<服务名>`                              |
| HostName             | string    | 数据库的主机名                                                 |
| ServiceName          | string    | 数据库的服务名                                                 |
| GroupName            | string    | 节点所属复制组的名称，standalone 模式下该字段为空字符串    |
| IsPrimary            | boolean   | 是否为主节点，standalone 模式下该字段为 false                  |
| ServiceStatus        | boolean   | 是否为可提供服务状态 <br>一些特殊状态，例如[全量同步][replicate_url]时，服务状态为 false |
| Status               | string    | 数据库状态，如：Normal、Shutdown、Rebuilding、FullSync、OfflineBackup |
| BeginLSN.Offset       | int64 | 节点同步日志的起始 LSN |
| BeginLSN.Version      | int32   | 版本号（内部使用）                                               |
| CurrentLSN.Offset     | int64 | 节点同步日志的当前 LSN<br>该字段可用于查看同步日志的结束位置  |
| CurrentLSN.Version    | int32   | 版本号（内部使用）                                                    |
| CommittedLSN.Offset   | int64 | 已刷盘的同步日志对应的 LSN  |
| CommittedLSN.Version  | int32   | 版本号（内部使用）                                                        |
| CompleteLSN           | int64     | 备节点已重放记录对应的 LSN |
| LSNQueSize           | int32     | 等待同步的LSN队列长度                                          |
| TransInfo.TotalCount | int32   | 正在执行的事务数量                                             |
| TransInfo.BeginLSN   | int64 | 正在执行的事务的起始 LSN（内部使用）                                |
| NodeID               | bson array| 节点的 ID 信息，格式为`[<分区组 ID>, <节点 ID>]`<br>standalone 模式下该字段为[0, 0] |
| CPU.User             | double | 操作系统启动后累计的用户 CPU 时间，单位为秒      |
| CPU.Sys              | double | 操作系统启动后累计的系统 CPU 时间，单位为秒             |
| CPU.Idle             | double | 操作系统启动后累计的空闲时间（不包括 IO 等待时间），单位为秒 |
| CPU.IOWait           | double | 操作系统启动后累计的 IO 等待时间，单位为秒     |
| CPU.Other            | double | 操作系统启动后软中断和硬中断的累计时间，单位为秒   |
| Memory.LoadPercent   | int32  | 操作系统的内存使用百分比（包括文件系统缓存）               |
| Memory.TotalRAM      | int64  | 操作系统的总内存空间，单位为字节                        |
| Memory.FreeRAM       | int64  | 操作系统的空闲内存空间，单位为字节                   |
| Memory.AvailableRAM  | int64  | 操作系统的可用内存空间，单位为字节                    |
| Memory.TotalSwap     | int64  | 操作系统的总交换空间，单位为字节                          |
| Memory.FreeSwap      | int64  | 操作系统的空闲交换空间，单位为字节                        |
| Memory.TotalVirtual  | int64  | 操作系统的总虚拟空间，单位为字节                         |
| Memory.FreeVirtual   | int64  | 操作系统的空闲虚拟空间，单位为字节                        |
| Disk.Name            | string | 节点数据文件所在磁盘的名称                                   |
| Disk.DatabasePath    | string | 节点数据文件所在路径                                                    |
| Disk.LoadPercent     | int32  | 节点数据文件所在文件系统的空间占用百分比                         |
| Disk.TotalSpace      | int64  | 节点数据文件所在磁盘的总存储空间，单位为字节                                 |
| Disk.FreeSpace       | int64  | 节点数据文件所在磁盘的空闲存储空间，单位为字节                               |


##协调节点字段信息##

| 字段名      | 类型   |  描述                            |
| ----------- | ------ | -------------------------------- |
| CPU.User             | double | 操作系统启动后累计的用户 CPU 时间，单位为秒      |
| CPU.Sys              | double | 操作系统启动后累计的系统 CPU 时间，单位为秒             |
| CPU.Idle             | double | 操作系统启动后累计的空闲时间（不包括 IO 等待时间），单位为秒 |
| CPU.IOWait           | double | 操作系统启动后累计的 IO 等待时间，单位为秒      |
| CPU.Other            | double | 操作系统启动后软中断和硬中断的累计时间，单位为秒   |
| Memory.TotalRAM      | int64  | 操作系统的总内存空间，单位为字节        |
| Memory.FreeRAM       | int64  | 操作系统的空闲内存空间，单位为字节                   |
| Memory.AvailableRAM  | int64  | 操作系统的可用内存空间，单位为字节                    |
| Memory.TotalSwap     | int64  | 操作系统的总交换空间，单位为字节                          |
| Memory.FreeSwap      | int64  | 操作系统的空闲交换空间，单位为字节                        |
| Memory.TotalVirtual  | int64  | 操作系统的总虚拟空间，单位为字节    |
| Memory.FreeVirtual   | int64  | 操作系统的空闲虚拟空间，单位为字节  |
| Disk.TotalSpace      | int64  | 节点数据文件所在磁盘的总存储空间，单位为字节<br>如果数据文件存储在多个磁盘，该字段值为所有磁盘的存储空间总和 |
| Disk.FreeSpace       | int64  | 节点数据文件所在磁盘的空闲存储空间，单位为字节<br>如果数据文件存储在多个磁盘，该字段值为所有磁盘的空闲存储空间总和   |
| ErrNodes.NodeName | string    | 异常节点名，格式为`<主机名>:<服务名>`                    |
| ErrNodes.GroupName| string    | 异常节点所属复制组的名称                                   |
| ErrNodes.Flag     | int32     | 异常节点的[错误码][error_code_url]                     |
| ErrNodes.ErrInfo  | bson      | 异常节点的错误信息                                     |


##应用场景##

###查看快照信息###

- 通过非协调节点查看快照

    ```lang-javascript
    > db.snapshot(SDB_SNAP_SYSTEM)
    ```

    输出结果如下：

    ```lang-json
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

- 通过协调节点查看快照

    ```lang-javascript
    > db.snapshot(SDB_SNAP_SYSTEM)
    ```

    输出结果如下：

    ```lang-json
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
    本文使用到的所有链接及引用。

[replicate_url]: manual/Distributed_Engine/Architecture/Thread_Model/edu.md
[error_code_url]: manual/Manual/Sequoiadb_error_code.md