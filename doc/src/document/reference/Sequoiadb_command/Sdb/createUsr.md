##语法##
***db.createUsr( \<name\>, \<password\> )***

为数据库创建数据库用户名和密码，防止非法用户对数据库进行非法操作。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 用户名 | 是 |
| password | string | 密码 | 是 |

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
