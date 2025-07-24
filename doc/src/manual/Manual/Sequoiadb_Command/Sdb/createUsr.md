##名称##

createUsr - 创建数据库用户

##语法##

<<<<<<< HEAD
**db.createUsr( \<name\>, \<password\>, [options] )**

**db.createUsr( \<User\>, [options] )**

**db.createUsr( \<CipherUser\>, [options] )**
=======
**db.createUsr(\<name\>, \<password\>, [options])**

**db.createUsr(\<User\>, [options])**

**db.createUsr(\<CipherUser\>, [options])**
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

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

<<<<<<< HEAD
###options取值###
=======
###options 取值###
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

| 选项名称  | 取值类型   |    描述   |
| --------- | ---------- | --------- |
| AuditMask | String     | 用户[审计日志][auditlog]的配置掩码，默认值为"SYSTEM\|DDL\|DCL"，取值如下：<br>ACCESS、CLUSTER、SYSTEM、DCL、DDL、DML、DQL、INSERT、UPDATE、DELETE、OTHER、ALL、NONE<br>● 支持使用按位或（\|）连接多个掩码，逻辑非（\!）禁止某个掩码<br>● 取值"ALL"表示选择全部配置掩码<br>● 取值"NONE"表示禁止全部配置掩码，即关闭审计功能 |
<<<<<<< HEAD
| Role      | String     | 旧版本的用户角色，只支持系统内置角色，取值列表："admin"、"monitor"。"admin" 为管理员角色，可执行任何操作；"monitor" 为监控角色，只能执行 snapshot 和 list 操作 |
| Roles     | Array      | 用户角色列表，可以为用户授予多个角色，详情请参考[Role-based Access Control][rbac] |
=======
| Role      | String     | 用户角色，当前只支持系统内置角色，默认值为 "admin"，取值列表："admin"、"monitor"。"admin" 为管理员角色，可执行任何操作；"monitor" 为监控角色，只能执行 snapshot 和 list 操作 |
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

> **Note:**
>
> - 该接口只能用于集群模式。
> - 当数据库创建了用户，连接数据库必须指定用户名和密码。
> - 数据库用户名和密码的限制请参考[数据库限制][databast_limite]。
<<<<<<< HEAD
=======
> - 数据库中创建的第一个用户必须是"admin"角色。
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][faq]。

##版本##

v2.0 及以上版本

##示例##

* 创建用户名为 sdbadmin，密码为 sdbadmin 的用户，并设置审计日志掩码。

    ```lang-javascript
<<<<<<< HEAD
    > db.createUsr( "sdbadmin", "sdbadmin", { AuditMask: "DDL|DML|!DQL" } )
    ```

* 使用 User 对象创建用户名为 sdbadmin，密码为 sdbadmin 的用户。

    ```lang-javascript
    > var a = User( "sdbadmin", "sdbadmin" )
    > db.createUsr( a )
=======
    > db.createUsr("sdbadmin", "sdbadmin", {AuditMask: "DDL|DML|!DQL"})
    ```
 
* 使用 User 对象创建用户名为 sdbadmin，密码为 sdbadmin 的用户。

    ```lang-javascript
    > var a = User("sdbadmin", "sdbadmin")
    > db.createUsr(a)
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    ```

* 使用 CipherUser 对象创建用户名为 sdbadmin，密码为 sdbadmin 的用户（密文文件中必须存在用户名为 sdbadmin，密码为 sdbadmin 的用户信息，关于如何在密文文件中添加删除密文信息，详细可见 [sdbpasswd][passwd]）。

    ```lang-javascript
<<<<<<< HEAD
    > var a = CipherUser( "sdbadmin" )
    > db.createUsr( a )
=======
    > var a = CipherUser("sdbadmin")
    > db.createUsr(a)
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
    ```


[^_^]:
     本文使用的所有引用及链接

[user]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/User.md
[cipherUser]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/CipherUser.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[passwd]:manual/Distributed_Engine/Maintainance/Mgmt_Tools/sdbpasswd.md
[databast_limite]:manual/Manual/sequoiadb_limitation.md#数据库
<<<<<<< HEAD
[auditlog]:manual/Distributed_Engine/Maintainance/DiagLog/auditlog.md
[rbac]: manual/Distributed_Engine/Maintainance/Security/Role_Based_Access_Control/Readme.md
=======
[auditlog]:manual/Distributed_Engine/Maintainance/DiagLog/auditlog.md
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
