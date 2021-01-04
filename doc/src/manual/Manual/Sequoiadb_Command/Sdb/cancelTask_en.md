
##NAME##

cancelTask - cancel the task.

##SYNOPSIS##
**db.cancelTask( \<id\>, [isAsync] )**

##CATEGORY##

Sdb

##DESCRIPTION##

cancel the task.

##PARAMETERS##

* `id` ( *Int32*, *Required* )

	Task id.

* `isAsync` ( *Bool*, *Optional* )

	Whether the asynchronous.


##RETURN VALUE##

On success, return a new object of Collection.

On error, exception will be thrown.

##ERRORS##

the exceptions of `cancelTask ()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
|-173|SDB_CAT_TASK_NOTFOUND|The specified task does not exist|use [listTasks()](reference/Sequoiadb_command/Sdb/listTasks.md) to check if the task exists |


when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##HISTORY##

since v1.2

##EXAMPLES##

1. Stop split task.

	```lang-javascript
	> var taskid1 = db.sample.employee.splitAsync( "group1", "group2", 50 );
	> db.cancelTask( taskid1, true )
	```