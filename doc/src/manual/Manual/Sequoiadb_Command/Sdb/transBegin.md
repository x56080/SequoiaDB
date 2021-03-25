##名称##

transBegin - 开启事务

##语法##

**db.transBegin()**

##类别##

Sdb

##描述##

该函数用于开启事务，SequoiaDB 数据库事务是指作为单个逻辑工作单元执行的一系列操作。事务处理可以确保除非事务性单元内的所有操作都成功完成，否则不会永久更新面向数据的资源。

##参数##

无

##返回值##

函数执行成功时，无返回值。

函数执行失败时，将抛异常并输出错误信息。

##错误##

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v2.0 及以上版本

##示例##

* 开启事务

	```lang-javascript
	> db.transBegin()
	```

* 插入记录

	```lang-javascript
	> cl.insert({date:99,id:8,a:0})
	```

* 回滚事务，插入的记录将被回滚，集合中无记录

	```lang-javascript
	> db.transRollback()
	> cl.count()
	Return 0 row(s)
	```

* 开启事务

	```lang-javascript
	> db.transBegin()
	```

* 插入记录

	```lang-javascript
	> cl.insert({date:99,id:8,a:0})
	```

* 提交事务，插入的记录将被写入数据库

	```lang-javascript
	> db.transCommit()
	> cl.count()
	1
	```

[^_^]:
     本文使用的所有引用及链接

[list_info]:manual/Manual/List/list.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md