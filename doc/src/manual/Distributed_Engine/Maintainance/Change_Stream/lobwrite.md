[^_^]:
    变更流

lobwrite 写大对象操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "lobwrite",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    DocumentKey: <lob oid>,
    Description:
    {
      Sequence: <sequence>,
      PageSize: <page size>,
      PageOffset: <offset>,
      FileOffset: <offset>,
      Length: <length>,
      Data: <data>,
      MetaData: <metadata>
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
| Description.PageOffset | int32 | 写入的数据在大对象的分片中的偏移 |
| Description.FileOffset | int64 | 写入的数据在大对象中的偏移 |
| Description.Length | int32 | 写入的数据的长度 |
| Description.Data | binary | 写入的数据 |
| Description.MetaData | bson | 写入的数据的元数据 |

例子：

```lang-javascript
{
  "Token": "00010000000003e800000000000000000000000014cedd380000000100000000",
  "Type": "change",
  "ChangeType": "lobwrite",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "DocumentKey": {
    "$oid": "000064b50faa3300024a4148"
  },
  "Description": {
    "Sequence": 0,
    "PageSize": 262144,
    "MetaData": {
      "Size": 3,
      "CreateTime": {
        "$timestamp": "2023-07-17-09.53.51.095000"
      },
      "ModificationTime": {
        "$timestamp": "2023-07-17-09.53.51.095000"
      },
      "Available": true,
      "HasPiecesInfo": false
    },
    "PageOffset": 1024,
    "FileOffset": 0,
    "Length": 3,
    "Data": {
      "$binary": "YWFh",
      "$type": "0"
    }
  }
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.53.51.438000"
    }
  }
}
```