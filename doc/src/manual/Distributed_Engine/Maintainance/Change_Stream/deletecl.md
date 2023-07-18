[^_^]:
    变更流

deletecl 删除集合操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "deletecl",
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
| Description | bson | 删除集合的选项，如回收站项目信息等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c7b180000000100000000",
  "Type": "change",
  "ChangeType": "deletecl",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "RecycleItem": {
      "RecycleName": "SYSRECYCLE_3_8589934593",
      "RecycleID": 3,
      "OriginName": "foo.bar",
      "OriginID": 8589934593,
      "Type": "Collection",
      "OpType": "Drop",
      "RecycleTime": "2023-07-17-10.11.15.065000",
      "Comment": ""
    }
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.11.15.092000"
    }
  }
}
```