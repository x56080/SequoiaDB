##语法##
***db.traceStatus()***

查看当前程序跟踪的状态。

##参数描述##
无

##返回值##
无返回值，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。


##示例##

* 查看当前程序跟踪的状态：

	```lang-javascript
	> db.traceStatus()
	{
	  "TraceStarted": true,
	  "Wrapped": false,
	  "Size": 524288,
	  "Mask": 
	  [
		"auth",
		"bps",
		"cat",
		"cls",
		"dps",
		"mig",
		"msg",
		"net",
		"oss",
		"pd",
		"rtn",
		"sql",
		"tools",
		"bar",
		"client",
		"coord",
		"dms",
		"ixm",
		"mon",
		"mth",
		"opt",
		"pmd",
		"rest",
		"spt",
		"util",
		"aggr",
		"spd",
		"qgm"
	  ],
	  "BreakPoint": [],
      "Threads": [],
	}
	```