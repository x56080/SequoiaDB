##名称##

count - 统计当前集合符合条件的记录总数

##语法##

**db.collectionspace.collection.count([cond])**

**db.collectionspace.collection.count([cond]).hint([hint])**

##类别##

SdbCollection

##描述##

该函数用于统计当前集合符合条件的记录总数，可通过 hint 指定查询使用的索引。

##参数##

参数 `cond` 和 `hint` 的用法与 [find()](manual/Manual/Sequoiadb_Command/SdbCollection/find.md) 的相同。

##返回值##

函数执行成功时，将返回一个 CLCount 类型的对象。通过该对象获取符合条件的记录总数。

函数执行失败时，将抛异常并输出错误信息。

##错误##

`count()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | 参数错误。 | 查看参数是否填写正确。|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -23 | SDB_DMS_NOTEXIST| 集合不存在。 | 检查集合是否存在。|

当异常抛出时，可以通过 [getLastErrMsg()][getLastErrMsg] 获取错误信息或通过 [getLastError()][getLastError] 获取错误码。更多错误处理可以参考[常见错误处理指南][error_guide]。

##版本##

v1.0 及以上版本

##示例##

1. 统计集合 employee 所有的记录数，即不指定参数 cond。

	```lang-javascript
	db.sample.employee.count()
	```
2. 统计符合条件 name 字段的值为"Tom"且 age 字段的值大于25的记录数。

	```lang-javascript
	> db.sample.employee.count( { name: "Tom", age: { $gt: 25 } } )
	```

3. 统计符合条件 name 字段的值为"Tom"且 age 字段的值大于25的记录数，使用"nameIdx"索引。

	```lang-javascript
	> db.sample.employee.count( { name: "Tom", age: { $gt: 25 } } ).hint({"":"nameIdx"})
	```

[^_^]:
     本文使用的所有引用及链接
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_guide]:manual/FAQ/faq_sdb.md