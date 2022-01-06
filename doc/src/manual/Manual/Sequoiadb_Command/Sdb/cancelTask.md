
##名称##
cancelTask - 取消任务。

##语法##
**db.cancelTask( \<id\>, [isAsync] )**

##类别##
Sdb

##描述##

取消任务。

##参数##

* `id` ( *Int32*， *必填* )

    任务ID。

* `isAsync` ( *Bool*， *选填* )

    是否异步。

##返回值##

成功：返回新集合的对象。  

失败：抛出异常。

##错误##

`cancelTask()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
|-173|SDB_CAT_TASK_NOTFOUND|任务不存在|使用[listTasks()][listTasks]检查任务是否存在

当异常抛出时，可以通过 [getLastError()][getLastError] 获取[错误码][Sequoiadb_error_code]，
或通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息。
可以参考[常见错误处理指南][faq]了解更多内容。

##版本##

v1.2及以上版本。

##示例##

1. 停止切分任务。

	```lang-javascript
	> var taskid1 = db.sample.employee.splitAsync( "group1", "group2", 50 );
	> db.cancelTask( taskid1, true )
	```

[^_^]:
    本文使用的所有引用及链接
[listTasks]:manual/Manual/Sequoiadb_Command/Sdb/listTasks.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[faq]:manual/FAQ/faq_sdb.md

