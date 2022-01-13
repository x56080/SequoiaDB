##描述##

当前[事务](basic_operation/transaction.md)列表 $LIST_TRANS_CUR 列出当前会话在数据库中正在进行的事务信息。

当前会话在每一个数据节点上正在进行的事务为一条记录（一般每个会话在每个数据节点上只有一个事务记录）。

##标示##

$LIST_TRANS_CUR

##字段信息##

字段说明可参考[当前事务列表](database_management/monitoring/list/SDB_LIST_TRANSACTIONS_CURRENT.md)。

##示例##

```lang-javascript
> db.exec("select * from $LIST_TRANS_CUR")
{
  "NodeName": "sdbserver:42000",
  "GroupName": "db2",
  "SessionID": 20,
  "TransactionID": "00040000000003",
  "TransactionIDSN": 3,
  "IsRollback": false,
  "CurrentTransLSN": 3314225744,
  "BeginTransLSN": 3314225744,
  "WaitLock": {},
  "TransactionLocksNum": 3,
  "RelatedID": "c0a8143ec35000005f33"
}
```
