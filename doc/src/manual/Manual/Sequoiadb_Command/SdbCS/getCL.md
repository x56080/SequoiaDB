
##名称##

getCL - 获取当前集合空间下指定的集合的对象引用。

##语法##
**db.collectionspace.getCL( \<name\> )**

##类别##

Collection Space

##描述##

* `name` ( *String*， *必填* )

	集合名。

##返回值##

成功：返回指定集合的对象。  

失败：抛出异常。

##错误##

`getCL()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | 集合不存在。| 检查集合是否存在。|

##版本##

v1.0及以上版本。

##示例##

1. 返回集合空间 sample 下集合 employee 的引用。

	```lang-javascript
	> var cl = db.sample.getCL( "employee" )
	```
