[^_^]:
    变更流

deletecs 删除集合空间操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "deletecs",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
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
| Description | bson | 删除集合空间的选项，如回收站项目信息等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c83e00000000100000000",
  "Type": "change",
  "ChangeType": "deletecs",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Description": {
    "RecycleItem": {
      "RecycleName": "SYSRECYCLE_4_3",
      "RecycleID": 4,
      "OriginName": "foo",
      "OriginID": 3,
      "Type": "CollectionSpace",
      "OpType": "Drop",
      "RecycleTime": "2023-07-17-10.11.28.600000",
      "Comment": ""
    }
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.11.28.623000"
    }
  }
}
```