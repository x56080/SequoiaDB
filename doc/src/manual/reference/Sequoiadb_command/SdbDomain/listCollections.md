##语法##
***domain.listCollections()***

获取指定域下的全部集合信息。

##参数描述##

无

##返回值##

返回指定域下的集合信息，出错抛异常，并输出错误信息，可以通过
[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。


##错误##

[错误码](reference/Sequoiadb_error_code.md)

##示例##

* 获取指定域下的集合

	```lang-javascript
  > domain.listCollections()
  {
      "Name": "foo.bar" 
  }
	```
