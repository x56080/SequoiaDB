[^_^]:
    变更流

return 恢复回收站项目操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "return",
    ChangeFlags: "",
    Description:
    {
        <return options>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 恢复回收站项目的选项，如回收站项目信息等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c9d600000000100000000",
  "Type": "change",
  "ChangeType": "return",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "RecycleItem": {
      "RecycleName": "SYSRECYCLE_5_17179869185",
      "RecycleID": 5,
      "OriginName": "foo.bar",
      "OriginID": 17179869185,
      "Type": "Collection",
      "OpType": "Drop",
      "RecycleTime": "2023-07-17-10.51.58.351000",
      "Comment": ""
    }
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.52.23.523000"
    }
  }
}
```