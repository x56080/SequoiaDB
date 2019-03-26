##语法##

***db.traceOn( \<bufferSize\>, [strComp], [strBreakPoint] )***

##类别##

Sdb

##描述##

开启数据库引擎跟踪功能。

##参数##

| 参数名 		| 参数类型 	| 描述 									| 是否必填 	|
| ------------- | --------- | ------------------------------------- | ----------|
| bufferSize 	| int 		| 开启追踪的文件大小，单位：字节 		| 是 		|
| strComp 		| string 	| 指定模块，默认为所有模块 				| 否 		|
| strBreakPoint | string 	| 于函数处打断点进行跟踪。 				| 否 		|

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 开启数据库引擎程序跟踪的功能

	```lang-javascript
	> db.traceOn(1000)
	```

* 开户数据库引擎程序跟踪功能，指定跟踪的模块名称和指定断点进行跟踪

	```lang-javascript
	> db.traceOn(1000, "cls, dms, mth", "_dmsTempCB::init")
	```

* 当被跟踪的模块遇到断点被阻塞，如果想唤醒被跟踪的模块，具体可参考[traceResume()](reference/Sequoiadb_command/Sdb/traceResume.md)

	```lang-javascript
	> db.traceResume()
	```

* 查看当前程序跟踪的状态，具体可参考[traceStatus()](reference/Sequoiadb_command/Sdb/traceStatus.md)

	```lang-javascript
	> db.traceStatus()
	```

* 关闭数据库引擎跟踪，并将跟踪情况导出二进制文件： /opt/sequoiadb/trace.dump，具体可参考[traceOff()](reference/Sequoiadb_command/Sdb/traceOff.md)

	```lang-javascript
	> db.traceOff("/opt/sequoiadb/trace.dump")
	```

* 解析二进制文件，具体可参考[traceFmt()](reference/Sequoiadb_command/Global/traceFmt.md)

	```lang-javascript
	> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace.flw" )
 	```