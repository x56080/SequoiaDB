[^_^]: 
    备份列表

备份列表可以列出当前数据库的备份信息。

##标识##

SDB_LIST_BACKUPS

##字段信息##

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Version | int32   | 版本号      |
| Name   | string | 备份名称   |
| ID     |  int32  | 备份 ID             |
| Description | string | 备份描述 |
| Path | string | 备份路径（仅在备份参数 [IsSubDir][backup] 生效时显示） |
| NodeName  | string | 节点主机名称       |
| GroupName  | string   | 数据组名称             |
| EnsureInc  | boolean | 是否开启增量备份                     |
| BeginLSNOffset | int64  | 备份文件的起始 LSN |
| EndLSNOffset   | int64  | 备份文件的结尾 LSN<br>该字段结合 BeginLSNOffset 查看时，可用于[检查备份文件是否连续][check] |
| TransLSNOffset | int64  | 事务日志的起始 LSN（内部使用） |
| StartTime      | string | 备份开始时间 |
| LastLSN        | int64  | 备份文件最后一条记录对应的 LSN<br>该字段可用于[检查是否需要执行全量备份][backupALL]或[查看未备份的数据量][no_backup] |
| LastLSNCode    | int32  | LastLSN 的哈希值（内部使用）   |
| HasError       | boolean| 是否发生异常                     |

##示例##

###查看列表信息###

查看备份列表

```lang-javascript
> db.list(SDB_LIST_BACKUPS)
```

输出结果如下：

```lang-json
{
  "Version": 2,
  "Name": "FullBackup1",
  "ID": 0,
  "Description": "backup group1",
  "Path": "opt/test",
  "NodeName": "sdbserver:30000",
  "GroupName": "SYSCatalogGroup",
  "EnsureInc": false,
  "BeginLSNOffset": 8034100,
  "EndLSNOffset": 8034172,
  "TransLSNOffset": -1,
  "StartTime": "2019-07-23-16:32:10",
  "LastLSN": 8034100,
  "LastLSNCode": 575697176,
  "HasError": false
}
...
```

###检查备份文件是否连续###

用户可通过对比相邻备份文件下字段 EndLSNOffset 和 BeginLSNOffset 的值，检查备份文件是否连续。如果取值不相等，则说明相邻的两个备份文件不连续，需要重新执行[全量备份][data_backup]。具体操作步骤如下：

检查节点 11820 的备份文件是否连续

```lang-javascript
> db.list(SDB_LIST_BACKUPS, {NodeName: "sdbserver:11820"}, {Name: null, ID: null, BeginLSNOffset: null, EndLSNOffset: null})
```

输出结果如下：

```lang-json
{
  "Name": "backup",
  "ID": 0,
  "BeginLSNOffset": 80228,
  "EndLSNOffset": 80308
}
{
  "Name": "backup",
  "ID": 1,
  "BeginLSNOffset": 160308,
  "EndLSNOffset": 240616
}
```

###检查是否需要执行全量备份###

用户可通过对比最新一个备份文件下字段 LastLSN 与[节点健康检测快照][SDB_SNAP_HEALTH]下字段 BeginLSN 的取值差异，检查是否需要执行全量备份。如果 BeginLSN 的值大于 LastLSN，说明需要重新执行[全量备份][data_backup]。具体操作步骤如下：

1. 通过备份列表查看字段 LastLSN

    ```lang-javascript
    > db.list(SDB_LIST_BACKUPS, {}, {Name: null, ID: null, NodeName: null, LastLSN: null}, {LastLSN: -1})
    ```

    输出结果如下：

    ```lang-json
    {
      "Name": "backup",
      "ID": 4,
      "NodeName": "sdbserver:11820",
      "LastLSN": 160228
    }
    {
      "Name": "backup",
      "ID": 3,
      "NodeName": "sdbserver:11820",
      "LastLSN": 80228
    }
    ...
    ```

2. 通过节点健康检测快照查看字段 BeginLSN

    ```lang-javascript
    > db.snapshot(SDB_SNAP_HEALTH, {}, {NodeName: null, BeginLSN: null})
    ```

    输出结果如下：

    ```lang-json
    ...
    {
      "NodeName": "sdbserver:11820",
      "BeginLSN": {
       "Offset": 671088640,
       "Version": 1
      }
    }
    ...
    ```

3. 执行全量备份

###查看未备份的数据量###

用户可通过对比最新一个备份文件下字段 LastLSN 与[节点健康检测快照][SDB_SNAP_HEALTH]下字段 CurrentLSN 的取值差异，查看未备份的数据量。具体操作步骤如下：

1. 通过备份列表查看字段 LastLSN

    ```lang-javascript
    > db.list(SDB_LIST_BACKUPS, {}, {Name: null, ID: null, NodeName: null, LastLSN: null}, {LastLSN: -1})
    ```

    输出结果如下：

    ```lang-json
    {
      "Name": "backup",
      "ID": 4,
      "NodeName": "sdbserver:11820",
      "LastLSN": 160228
    }
    {
      "Name": "backup",
      "ID": 3,
      "NodeName": "sdbserver:11820",
      "LastLSN": 80228
    }
    ...
    ```

2. 通过节点健康检测快照查看字段 CurrentLSN

    ```lang-javascript
    > db.snapshot(SDB_SNAP_HEALTH, {}, {NodeName: null, CurrentLSN: null})
    ```

    输出结果如下：

    ```lang-json
    ...
    {
      "NodeName": "sdbserver:11820",
      "CurrentLSN": {
        "Offset": 157288368,
        "Version": 1
      }
    }
    ...
    ```

[^_^]:
    本文使用的所有引用及链接
[SDB_SNAP_HEALTH]:manual/Manual/Snapshot/SDB_SNAP_HEALTH.md
[check]:manual/Manual/List/SDB_LIST_BACKUPS.md#检查备份文件是否连续
[no_backup]:manual/Manual/List/SDB_LIST_BACKUPS.md#查看未备份的数据量
[backupALL]:manual/Manual/List/SDB_LIST_BACKUPS.md#检查是否需要执行全量备份
[data_backup]:manual/Distributed_Engine/Maintainance/Backup_Recovery/data_backup.md#全量备份