
##语法##
***db.transRollback()***

事务回滚。在开启事务之后，如果单个逻辑工作单元执行的操作出现异常，执行事务回滚命令，那么数据库回到原来状态。


##参数描述##
无


##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。

关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##错误##
常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##示例##

* 事务回滚命令

	```lang-javascript
	> db.transRollback()
	```