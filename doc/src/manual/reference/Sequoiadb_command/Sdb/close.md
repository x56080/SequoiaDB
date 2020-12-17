##语法##
***db.close()***

关闭数据库连接。

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

- 关闭数据库连接

 ```lang-javascript
 > var db = new Sdb( "localhost", 11810 )
 > db.close()
 ```
