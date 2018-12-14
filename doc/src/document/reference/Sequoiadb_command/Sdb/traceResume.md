##语法##
***db.traceResume()***

重新开启断点跟踪程序。

 db.traceOn() 指定断点开启数据库引擎程序跟踪功能，当被跟踪的模块遇到断点被阻塞， db.traceResume() 可以唤醒被跟踪的模块。

##参数描述##
无

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 连接数据节点 20000，开启数据库引擎程序跟踪的功能

	```lang-javascript
	> var data = new Sdb( "localhost", 20000 )
	> data.traceOn( 10000, "dms", "_dmsStorageUnit::insertRecord" )
	```

* 连接到协调节点 50000， foo.bar 集合落在数据节点 20000 上，执行插入操作，操作会被阻塞无法完成
   
	```lang-javascript
	> var db = new Sdb( "localhost", 50000 )
	> db.foo.bar.insert( { _id: 1, name: "a" } )
	```      

* 重新开启断点跟踪程序，插入操作执行成功，并返回结果

	```lang-javascript
	> db.traceResume()
	```

* 查看当前断点跟踪程序的状态可参考[traceStatus()](reference/Sequoiadb_command/Sdb/traceStatus.md)

	```lang-javascript
	> db.traceStatus()
	```
