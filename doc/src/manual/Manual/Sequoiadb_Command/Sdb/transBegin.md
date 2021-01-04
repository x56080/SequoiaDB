
##语法##
***db.transBegin()***

开启事务，SequoiaDB 数据库事务是指作为单个逻辑工作单元执行的一系列操作。事务处理可以确保除非事务性单元内的所有操作都成功完成，否则不会永久更新面向数据的资源。

##参数描述##
无

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过 [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) 获取错误信息 或 通过 [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) 获取错误码。关于错误处理可以参考[常见错误处理指南](manual/faq.md) 。

##错误##
常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

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