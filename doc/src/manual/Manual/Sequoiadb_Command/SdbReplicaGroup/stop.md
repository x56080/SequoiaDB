
##语法##
***rg.stop()***

停止当前复制组。

> **Note:**  
> 停止后将不能执行创建节点等相关操作，可以通过 [rg.start](manual/Manual/Sequoiadb_Command/SdbReplicaGroup/start.md) 启动节点后操作。

##返回值##

无返回值，出错抛异常，并输出错误信息。可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息，通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##

[错误码](manual/Manual/Sequoiadb_error_code.md)

##示例##

停止 group1 复制组

```lang-javascript
> var rg = db.getRG( "group1" )
> rg.stop()
```
