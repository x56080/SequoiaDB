[^_^]:
    变更流

renamecs 重命名集合空间操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "renamecs",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Description:
    {
        NewName: "<new collection space name>"
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 重命名集合空间的选项，新名字等 |
| Description.NewName | string | 集合空间的新名字 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c8a280000000100000000",
  "Type": "change",
  "ChangeType": "renamecs",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Description": {
    "NewName": "new"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.21.23.947000"
    }
  }
}
```