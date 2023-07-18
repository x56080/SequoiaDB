[^_^]:
    变更流

createcs 创建集合空间操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType："createcs",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
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
| Description | bson | 创建集合空间的选项，唯一标识、数据页大小等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000000000000000000100000000",
  "Type": "change",
  "ChangeType": "createcs",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Description": {
    "UniqueID": 1,
    "PageSize": 65536,
    "LobPageSize": 262144,
    "Type": "NORMAL"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-09.32.27.782000"
    }
  }
}
```