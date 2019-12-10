##语法##

***db.traceOff( \<dumpFile\> )***

##类别##

Sdb

##描述##

关闭数据库引擎跟踪功能，并将跟踪情况以二进制文件的形式导出

##参数##

| 参数名   | 参数类型 | 默认值 | 描述                 | 是否必填  |
| -------- | -------- | ------ | -------------------- | --------- |
| dumpFile | string   | ---    | 二进制文件的文件名称 | 是        |

> Note：

> 参数 dumpFile 可以填写为空字符串，表示关闭数据库引擎跟踪功能，但是不导出二进制文件。如果指定文件为相对路径则存放于相应节点的数据目录中的 `diagpath` 目录中；

##返回值##

无返回值

##错误##

如果出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 关闭数据库引擎跟踪 /opt/sequoiadb/trace.dump

	```lang-javascript
	> db.traceOff("/opt/sequoiadb/trace.dump")
	```

* 解析二进制文件可参考 [traceFmt()](reference/Sequoiadb_command/Global/traceFmt.md)

	```lang-javascript
	> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace_output" )
 	```
