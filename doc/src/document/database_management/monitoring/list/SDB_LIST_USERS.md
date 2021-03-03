##描述##

用户列表 SDB_LIST_USERS 列出当前集群中的所有用户信息。

##标示##

SDB_LIST_USERS

##字段信息##

| 字段名             | 类型   | 描述               |
| ----------------  | ------ | ------------------ |
| User              | 字符串  | 用户名             |
| Options.AuditMask | 字符串  | 用户审计日志配置掩码 |

##示例##

```lang-javascript
> db.list( SDB_LIST_USERS )
{
  "User": "admin",
  "Options": {}
}
{
  "User": "user2",
  "Options": {
    "AuditMask": "DDL|DML|!DQL"
  }
}
```
