
##语法##
***db.waitTasks(\<id1\>,[id2],...)***

同步等待指定任务结束或取消。

##参数描述##

| 参数名 	| 参数类型 	| 描述 		| 是否必填 	|
| ------ 	| ------ 	| ------ 	| ------ 	|
| id1, id2… | 整数 		| 任务 ID 	| 是	 	|


##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。

关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##错误##
常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。


##示例##

* 同步等待数据切分任务完成

	```lang-javascript
	> var taskid1 = db.test.test.splitAsync("db1", "db2", 50);
	> var taskid2 = db.my.my.splitAsync("db3", "db4", 50) ;
	> db.waitTasks( taskid1, taskid2 )
	```
