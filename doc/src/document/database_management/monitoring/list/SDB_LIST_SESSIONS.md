##描述##

会话列表 SDB_LIST_SESSIONS 列出当前数据库节点中所有的用户与系统会话，每一个会话为一条记录。

##标示##

SDB_LIST_SESSIONS

##字段信息##

| 字段名    | 类型         | 描述                                   |
| --------- | ------------ | -------------------------------------- |
| NodeName  | 字符串       | 会话所在的节点                         |
| SessionID | 整型或长整型 | 会话 ID                                |
| TID       | 整型         | 该会话所对应的系统线程 ID              |
| Status    | 字符串       | 会话状态<br>- Creating：创建状态<br>- Running：运行状态<br>- Waiting：等待状态<br>- Idle：线程池待机状态<br>- Destroying：销毁状态 |
| Type      | 字符串       | [EDU 类型](database_management/EDU.md) |
| Name      | 字符串       | EDU 名，一般系统 EDU 名为空            |
| RelatedID | 字符串       | 会话的内部标识                         |

##示例##

```lang-javascript
> db.list( SDB_LIST_SESSIONS )
{
  "NodeName": "hostname1:11820",
  "SessionID": 1,
  "TID": 6168,
  "Status": "Running",
  "Type": "TCPListener",
  "Name": "",
  "RelatedID": "7f000101a41000006d9c"
}
{
  "NodeName": "hostname1:11820",
  "SessionID": 2,
  "TID": 6169,
  "Status": "Running",
  "Type": "HTTPListener",
  "Name": "",
  "RelatedID": "7f0001019c4000006dd9"
}
...
{
  "NodeName": "hostname1:11820",
  "SessionID": 21,
  "TID": 6691,
  "Status": "Running",
  "Type": "ShardAgent",
  "Name": "hostname1:11821",
  "RelatedID": "7f0001019c4000006dd9"
}
```
