##语法##
***db.traceOff( [dumpFile] )***

关闭数据库引擎跟踪功能，并将跟踪情况导出二进制文件，如：/opt/sequoiadb/trace.dump

##参数描述##

| 参数名 		| 参数类型 	| 描述 				| 是否必填 	|
| ------ 		| ------ 	| ------ 			| ------ 	|
| dumpFile 		| string 	| dump 的文件名称	| 否 		|

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 关闭数据库引擎跟踪 /opt/sequoiadb/trace.dump

	```lang-javascript
	> db.traceOff("/opt/sequoiadb/trace.dump")
	```