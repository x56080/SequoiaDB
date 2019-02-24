##语法##
***db.createUsr( \<name\>, \<password\>, [options] )***

为数据库创建数据库用户名和密码，防止非法用户对数据库进行非法操作。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name   | string | 用户名 | 是 |
| password | string | 密码 | 是 |
| options| Json   | 扩展选项 | 否 |

###options取值###
| 选项名称  | 取值类型   |    描述   |
| --------- | ---------- | --------- |
| AuditMask | String     | 用户审计日志配置掩码，取值列表：ACCESS、CLUSTER、SYSTEM、DML、DDL、DCL、DQL、INSERT、DELETE、UPDATE、OTHER；ALL表示全部开启，NONE表示全部不开启；可以使用‘\|’连接多个取值。当某位掩码未配置时，则继承节点相应的配置掩码；也可以通过 ‘!’ 来禁止某位掩码的继承 |

> **Note:**
>
> * 当为数据库设置了用户名和密码时，只能使用正确的用户名和密码才能登录数据库进行相关操作。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

* 为数据库创建数据库用户名为 admin，密码为 admin 的命令如下：

 ```lang-javascript
 > db.createUsr( "admin", "admin" )
 ```

* 使用鉴权连接到节点

 ```lang-javascript
 > var db = new Sdb("localhost", 11810, "admin", "admin")
 ```

* 创建用户，并设置审计日志

 ```lang-javascript
 > db.createUsr( "user2", "user2", {AuditMask:"DDL|DML|!DQL"} )
 ```
