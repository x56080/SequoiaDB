##语法##

***db.dropUsr( \<name\>, \<password\> )***

***db.dropUsr( \<User\> )***

***db.dropUsr( \<CipherUser\> )***

删除数据库用户。

##参数描述##

| 参数名     | 参数类型 | 描述            | 是否必填 |
| ---------- | -------- | --------------- | -------- |
| name       | string   | 用户名          | 是       |
| password   | string   | 密码            | 是       |
| User       | object   | [User](reference/Sequoiadb_command/AuxiliaryObjects/User.md)对象       | 是       |
| CipherUser | object   | [CipherUser](reference/Sequoiadb_command/AuxiliaryObjects/CipherUser.md)对象 | 是       |

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md) 。

##示例##

* 删除用户名为 sdbadmin，密码为 sdbadmin 的用户。

 ```lang-javascript
 > db.dropUsr( "sdbadmin", "sdbadmin" )
 ```

* 使用 User 对象删除用户名为 sdbadmin，密码为 sdbadmin 的用户。

 ```lang-javascript
 > var a = User( "sdbadmin", "sdbadmin" )
 > db.dropUsr( a )
 ```

* 使用 CipherUser 对象删除用户名为 sdbadmin，密码为 sdbadmin 的用户（密文文件中必须存在用户名为 sdbadmin，密码为 sdbadmin 的用户信息，关于如何在密文文件中添加删除密文信息，详细可见[sdbpasswd](database_management/tools/sdbpasswd.md)）。

 ```lang-javascript
 > var a = CipherUser( "sdbadmin" )
 > db.dropUsr( a )
 ```