[^_^]:
    变更流

renamecl 重命名集合操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "renamecl",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    Description:
    {
        NewName: "<new collection name>"
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 重命名集合的选项，新名字等 |
| Description.NewName | string | 集合的新名字 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c8e1c0000000100000000",
  "Type": "change",
  "ChangeType": "renamecl",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "NewName": "foo.new"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.22.52.344000"
    }
  }
}
```