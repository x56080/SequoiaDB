[^_^]:
    变更流

delete 删除记录操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType："delete",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    DocumentKey: <delete object>,
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| DocumentKey | oid | 删除记录的主键 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000000003ac0000000100000000",
  "Type": "change",
  "ChangeType": "delete",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "DocumentKey": {
    "_id": {
      "$oid": "64b49a31d42071e884409e6a"
    }
  },
  "TransInfo": {
    "TransID": "0x00020039b1fc6a",
    "TransAttr": ""
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.35.00.322000"
    }
  }
}
```