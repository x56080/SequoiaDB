##语法##
***db.removeProcedure( \<function name\> )***

删除指定的函数名，函数名必须存在，否则出现异常信息。

##参数描述##

| 参数名 		| 参数类型 	| 描述 			| 是否必填 	|
| ------ 		| ------ 	| ------ 		| ------ 	|
| function name | 字符串 	| 函数名 		| 是 		|

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##
| 错误码 		| 可能的原因 	| 解决方法										|
| ------ 		| ------ 		| ------										|
| -233			| 存储过程不存在| 使用命令行[listProcedures()](reference/Sequoiadb_command/Sdb/listProcedures.md) 确认存储过程名是否存在	|
	
其他错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 删除 sum 函数

	```lang-javascript
	> db.removeProcedure("sum")
	```
