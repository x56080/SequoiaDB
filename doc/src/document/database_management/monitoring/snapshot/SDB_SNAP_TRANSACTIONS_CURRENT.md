##描述##

事务快照 SDB_SNAP_TRANSACTIONS_CURRENT 列出当前会话在数据库中正在进行的事务信息。

当前会话在每一个数据节点上正在进行的事务为一条记录。（一般每个会话在每个数据节点上只有一个事务记录）

>   **Note:**
>
>   默认情况下，事务功能是关闭的。
>
>   如要打开事务功能需要在节点的配置文件中配置参数：transactionon = TRUE；在创建数据节点时，增加 JSON 类型的参数：{ "transactionon": "YES" } 或 { "transactionon": true }。
>
>   请参考：[事务](basic_operation/transaction.md)

##标示##

SDB_SNAP_TRANSACTIONS_CURRENT

###字段信息###

| 字段名                 | 类型     | 描述                                     |
| ---------------------- | -------- | ---------------------------------------- |
| NodeName               | 字符串   | 节点名（主机名：端口号）                 |
| SessionID              | 长整型   | 会话 ID                                  |
| TransactionID          | 字符串   | 事务 ID                                  |
| IsRollback             | 布尔型   | 表示这个事务是否处于回滚中               |
| CurrentTransLSN        | 长整型   | 事务当前的日志LSN                        |   
| WaitLock               | BSON对象 | 正在等待的锁                             |
| TransactionLocksNum    | 整型     | 事务已经获得的锁                         |
| RelatedID              | 字符串   | 内部标示                                 |
| GotLocks               | BSON数组 | 事务已经获得的锁列表                     |

###锁对象信息###

WaitLock 和 GetLocks 字段中锁对象的信息：

| 字段名       | 类型 | 描述                     |
| ------------ | ---- | ------------------------ |
| CSID         | 整型 | 锁对象所在集合空间的 ID  |
| CLID         | 整型 | 锁对象所在集合的 ID      |
| recordID     | 整型 | 锁对象所在记录的ID       |
| recordOffset | 整型 | 锁对象所在记录的偏移量   |

###锁对象的描述###

锁对象每个字段取值不同表示不同的锁对象：

| 锁对象       | CSID | CLID  | recordID | recordOffset | 备注 |
| ------------ | ---- | ----- | ---- | ---- | ------------ |
| 没有锁对象   | -1   | 65535 | -1   | -1   | 一般在WaitLock为没有锁对象时，表示当前事务没有在等待锁 |
| 集合空间锁   | >= 0 | 65535 | -1   | -1   | |
| 集合锁       | >= 0 | >= 0  | -1   | -1   | |
| 记录锁       | >= 0 | >= 0  | >= 0 | >= 0 | |

##示例##

```lang-javascript
> db.snapshot( SDB_SNAP_TRANSACTIONS_CURRENT )
{
  "NodeName": "hostname1:11820",
  "SessionID": 17,
  "TransactionID": "03e80000000002",
  "IsRollback": false,
  "CurrentTransLSN": 296,
  "WaitLock": {
    "CSID": -1,
    "CLID": 65535,
    "recordID": -1,
    "recordOffset": -1
  },
  "TransactionLocksNum": 3,
  "RelatedID": "7f000101c350000059a5",
  "GotLocks": [
    {
      "CSID": 1,
      "CLID": 0,
      "recordID": -1,
      "recordOffset": -1
    },
    {
      "CSID": 1,
      "CLID": 0,
      "recordID": 9,
      "recordOffset": 84
    },
    {
      "CSID": 1,
      "CLID": 65535,
      "recordID": -1,
      "recordOffset": -1
    }
  ]
}
{
  "NodeName": "hostname1:11830",
  "SessionID": 19,
  "TransactionID": "03ea0000000001",
  "IsRollback": false,
  "CurrentTransLSN": 124,
  "WaitLock": {
    "CSID": -1,
    "CLID": 65535,
    "recordID": -1,
    "recordOffset": -1
  },
  "TransactionLocksNum": 3,
  "RelatedID": "7f000101c350000059a5",
  "GotLocks": [
    {
      "CSID": 1,
      "CLID": 0,
      "recordID": -1,
      "recordOffset": -1
    },
    {
      "CSID": 1,
      "CLID": 0,
      "recordID": 9,
      "recordOffset": 36
    },
    {
      "CSID": 1,
      "CLID": 65535,
      "recordID": -1,
      "recordOffset": -1
    }
  ]
}
```