[^_^]:
    变更流

invalidatecata 清空编目缓存操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "invalidatecata",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    Description:
    {
        <invalidate options>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 清空编目缓存的选项，如清空类型等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c92300000000100000000",
  "Type": "change",
  "ChangeType": "invalidatecata",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "Type": "all"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.23.55.996000"
    }
  }
}
```