[^_^]:
    数据流列表

数据流列表可以列出当前集群中所有正在运行的数据流的信息。

##标识##

SDB_LIST_STREAMS

##字段信息##

| 字段名 | 类型 | 描述 |
| ---- | ---- | ---- |
| NodeName | string | 所在的节点名 |
| GroupName | string | 所在的复制组名 |
| Type | string | 数据流类型，取值如下：<br>"change"：变更流 |
| SessionID | int64 | 执行数据流的会话的唯一标识 |
| ContextID | int64 | 执行数据流的上下文的唯一标识 |
| Options | bson | 数据流的配置参数 |
| CurToken | string | 数据流的当前位置 |

##示例##

查看数据流列表

```lang-javascript
> db.list(SDB_LIST_STREAMS)
```

输出结果如下：

```lang-json
{
  "NodeName": "sdbserver:20000",
  "GroupName": "group1",
  "Type": "change",
  "SessionID": 26,
  "ContextID": 2,
  "Options": {
    "Token": "",
    "Collections": [
      "foo.bar"
    ],
    "CollectionSpaces": []
  },
  "CurToken": "00010000000003e80000000000000000000000000000013c0000000100000000"
}
```

[^_^]:
    本文使用的所有引用及链接