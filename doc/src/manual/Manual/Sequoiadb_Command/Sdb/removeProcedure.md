##名称##

removeProcedure - 删除指定的函数名

##语法##

**db.removeProcedure( \<function name\> )**

##类别##

Sdb

##描述##

该函数用于删除指定的函数名，函数名必须存在，否则出现异常信息。

##参数##

| 参数名 		| 参数类型 	| 描述 			| 是否必填 	|
| ------ 		| ------ 	| ------ 		| ------ 	|
| function name | 字符串 	| 函数名 		| 是 		|

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`removeProcedure()`函数常见异常如下

| 错误码 		| 可能的原因 	| 解决方法										|
| ------ 		| ------ 		| ------										|
| -233			| 存储过程不存在| 使用命令行[listProcedures()](manual/Manual/Sequoiadb_Command/Sdb/listProcedures.md) 确认存储过程名是否存在	|
	
当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 删除 sum 函数

	```lang-javascript
	> db.removeProcedure("sum")
	```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md