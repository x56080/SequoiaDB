##描述##

会话列表 $LIST_SESSION 列出当前数据库节点中所有的用户与系统会话，每一个会话为一条记录。

##标示##

$LIST_SESSION

##字段信息##

| 字段名    | 类型         | 描述                                   |
| --------- | ------------ | -------------------------------------- |
| NodeName  | 字符串       | 会话所在的节点                         |
| SessionID | 整型或长整型 | 会话 ID                                |
| TID       | 整型         | 该会话所对应的系统线程 ID              |
| Status    | 字符串       | 会话状态<br>- Creating：创建状态<br>- Running：运行状态<br>- Waiting：等待状态<br>- Idle：线程池待机状态<br>- Destroying：销毁状态 |
| Type      | 字符串       | [EDU 类型](manual/Distributed_Engine/Architecture/Thread_Model/edu.md)  |
| Name      | 字符串       | EDU 名，一般系统 EDU 名为空            |
| Source            | 字符串        | 会话来源信息，该字段仅在与 SQL 实例相关的会话中有值 |
| RelatedID | 字符串       | 会话的内部标识                         |

##示例##

```lang-javascript
> db.exec( "select * from $LIST_SESSION" )
{
  "NodeName": "hostname:41000",
  "SessionID": 4,
  "TID": 23272,
  "Status": "Waiting",
  "Type": "DpsRollback",
  "Name": "",
  "Source": "",
  "RelatedID": "c0a8143ea02800005ae8"
}
{
  "NodeName": "hostname:41000",
  "SessionID": 5,
  "TID": 23273,
  "Status": "Running",
  "Type": "Task",
  "Name": "PAGEMAPPING-JOB-D",
  "Source": "",
  "RelatedID": "c0a8143ea02800005ae9"
}
...
```
