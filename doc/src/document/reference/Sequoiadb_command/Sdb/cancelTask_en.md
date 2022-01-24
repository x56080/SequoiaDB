##NAME##

cancelTask - cancel the background task

##SYNOPSIS##

**db.cancelTask(\<id\>, [isAsync])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to cancel the background task.

##PARAMETERS##

- id ( *number, required* )

    Task ID

- isAsync ( *boolean, optional* )

    Whether to cancel the task asynchronously, the default value is false, which means that the task will not be cancelled asynchronously.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `cancelTask()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
|-173|SDB_CAT_TASK_NOTFOUND|Task does not exist.|Use [listTasks()](reference/Sequoiadb_command/Sdb/listTasks.md) to check whether the task exists.|

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

Cancel a split task asynchronously.

```lang-javascript
> var taskid1 = db.sample.employee.splitAsync("group1", "group2", 50)
> db.cancelTask(taskid1, true)
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[listTasks]:manual/Manual/Sequoiadb_Command/Sdb/listTasks.md