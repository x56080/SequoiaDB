##语法##

***db.createUsr( \<name\>, \<password\>, [options] )***

***db.createUsr( \<User\>, [options] )***

***db.createUsr( \<CipherUser\>, [options] )***

创建数据库用户，防止非法用户操作数据库。

##参数描述##

| 参数名     | 参数类型 | 描述            | 是否必填 |
| ---------- | -------- | --------------- | -------- |
| name       | string   | 用户名          | 是       |
| password   | string   | 密码            | 是       |
| User       | object   | [User](reference/Sequoiadb_command/AuxiliaryObjects/User.md)对象       | 是       |
| CipherUser | object   | [CipherUser](reference/Sequoiadb_command/AuxiliaryObjects/CipherUser.md)对象 | 是       |
| options    | Json     | 扩展选项        | 否       |

###options取值###

| 选项名称  | 取值类型   |    描述   |
| --------- | ---------- | --------- |
| AuditMask | String     | 用户审计日志配置掩码，取值列表：ACCESS、CLUSTER、SYSTEM、DML、DDL、DCL、DQL、INSERT、DELETE、UPDATE、OTHER；ALL表示全部开启，NONE表示全部不开启；可以使用‘\|’连接多个取值。当某位掩码未配置时，则继承节点相应的配置掩码；也可以通过 ‘!’ 来禁止某位掩码的继承 |

> **Note:**

> * 该接口只能用于集群模式。

> * 当数据库创建了用户，连接数据库必须指定用户名和密码。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

* 创建用户名为 sdbadmin，密码为 sdbadmin 的用户，并设置审计日志掩码。

 ```lang-javascript
 > db.createUsr( "sdbadmin", "sdbadmin", { AuditMask: "DDL|DML|!DQL" } )
 ```
 
* 使用 User 对象创建用户名为 sdbadmin，密码为 sdbadmin 的用户。

 ```lang-javascript
 > var a = User( "sdbadmin", "sdbadmin" )
 > db.createUsr( a )
 ```

* 使用 CipherUser 对象创建用户名为 sdbadmin，密码为 sdbadmin 的用户（密文文件中必须存在用户名为 sdbadmin，密码为 sdbadmin 的用户信息，关于如何在密文文件中添加删除密文信息，详细可见[sdbpasswd](database_management/tools/sdbpasswd.md)）。

 ```lang-javascript
 > var a = CipherUser( "sdbadmin" )
 > db.createUsr( a )
 ```

