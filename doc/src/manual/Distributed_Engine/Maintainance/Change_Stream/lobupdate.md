[^_^]:
    变更流

lobupdate 更新大对象操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "lobupdate",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    DocumentKey: <lob oid>,
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| DocumentKey | bson | 大对象的主键 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000297030e00000000100000000",
  "Type": "change",
  "ChangeType": "lobupdate",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "DocumentKey": {
    "$oid": "000064b50faa3300024a4148"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.56.24.415000"
    }
  }
}
```