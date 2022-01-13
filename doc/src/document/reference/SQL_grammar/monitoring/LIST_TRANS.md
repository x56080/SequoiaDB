##描述##

[事务](basic_operation/transaction.md)列表 $LIST_TRANS 列出数据库中正在进行的事务信息。

每一个数据节点上正在进行的每一个事务为一条记录。

##标示##

$LIST_TRANS

##字段信息##

字段说明可参考[事务列表](database_management/monitoring/list/SDB_LIST_TRANSACTIONS.md)。

##示例##

```lang-javascript
> db.exec("select * from $LIST_TRANS")
{
  "NodeName": "sdbserver:42000",
  "GroupName": "db2",
  "SessionID": 20,
  "TransactionID": "00040000000003",
  "TransactionIDSN": 3,
  "IsRollback": false,
  "CurrentTransLSN": 3314225876,
  "BeginTransLSN": 3314225744,
  "WaitLock": {},
  "TransactionLocksNum": 3,
  "IsLockEscalated": false,
  "UsedLogSpace": 100,
  "ReservedLogSpace": 116,
  "RelatedID": "c0a8143ec35000005f33"
}
```
