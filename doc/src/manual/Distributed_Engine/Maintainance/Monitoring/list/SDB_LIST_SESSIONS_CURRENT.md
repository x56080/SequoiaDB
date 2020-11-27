##描述##

当前会话列表 SDB_LIST_SESSIONS_CURRENT 列出数据库节点中的当前用户会话。

如果当前连接在协调节点上，将会返回当前会话通过协调节点连接各个数据节点或者编目节点的会话，每个数据节点或者编目节点连接产生一条记录；如果当前连接在数据节点或者编目节点上，将会返回一条记录。

##标示##

SDB_LIST_SESSIONS_CURRENT

##字段信息##

| 字段名    | 类型         | 描述                                   |
| --------- | ------------ | -------------------------------------- |
| NodeName  | 字符串       | 会话所在的节点                         |
| SessionID | 整型或长整型 | 会话 ID                                |
| TID       | 整型         | 该会话所对应的系统线程 ID              |
| Status    | 字符串       | 会话状态<br>- Creating：创建状态<br>- Running：运行状态<br>- Waiting：等待状态<br>- Idle：线程池待机状态<br>- Destroying：销毁状态 |
| Type      | 字符串       | [EDU 类型](manual/Distributed_Engine/Architecture/Thread_Model/edu.md)  |
| Name      | 字符串       | EDU 名，一般系统 EDU 名为空            |
| RelatedID | 字符串       | 会话的内部标识                         |

##示例##

```lang-javascript
> db.list( SDB_LIST_SESSIONS_CURRENT )
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
