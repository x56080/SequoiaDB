##名称##

count - 统计当前集合符合条件的记录总数。

##语法##
**db.collectionspace.collection.count([cond])**

**db.collectionspace.collection.count([cond]).hint([hint])**

##类别##

Collection

##描述##
统计当前集合符合条件的记录总数，可通过hint指定查询使用的索引。

##参数##

参数`cond`和`hint`的用法与[find()](reference/Sequoiadb_command/SdbCollection/find.md)的相同。

##返回值##

成功：返回符合条件的记录总数。  

失败：抛出异常。

##错误##

`count()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | 参数错误。 | 查看参数是否填写正确。|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -23 | SDB_DMS_NOTEXIST| 集合不存在。 | 检查集合是否存在。|

当异常抛出时，可以通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取[错误码](reference/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](manual/faq.md)了解更多内容。

##版本##

v1.0及以上版本。

##示例##

1. 统计集合 bar 所有的记录数，即不指定参数 cond。

	```lang-javascript
 	> db.foo.bar.count()
 	```

2. 统计符合条件 name 字段的值为"Tom"且 age 字段的值大于25的记录数。

	```lang-javascript
	> db.foo.bar.count( { name: "Tom", age: { $gt: 25 } } )
	```

3. 统计符合条件 name 字段的值为"Tom"且 age 字段的值大于25的记录数，使用"nameIdx"索引。

	```lang-javascript
	> db.foo.bar.count( { name: "Tom", age: { $gt: 25 } } ).hint({"":"nameIdx"})
	```
