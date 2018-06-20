##语法##
***db.createSpareRG()***

增加备份组，用于管理备份节点。

##参数描述##
无
##返回值##

返回备份组的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

- 创建一个备份组

 ```lang-javascript
 > db.createSpareRG()
 ```
