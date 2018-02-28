##描述##

上下文快照 SDB_SNAP_CONTEXTS 列出当前数据库节点中所有的会话所对应的上下文。

每一个会话为一条记录，如果一个会话中包括一个或一个以上的上下文时，其 Contexts 数组字段对每个上下文产生一个对象。

>   **Note:**
>
>   快照操作自身需产生一个上下文，因此结果集中至少会返回一个当前快照的上下文信息。

##标示##

SDB_SNAP_CONTEXTS

##字段信息###

| 字段名                  | 类型   | 描述                                     |
| ----------------------- | ------ | ---------------------------------------- |
| NodeName                | 字符串 | 节点名（主机名：端口号）                 |
| SessionID               | 长整型 | 会话 ID                                  |
| Contexts.ContextID      | 长整型 | 上下文 ID                                |
| Contexts.Type           | 字符串 | 上下文类型，如：DUMP                     |
| Contexts.Description    | 字符串 | 上下文的描述信息，如：包含当前的查询条件 |
| Contexts.DataRead       | 长整型 | 所读数据                                 |
| Contexts.IndexRead      | 长整型 | 所读索引                                 |
| Contexts.QueryTimeSpent | 浮点数 | 查询总时间（单位：秒）                   |
| Contexts.StartTimestamp | 时间戳 | 创建时间                                 |

###示例###

```lang-javascript
> db.snapshot( SDB_SNAP_CONTEXTS )
{
  "NodeName": "hostname1:11820",
  "SessionID": 28,
  "Contexts": [
    {
      "ContextID": 12,
      "Type": "DUMP",
      "Description": "BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2013-09-27-18.06.37.079570"
    }
  ]
}
```
