##语法##
***db.listTasks( [cond], [sel], [sort], [hint] )***

查看数据库所有后台任务。

##参数描述##

| 参数名 | 参数类型  | 描述        												    | 是否必填  |
| ------ | ------ 	 | -------------------------------------------------------------| ----------|
| cond   | Json 对象 | 任务过滤条件 												| 否 		|
| sel 	 | Json 对象 | 任务选择字段 												| 否 		|
| sort   | Json 对象   | 对返回的记录按选定的字段排序。1为升序；-1为降序。        	| 否 	    |
| hint 	 | Json 对象 | 保留项 														| 否 	    |

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##返回值##
返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##示例##

* 列出系统所有后台任务

	```lang-javascript
	> db.listTasks()
	```
