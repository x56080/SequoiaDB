##描述##

服务任务列表 SDB_LIST_SVCTASKS 列出当前数据库节点中所有的服务任务。

##标示##

SDB_LIST_SVCTASKS

##字段信息##

| 字段名    | 类型         | 描述                                   |
| --------- | ------------ | -------------------------------------- |
| NodeName  | 字符串       | 任务所在的节点                         |
| TaskID | 整型       |  任务ID                        |
| TaskName      | 字符串       | 任务名称           |

##示例##

```lang-javascript
> db.list(SDB_LIST_SVCTASKS)
{
  "NodeName": "hostname:30000",
  "TaskID": 0,
  "TaskName": "Default"
}
{
  "NodeName": "hostname:30010",
  "TaskID": 0,
  "TaskName": "Default"
}
...
```
