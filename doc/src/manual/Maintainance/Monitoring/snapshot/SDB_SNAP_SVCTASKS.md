##描述##

服务任务快照 SDB_SNAP_SVCTASKS 列出当前数据库节点中服务任务的统计信息，输出一条记录。

##标示##

SDB_SNAP_SVCTASKS

##字段信息##

| 字段名            | 类型          | 描述                                               |
| ----------------- | ------------- | -------------------------------------------------- |
| NodeName  | 字符串       | 任务所在的节点                         |
| TaskName          | 字符串        | 任务名称                           |
| TaskID         | 整型        | 任务ID                                                  |
| Time              | 长整型        |  任务持续时间                                      |
| TotalContexts     | 长整型        | 总上下文记录数量                                   |
| TotalDataRead     | 长整型        | 数据记录读                                         |
| TotalIndexRead    | 长整型        | 索引读                                             |
| TotalDataWrite    | 长整型        | 数据记录写                                         |
| TotalIndexWrite   | 长整型        | 索引写                                             |
| TotalUpdate       | 长整型        | 总更新记录数量                                     |
| TotalDelete       | 长整型        | 总删除记录数量                                     |
| TotalInsert       | 长整型        | 总插入记录数量                                     |
| TotalSelect       | 长整型        | 总选取记录数量                                     |
| TotalRead         | 长整型        | 总数据读                                           |
| TotalWrite        | 长整型        | 总数据写                                           |
| StartTimestamp    | 字符串        | 开始时间                                           |
| ResetTimestamp    | 字符串        | 重置时间                                           |

##示例##

```lang-javascript
> db.snapshot( SDB_SNAP_SVCTASKS )
{
  "NodeName": "u1604-ljh:42000",
  "TaskID": 0,
  "TaskName": "Default",
  "Time": 4271,
  "TotalContexts": 62,
  "TotalDataRead": 0,
  "TotalIndexRead": 0,
  "TotalDataWrite": 0,
  "TotalIndexWrite": 0,
  "TotalUpdate": 0,
  "TotalDelete": 0,
  "TotalInsert": 0,
  "TotalSelect": 25,
  "TotalRead": 0,
  "TotalWrite": 0,
  "StartTimestamp": "2019-08-14-10.30.59.172628",
  "ResetTimestamp": "2019-08-14-10.30.59.172628"
}
...
```