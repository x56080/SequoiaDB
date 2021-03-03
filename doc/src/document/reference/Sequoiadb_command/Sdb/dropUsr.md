##语法##
***db.dropUsr( \<name\>, \<password\> )***

删除数据库已有的用户名和密码。

##参数描述##

| 参数名 | 参数类型 | 描述 | 是否必填 |
| ------ | ------ | ------ | ------ |
| name | string | 用户名 | 是 |
| password | string | 密码 | 是 |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

* 删除用户名为 root，密码为 admin 的数据库权限。

 ```lang-javascript
 > db.dropUsr( "root", "admin" )
 ```
