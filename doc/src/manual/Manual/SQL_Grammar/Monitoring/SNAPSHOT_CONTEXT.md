##描述##

上下文快照 $SNAPSHOT_CONTEXT 列出当前数据库节点中所有的会话所对应的上下文。

每一个会话为一条记录，如果一个会话中包括一个或一个以上的上下文时，其 Contexts 数组字段对每个上下文产生一个对象。 
>   **Note:**
>
>   快照操作自身需产生一个上下文，因此结果集中至少会返回一个当前快照的上下文信息。

##标识##

$SNAPSHOT_CONTEXT

##字段信息###

| 字段名                  | 类型   | 描述                                     |
| ----------------------- | ------ | ---------------------------------------- |
| NodeName                | string | 节点名，格式为<主机名>:<端口号>                 |
| SessionID               | int64  | 会话 ID                                  |
| Contexts.ContextID      | int64  | 上下文 ID                                |
| Contexts.Type           | string | 上下文类型，如 DUMP                     |
| Contexts.Description    | string | 上下文的描述信息，如包含当前的查询条件 |
| Contexts.DataRead       | int64  | 所读数据                                 |
| Contexts.IndexRead      | int64  | 所读索引                                 |
| Contexts.QueryTimeSpent | double | 查询总时间，单位为秒                   |
| Contexts.StartTimestamp | timestamp | 创建时间                                 |

##示例##

查看上下文快照

```lang-javascript
> db.exec( "select * from $SNAPSHOT_CONTEXT" )
```

输出结果如下：

```lang-json
{
  "NodeName": "hostname:30000",
  "SessionID": 29,
  "Contexts": [
    {
      "ContextID": 143397,
      "Type": "DUMP",
      "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2019-06-03-14.47.17.144611"
    }
  ]
}
{
  "NodeName": "hostname:30010",
  "SessionID": 25,
  "Contexts": [
    {
      "ContextID": 13203,
      "Type": "DUMP",
      "Description": "IsOpened:1,IsTrans:0,HitEnd:0,BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2019-06-03-14.47.17.144643"
    }
  ]
}
...
```
