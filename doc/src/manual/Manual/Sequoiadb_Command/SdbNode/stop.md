
##语法##
***node.stop()***

停止当前节点。

##参数描述##

无

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

* 停止 node 节点

 ```lang-javascript
 > node.stop()
 ```
