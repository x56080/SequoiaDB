##语法##
***db.createCoordRG()***

创建协调分区组。

##返回值##

返回协调分区组的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

- 创建一个协调分区组

 ```lang-javascript
 > db.createCoordRG()
 ```
