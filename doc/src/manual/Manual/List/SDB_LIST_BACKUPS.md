
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
| BeginLSNOffset | int64  | 起始 LSN 的偏移              |
| EndLSNOffset   | int64  | 结尾 LSN 的偏移              |
| TransLSNOffset | int64  | 事务当前的日志 LSN 偏移               |
| StartTime      | string | 备份开始时间                     |
| LastLSN        | int64  | 最后的日志 LSN     |
| LastLSNCode    | int32  | LastLSN 的哈希值   |
| HasError       | boolean| 是否发生异常                     |

##示例##

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

[^_^]:
    本文使用的所有引用及链接
[backup]:manual/Manual/Sequoiadb_Command/Sdb/backup.md