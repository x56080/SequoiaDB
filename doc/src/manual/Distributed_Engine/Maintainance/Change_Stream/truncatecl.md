[^_^]:
    变更流

truncate 清空集合操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "truncatecl",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    Description:
    {
        <truncate options>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 清除数据的选项，如回收站项目信息等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000000008cc0000000100000000",
  "Type": "change",
  "ChangeType": "truncatecl",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "RecycleItem": {
      "RecycleName": "SYSRECYCLE_1_4294967297",
      "RecycleID": 1,
      "OriginName": "foo.bar",
      "OriginID": 4294967297,
      "Type": "Collection",
      "OpType": "Truncate",
      "RecycleTime": "2023-07-17-09.41.46.524000",
      "Comment": ""
    }
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.41.46.845000"
    }
  }
}
```