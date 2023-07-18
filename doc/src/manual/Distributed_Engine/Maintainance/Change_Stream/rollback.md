[^_^]:
    变更流

rollback 回滚事务操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "rollback",
    ChangeFlags: "",
    TransInfo:
    {
        TransID: <transaction ID>,
        TransAttr: <transaction attributes>
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| TransInfo | bson | 回滚事务的信息 |
| TransInfo.TransID | string | 事务 ID |
| TransInfo.TransAttr | string | 事务的属性，如自动提交等 |

例子：

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299cab780000000200000000",
  "Type": "change",
  "ChangeType": "rollback",
  "ChangeFlags": "",
  "TransInfo": {
    "TransID": "0x00020015e8951a",
    "TransAttr": "Rollback"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-11.39.19.379000"
    }
  }
}
```