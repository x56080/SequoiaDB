[^_^]:
    变更流

insert 插入记录操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType："insert",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    DocumentKey: { <oid> },
    Document: <insert object>,
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| DocumentKey | oid | 插入记录的主键 |
| Document | bson | 插入记录的内容 |

例子：

```lang-javascript
{
  "Token": "00010000000003e800000000000000000000000000001ae00000000200000000",
  "Type": "change",
  "ChangeType": "insert",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "DocumentKey": {
    "_id": {
      "$oid": "64b3fa4d17df4f2c8d5fb48e"
    }
  },
  "Document": {
    "_id": {
      "$oid": "64b3fa4d17df4f2c8d5fb48e"
    },
    "a": 1
  },
  "TransInfo": {
    "TransID": "0x0002005584ef46",
    "TransAttr": ""
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-16-22.10.21.937000"
    }
  }
}
```