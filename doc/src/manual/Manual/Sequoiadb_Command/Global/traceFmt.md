
##语法##

**traceFmt(\<formatType\>,\<input\>,\<output\>)**

##类别##

Global

##描述##

将 db.traceOff() 导出来的 trace 文件格式化为用户可读的内容，并输出到指定文件。

##参数##

* `formatType` ( *Int32*， *必填* )

	traceFmt输出两种信息:

 0：输出分析文件，包含线程的执行序列（ flw 文件）、函数的执行时间分析（ CSV 文件）、执行时间峰值（ except 文件）、 trace 记录错误信息（ error 文件）；

 1：输出 dump 记录信息（ fmt 文件）；

	> **Note:**   

	> CSV 文件可以使用 Excel 软件查看

* `input` ( *String*， *必填* )

	db.traceOff() 导出来的二进制文件。

* `output` ( *String*， *必填* )

	输出的目标文件。

##返回值##

成功：无返回值。

失败：抛出异常。

##错误##

`traceFmt()`函数常见异常如下：

| 错误码 | 错误类型                  | 可能的原因            | 解决方法               |
| ------ | ------------------------- | --------------------- | ---------------------- |
| -3     | SDB_PERM                  | 权限错误              | 检查输入、输出文件路径是否存在权限问题 |
| -4     | SDB_FNE                   | 文件不存在            | 检查输入文件是否存在   |
| -6     | SDB_INVALIDARG            | 参数错误              | 检查输入的类型是否正确 |
| -189   | SDB_PD_TRACE_FILE_INVALID | 输入的trace文件不合法 | 确认输入的文件是否合法 |

如果出错则抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/FAQ/faq_sdb.md)。

##版本##

v1.0及以上版本。

##示例##

* 解析二进制文件

	```lang-javascript
	> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace_output" )
 	```

* 查看当前程序跟踪的状态可参考[traceStatus()](manual/Manual/Sequoiadb_Command/Sdb/traceStatus.md)

	```lang-javascript
	> db.traceStatus()
	```