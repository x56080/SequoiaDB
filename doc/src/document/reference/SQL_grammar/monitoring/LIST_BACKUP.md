##描述##

备份列表 $LIST_BACKUP 列出当前数据库的备份信息。

##标示##

$LIST_BACKUP

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
| EndTime        | 字符串 | 备份结束时间                     |
| UseTime        | 整型   | 备份所花的时间                     |
| CSNum          | 整型   | 备份的集合空间数量     |
| DataFileNum    | 整型   | 备份的文件数量     |
| BeginDataFileSeq  | 整型   | 备份的文件起始序号     |
| LastDataFileSeq   | 整型   | 备份的文件结束序号     |
| LastExtentID      | 长整型   | 上一个数据块的 ID     |
| DataSize       | 长整型    | 备份的数据大小     |
| ThinDataSize   | 长整型    | 备份数据中未压缩的数据大小     |
| CompressedDataSize | 长整型 | 备份数据中已压缩的数据大小    |
| CompressedRatio | 浮点数   | 压缩率    |
| CompressionType | 字符串   | 压缩类型    |
| LastLSN        | 长整型 | 最后的日志 LSN     |
| LastLSNCode    | 整型   | LastLSN 的哈希值   |
| HasError       | 布尔值 | 是否有错误      |

##示例##

```lang-javascript
> db.exec( "select * from $LIST_BACKUP" )
{
  "Version": 2,
  "Name": "2019-08-14-13:27:02",
  "ID": 0,
  "NodeName": "u1604-ljh:42000",
  "GroupName": "db2",
  "EnsureInc": false,
  "BeginLSNOffset": 6645140616,
  "EndLSNOffset": 6645140668,
  "TransLSNOffset": -1,
  "StartTime": "2019-08-14-13:27:02",
  "EndTime": "2019-08-14-13:27:02",
  "UseTime": 0,
  "CSNum": 2,
  "DataFileNum": 1,
  "BeginDataFileSeq": 1,
  "LastDataFileSeq": 1,
  "LastExtentID": 24,
  "DataSize": 3725093,
  "ThinDataSize": 534118400,
  "CompressedDataSize": 74816015,
  "CompressedRatio": 0,
  "CompressionType": "snappy",
  "LastLSN": 6645140616,
  "LastLSNCode": -1200110631,
  "HasError": false
}
...
```

