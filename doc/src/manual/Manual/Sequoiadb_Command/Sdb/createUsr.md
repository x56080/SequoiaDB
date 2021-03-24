##名称##

createUsr - 创建数据库用户

##语法##

**db.createUsr( \<name\>, \<password\>, [options] )**

**db.createUsr( \<User\>, [options] )**

**db.createUsr( \<CipherUser\>, [options] )**

##类别##

Sdb

##描述##

该函数用于创建数据库用户，防止非法用户操作数据库。

##参数##

| 参数名     | 参数类型 | 描述            | 是否必填 |
| ---------- | -------- | --------------- | -------- |
| name       | string   | 用户名          | 是       |
| password   | string   | 密码            | 是       |
| User       | object   | [User][user] 对象       | 是       |
| CipherUser | object   | [CipherUser][cipherUser] 对象 | 是       |
| options    | Json     | 扩展选项        | 否       |

###options取值###

| 选项名称  | 取值类型   |    描述   |
| --------- | ---------- | --------- |
| AuditMask | String     | 用户审计日志配置掩码，取值列表：ACCESS、CLUSTER、SYSTEM、DML、DDL、DCL、DQL、INSERT、DELETE、UPDATE、OTHER；ALL表示全部开启，NONE表示全部不开启；可以使用‘\|’连接多个取值。当某位掩码未配置时，则继承节点相应的配置掩码；也可以通过 ‘!’ 来禁止某位掩码的继承 |

> **Note:**
>
> * 该接口只能用于集群模式。
> * 当数据库创建了用户，连接数据库必须指定用户名和密码。
> * 数据库用户名和密码的限制请参考[数据库限制][databast_limite]

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

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

* 使用 CipherUser 对象创建用户名为 sdbadmin，密码为 sdbadmin 的用户（密文文件中必须存在用户名为 sdbadmin，密码为 sdbadmin 的用户信息，关于如何在密文文件中添加删除密文信息，详细可见 [sdbpasswd][passwd]）。

 ```lang-javascript
 > var a = CipherUser( "sdbadmin" )
 > db.createUsr( a )
 ```


[^_^]:
     本文使用的所有引用及链接

[user]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[cipherUser]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/faq.md
[passwd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md
[databast_limite]:manual/Manual/sequoiadb_limitation.md#数据库