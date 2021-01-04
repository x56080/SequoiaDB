
##语法##
***db.listCollections()***

枚举数据库中所有的集合信息。

##参数描述##
无

##返回值##
返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##
常见错误可参考[错误码](manual/Manual/Sequoiadb_error_code.md)。

##示例##
*  数据库中所有的集合信息

	```lang-javascript
	> db.listCollections()
	{
		"Name": "sample.employee"
	}
	```
