
##语法##
***db.removeRG( \<name\> )***

删除数据库中指定的复制组，复制组名必须存在。

##参数描述##

| 参数名 | 参数类型 | 描述 											| 是否必填 	|
| ------ | -------- | ----------------------------------------------| ------ 	|
| name 	 | string	| 复制组名，同一个数据库对象中，复制组名唯一。 	| 是 		|

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##错误##
常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md))。

##示例##

* 删除名为“group”的复制组

	```lang-javascript
	> db.removeRG("group")
	```
