##名称##

dropCL - 删除当前集合空间下指定的集合。

##语法##

**db.collectionspace.dropCL( \<name\> )**

##类别##

Collection Space

##描述##

删除当前集合空间下指定的集合。

##参数##

* `name` ( *String*， *必填* )

	集合名。

##返回值##

成功：无。

失败：抛出异常。

##错误##

`dropCL()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | 集合不存在。| 检查集合是否存在。|

当异常抛出时，可以通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取[错误码](reference/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/faq.md)了解更多内容。

##版本##

v1.0及以上版本。

##例子##

1. 删除集合空间 foo 下的集合 bar。

	```lang-javascript
	> db.foo.dropCL( "bar" )
 	```
