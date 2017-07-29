##描述##

事务快照 SDB_LIST_TRANSACTIONS 列出数据库中正在进行的事务信息。

每一个数据节点上正在进行的每一个事务为一条记录。

>   **Note:**
>
>   默认情况下，事务功能是关闭的。
>
>   如要打开事务功能需要在节点的配置文件中配置参数：transactionon = TRUE；在创建数据节点时，增加 JSON 类型的参数：{ "transactionon": "YES" } 或 { "transactionon": true }。
>
>   请参考：[事务](basic_operation/transaction.md)

##标示##

SDB_LIST_TRANSACTIONS

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
| recordID     | 整型 | 锁对象所在记录的 ID       |
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
> db.list(SDB_LIST_TRANSACTIONS)
{
  "NodeName": "susetzb:20000",
  "GroupName": "db1",
  "SessionID": 80,
  "TransactionID": "03e80000000022",
  "IsRollback": false,
  "CurrentTransLSN": 1893229680,
  "WaitLock": {
    "CSID": -1,
    "CLID": 65535,
    "recordID": -1,
    "recordOffset": -1
  },
  "TransactionLocksNum": 3,
  "RelatedID": "c0a8142ac35000001d50"
}
{
  "NodeName": "susetzb:42000",
  "GroupName": "db2",
  "SessionID": 112,
  "TransactionID": "03eb0000000025",
  "IsRollback": false,
  "CurrentTransLSN": 4629597700,
  "WaitLock": {
    "CSID": -1,
    "CLID": 65535,
    "recordID": -1,
    "recordOffset": -1
  },
  "TransactionLocksNum": 3,
  "RelatedID": "c0a8142ac35000001fd2"
}
Return 2 row(s).
```
