
##语法##
***db.getCoordRG()***

获取协调复制组的引用。

##返回值##

返回协调复制组的引用，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##示例##

- 获取协调复制组的引用

 ```lang-javascript
 > var rg = db.getCoordRG()
 ```
