##名称##

listCollections - 枚举域中的集合信息

##语法##

**domain.listCollections()**

##描述##

该函数用于枚举指定域中的全部集合信息。

##参数##

无

##返回值##

函数执行成功时，将返回游标对象。通过游标对象获取的结果字段说明可查看[集合列表](database_management/monitoring/list/SDB_LIST_COLLECTIONS.md)。

函数执行失败时，将抛异常并输出错误信息。


##错误##

当异常抛出时，可以通过 [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) 获取错误信息或通过 [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) 获取错误码。更多错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##版本##



v2.0 及以上版本


##示例##

* 获取指定域下的集合

	```lang-javascript
  > domain.listCollections()
  {
      "Name": "foo.bar" 
  }
	```
