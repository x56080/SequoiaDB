[^_^]:
    变更流

alter 修改集合空间、集合属性操作，格式如下：

- 修改集合空间属性

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "alter",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Description:
    {
        <alter options>
    },
    ...
}
```

- 修改集合属性

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "alter",
    ChangeFlags: "",
    CollectionSpace: "<collection space name>",
    Collection: "<collection full name>",
    Description:
    {
        <alter options>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| Description | bson | 修改集合空间、集合的选项 |

例子：

- 修改集合空间属性

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c8f280000000100000000",
  "Type": "change",
  "ChangeType": "alter",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Description": {
    "AlterType": "collection space",
    "Version": 1,
    "Name": "foo",
    "Options": {
      "IgnoreException": false
    },
    "Alter": {
      "Name": "set attributes",
      "Args": {
        "LobPageSize": 8192
      }
    },
    "AlterInfo": {}
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.23.28.881000"
    }
  }
}
```

- 修改集合属性

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c92700000000100000000",
  "Type": "change",
  "ChangeType": "alter",
  "ChangeFlags": "",
  "CollectionSpace": "foo",
  "Collection": "foo.bar",
  "Description": {
    "AlterType": "collection",
    "Version": 1,
    "Name": "foo.bar",
    "Options": {
      "IgnoreException": false
    },
    "Alter": {
      "Name": "set attributes",
      "Args": {
        "Compressed": false
      }
    },
    "AlterInfo": {}
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.23.55.488000"
    }
  }
}
```