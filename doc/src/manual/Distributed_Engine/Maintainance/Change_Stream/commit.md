[^_^]:
    变更流

commit 提交事务操作，格式如下：

```lang-javascript
{
    Token: "<token>",
    Type: "change",
    ChangeType: "commit",
    ChangeFlags: "",
    TransInfo:
    {
        TransID: <transaction ID>,
        TransAttr: <transaction attributes>,
        TransCommitAttr: <transaction commit attr>,
        TransNodes: [ <transaction nodes> ]
    },
    ...
}
```

字段说明如下：

| 字段名 | 类型 | 描述 |
| --- | --- | --- |
| TransInfo | bson | 提交事务的信息 |
| TransInfo.TransID | string | 事务 ID |
| TransInfo.TransAttr | string | 事务的属性，如自动提交等 |
| TransInfo.TransCommitAttr | string | 事务的提交属性，如二段提交等 |
| TransInfo.TransNodes | string array | 事务涉及的节点列表，表示事务在哪些节点上执行 |

例子：

- 预提交阶段

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c93d00000000100000000",
  "Type": "change",
  "ChangeType": "commit",
  "ChangeFlags": "",
  "TransInfo": {
    "TransID": "0x00020039b1fc6b",
    "TransAttr": "",
    "TransCommitAttr": "Pre-Commit",
    "TransCommitNodes": [
      {
        "GroupID": 1000,
        "NodeID": 1000
      }
    ]
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.40.57.401000"
    }
  }
}
```

- 提交阶段

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c94400000000100000000",
  "Type": "change",
  "ChangeType": "commit",
  "ChangeFlags": "",
  "TransInfo": {
    "TransID": "0x00020039b1fc6b",
    "TransAttr": "",
    "TransCommitAttr": "Snd-Commit"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.40.57.450000"
    }
  }
}
```

- 自动提交

```lang-javascript
{
  "Token": "00010000000003e8000000000000000000000000299c95080000000100000000",
  "Type": "change",
  "ChangeType": "commit",
  "ChangeFlags": "",
  "TransInfo": {
    "TransID": "0x03e800395852f3",
    "TransAttr": "AutoCommit",
    "TransCommitAttr": "Unknown"
  },
  "TimeInfo": {
    "RealTime": {
      "$timestamp": "2023-07-17-10.41.20.632000"
    }
  }
}
```