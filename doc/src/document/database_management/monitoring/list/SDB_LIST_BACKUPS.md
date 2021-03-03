##描述##

备份列表 SDB_LIST_BACKUPS 列出当前数据库的备份信息。

##标示##

SDB_LIST_BACKUPS

##字段信息##

| 字段名 | 类型   | 描述       |
| ------ | ------ | ---------- |
| Version | 整型   | 版本号      |
| Name   | 字符串 | 备份名称   |
| ID     |  整型 | 备份ID             |
| NodeName  | 字符串 | 节点主机名称       |
| GroupName  | 字符串   | 数据组名称             |
| EnsureInc  | 布尔值 | 是否开启增量备份                     |
| BeginLSNOffset | 长整型 | 起始 LSN 的偏移              |
| EndLSNOffset   | 长整型 | 结尾 LSN 的偏移              |
| TransLSNOffset | 长整型 | 事务当前的日志 LSN 的偏移               |
| StartTime      | 字符串 | 备份开始时间                     |
| LastLSN        | 长整型 | 最后的日志 LSN     |
| LastLSNCode    | 整型   | LastLSN 的哈希值   |
| HasError       | 布尔值 | 是否有错误                     |

##示例##

```lang-javascript
> db.list( SDB_LIST_BACKUPS )
{
  "Version": 2,
  "Name": "FullBackup1",
  "ID": 0,
  "NodeName": "hostname:30000",
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
