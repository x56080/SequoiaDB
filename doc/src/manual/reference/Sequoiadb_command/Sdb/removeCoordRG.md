##语法##
***db.removeCoordRG()***

删除数据库中协调分区组。原则上会把该分区组的所有协调节点都删除，但如果在删除这些节点过程中，先把db对象所连接上的协调节点删除，则有可能会遗留部分协调节点未删除，需要使用Oma类的removeCoord方法删除遗留的协调节点。

##参数描述##
无

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 删除协调分区组

	```lang-javascript
	> db.removeCoordRG()
	```
