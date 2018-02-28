##描述##

当前上下文快照 SDB_SNAP_CONTEXTS_CURRENT 列出数据库节点中，当前连接所对应的会话中的上下文。

如果当前连接在协调节点上，将会返回当前会话通过协调节点连接各个数据节点或者编目节点的上下文，每个数据节点或者编目节点连接产生一条记录；如果当前连接在数据节点或者编目节点上，将会返回一条记录。每个记录中的 Contexts 数组字段中包含当前会话中所有的上下文。

>   **Note:** 
>
>   快照操作自身需产生一个上下文，因此结果集中至少包含一个上下文。

##标示##

SDB_SNAP_CONTEXTS_CURRENT

##字段信息##

| 字段名                    | 类型     | 描述                                     |
| ------------------------- | -------- | ---------------------------------------- |
| NodeName                  | 字符串   | 节点名（主机名：端口号）                 |
| SessionID                 | 长整型   | 会话 ID                                  |
| Contexts.ContextID        | 长整型   | 上下文 ID                                |
| Contexts.Type             | 字符串   | 上下文类型，如：DUMP                     |
| Contexts.Description      | 字符串   | 上下文的描述信息，如：包含当前的查询条件 |
| Contexts.DataRead         | 长整型   | 所读数据                                 |
| Contexts.IndexRead        | 长整型   | 所读索引                                 |
| Contexts.QueryTimeSpent   | 浮点数   | 查询总时间（单位：秒）                   |
| Contexts.StartTimestamp   | 时间戳   | 创建时间                                 |

##示例##

```lang-javascript
> db.snapshot( SDB_SNAP_CONTEXTS_CURRENT )
{
  "NodeName": "hostname1:11810",
  "SessionID": 28,
  "Contexts": [
    {
      "ContextID": 13,
      "Type": "DUMP",
      "Description": "BufferSize:0",
      "DataRead": 0,
      "IndexRead": 0,
      "QueryTimeSpent": 0,
      "StartTimestamp": "2013-09-27-18.25.17.311168"
    }
  ]
}
```
