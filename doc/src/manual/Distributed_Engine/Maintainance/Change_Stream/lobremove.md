[^_^]:
    变更流

lobremove 删除大对象操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "lobremove",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    DocumentKey: <lob oid>,
    Description:
    {
      Sequence: <sequence>,
      PageSize: <page size>,
      PageOffset: <offset>,
      FileOffset: <offset>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| DocumentKey | oid | 大对象的主键 |
| Description | bson | 大对象的写入信息 |
| Description.Sequence | int32 | 大对象的分片号 |
| Description.PageSize | int32 | 大对象的分片大小 |
| Description.PageOffset | int32 | 删除的数据在大对象的分片中的偏移 |
| Description.FileOffset | int64 | 删除的数据在大对象中的偏移 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000297030e00000000100000000",
  "Type": "change",
  "ChangeType": "lobremove",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "DocumentKey": {
    "$oid": "000064b50faa3300024a4148"
  },
  "Description": {
    "Sequence": 0,
    "PageSize": 262144,
    "PageOffset": 1024,
    "FileOffset": 0
  }
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.56.24.415000"
    }
  }
}
```