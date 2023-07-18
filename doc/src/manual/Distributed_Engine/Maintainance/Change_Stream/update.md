[^_^]:
    变更流

update 更新记录操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType："update",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    DocumentKey: { <oid> },
    UpdateAction: { $set: ..., $unset: ..., $set_array: ... },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| DocumentKey | bson | 更新记录的主键 |
| UpdateAction | bson | 更新记录的操作 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000000002ac0000000100000000",
  "Type": "change",
  "ChangeType": "update",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "DocumentKey": {
    "_id": {
      "$oid": "64b49a31d42071e884409e6a"
    }
  },
  "UpdateAction": {
    "$set": {
      "a": 20,
      "b": 10,
      "c": 20
    }
  },
  "TransInfo": {
    "TransID": "0x00020039b1fc6a",
    "TransAttr": ""
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.33.10.182000"
    }
  }
}
```