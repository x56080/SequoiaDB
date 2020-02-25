##描述##

当前[事务](basic_operation/transaction.md)列表 SDB_LIST_TRANSACTIONS_CURRENT 列出当前会话在数据库中正在进行的事务信息。

当前会话在每一个数据节点上正在进行的事务为一条记录（一般每个会话在每个数据节点上只有一个事务记录）。

##标示##

SDB_LIST_TRANSACTIONS_CURRENT

###字段信息###

| 字段名                 | 类型     | 描述                                     |
| ---------------------- | -------- | ---------------------------------------- |
| NodeName               | 字符串   | 节点名（主机名：端口号：ID）             |
| GroupName              | 字符串   | 数据组名                                 |
| SessionID              | 长整型   | 会话 ID                                  |
| TransactionID          | 字符串   | 事务 ID                                  |
| IsRollback             | 布尔型   | 表示这个事务是否处于回滚中               |
| CurrentTransLSN        | 长整型   | 事务当前的日志LSN                        |   
| WaitLock               | BSON对象 | 正在等待的锁                             |
| TransactionLocksNum    | 整型     | 事务已经获得的锁                         |
| RelatedID              | 字符串   | 内部标示                                 |

###锁对象信息###

WaitLock 字段中锁对象的信息：

| 字段名       | 类型 | 描述                     |
| ------------ | ---- | ------------------------ |
| CSID         | 整型 | 锁对象所在集合空间的 ID  |
| CLID         | 整型 | 锁对象所在集合的 ID      |
| ExtentID     | 整型 | 锁对象所在记录的 ID      |
| Offset       | 整型 | 锁对象所在记录的偏移量   |
| Mode         | 字符串 | 锁的类型，对应有"IS","IX","S","U"和"X" |
| Duration     | 整型 | 锁的持有或等待时间，单位：毫秒 |

###锁对象的描述###

锁对象每个字段取值不同表示不同的锁对象：

| 锁对象       | CSID | CLID  | ExtentID | Offset | 备注 |
| ------------ | ---- | ----- | ---- | ---- | ------------ |
| 没有锁对象   | -1   | 65535 | -1   | -1   | 一般在WaitLock为没有锁对象时，表示当前事务没有在等待锁 |
| 集合空间锁   | >= 0 | 65535 | -1   | -1   | |
| 集合锁       | >= 0 | >= 0  | -1   | -1   | |
| 记录锁       | >= 0 | >= 0  | >= 0 | >= 0 | |

##示例##

```lang-javascript
> db.list(SDB_LIST_TRANSACTIONS_CURRENT)
{
  "NodeName": "ubuntu1604-xjh:20000",
  "GroupName": "db1",
  "SessionID": 89,
  "TransactionID": "03e80000000001",
  "IsRollback": false,
  "CurrentTransLSN": -1,
  "WaitLock": {},
  "TransactionLocksNum": 3,
  "RelatedID": "c0a81457c35000006b75"
}
Return 1 row(s).
```
