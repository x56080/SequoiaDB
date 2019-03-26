##语法##
***traceFmt( \<formatType\>, \<input\>, \<output\> )***

##类别##

Global

##描述##

将 db.traceOff() 导出来的二进制文件格式化输出到指定文件。

##参数##

| 参数名     | 参数类型 | 描述     | 是否必填 |
| ---------- | -------- | -------- | -------- |
| formatType | int      | 格式类型 | 是       |
| input      | string   | 输入文件 | 是       |
| output     | string   | 输出文件 | 是       |

> **Note:**   

> 参数 formatType 只能为 0 或者 1

> * formatType 为 0 时，数据库引擎跟踪程序会将跟踪信息按照进程号（ tid ）进行分类记录在输出文件中

> * formatType 为 1 时，数据库引擎跟踪程序会将跟踪信息按照时间的先后顺序记录在输出文件中

##返回值##

无返回值，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##

| 错误码 | 可能的原因 	  | 解决方法                  |
| ------ | -------------- | ------------------------- |
| -189	 | 跟踪文件不合法 | 确认所用文件是否合法	  |

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 解析二进制文件

	```lang-javascript
	> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace.flw" )
 	```

* 查看当前程序跟踪的状态可参考[traceStatus()](reference/Sequoiadb_command/Sdb/traceStatus.md)

	```lang-javascript
	> db.traceStatus()
	```