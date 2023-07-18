[^_^]:
    变更流

createcl 创建集合操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "createcl",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    Description:
    {
        <create options>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 创建集合的选项，唯一标识、压缩属性等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e80000000000000000000000000000005c0000000100000000",
  "Type": "change",
  "ChangeType": "createcl",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "UniqueID": 4294967297,
    "Attribute": "Compressed",
    "CompressionType": "lzw",
    "IdIndex": {
      "name": "$id",
      "_id": {
        "$oid": "64b49a2b57e9964f1a7bd7df"
      },
      "UniqueID": 4294967296,
      "key": {
        "_id": 1
      },
      "v": 0,
      "unique": true,
      "dropDups": false,
      "enforced": true,
      "NotNull": false,
      "NotArray": true,
      "Global": false,
      "Standalone": false
    }
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.32.27.076000"
    }
  }
}
```