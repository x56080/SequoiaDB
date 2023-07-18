[^_^]:
    变更流

deleteix 删除索引操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "deleteix",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    Description:
    {
        <drop options>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 删除索引的选项，索引名、索引定义等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c88900000000100000000",
  "Type": "change",
  "ChangeType": "deleteix",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "_id": {
      "$oid": "64b4a43e57e9964f1a7bd7fb"
    },
    "UniqueID": 17179869185,
    "key": {
      "a": 1
    },
    "name": "a"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.18.59.031000"
    }
  }
}
```