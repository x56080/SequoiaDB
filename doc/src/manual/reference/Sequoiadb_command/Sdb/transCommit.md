##语法##
***db.transCommit()***

事务提交。在开启事务之后，如果单个逻辑工作单元执行的操作无异常，执行事务提交命令，那么数据库的数据将被更新。

##参数描述##
无

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 事务提交命令

	```lang-javascript
	> db.transCommit()
	```