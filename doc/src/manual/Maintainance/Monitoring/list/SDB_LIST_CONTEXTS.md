##描述##

上下文列表 SDB_LIST_CONTEXTS 列出当前数据库节点中所有的会话所对应的上下文。

每一个会话为一条记录，如果一个会话中包括一个或一个以上的上下文时，其 Contexts 数组字段对每个上下文产生一个对象。

>   **Note:**
>
>   列表操作自身需产生一个上下文，因此结果集中至少会返回一个当前列表的上下文信息。

##标示##

SDB_LIST_CONTEXTS

##字段信息##

| 字段名     | 类型       | 描述                                           |
| ---------- | ---------- | ---------------------------------------------- |
| NodeName   | 字符串     | 上下文所在的节点                               |
| SessionID  | 长整型     | 会话 ID                                        |
| TotalCount | 整型       | 上下文列表长度                                 |
| Contexts   | 长整型数组 | 上下文 ID 数组，为该会话所包含的所有上下文列表 |

##示例##

```lang-javascript
> db.list( SDB_LIST_CONTEXTS )
{
  "NodeName": "hostname1:11820",
  "SessionID": 21,
  "TotalCount": 1,
  "Contexts": [
    182
  ]
}
...
```
