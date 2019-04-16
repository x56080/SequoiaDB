##名称##

insert - 将单条或者批量记录插入当前集合。

##语法##
**db.collectionspace.collection.insert(\<doc|docs\>,[flag])**

##类别##

Collection

##描述##

将单条或者批量记录插入当前集合。

## 参数描述##

* `doc|docs` ( *Object|Object of Array*， *必填* )

	单条或者批量记录。

* `flag` ( *Int*， *选填* )

	插入标志位，用于控制插入操作的行为及结果。其默认值为 0。可单独使用或者通过“位与”的方式使用如下选项来控制插入操作的行为及结果：
	* SDB_INSERT_RETURN_ID：表示插入成功后返回记录中“_id”字段的内容。
	* SDB_INSERT_CONTONDUP：默认情况下，当发生索引键冲突时，插入操作将失败并且终止。设置该选项后，当发生索引键冲突时，跳过该条记录并继续插入其他记录。
	* SDB_INSERT_REPLACEONDUP：默认情况下，当发生索引键冲突时，插入操作将失败并且终止。设置该选项后，当发生索引键冲突时，将已存在的记录更新为待插入的新记录，并继续插入其他记录。

**Note:**

* 当插入的记录不包含 “_id” 字段时，SequoiaDB 会自动为记录添加一个 “_id” 字段来唯一标识该记录。

* 参数 `flag` 中的 SDB_INSERT_CONTONDUP 和 SDB_INSERT_REPLACEONDUP 选项不能同时组合使用。

##返回值##

成功：

* 当用户通过 `flag` 参数的 SDB_INSERT_RETURN_ID 选项要求数据库返回记录的“_id”字段的内容时，情况如下：
	
	* 单条插入：直接返回插入记录的“_id”字段的内容。
	* 批量插入：以数组的方式返回插入记录的“_id”字段的内容。

* 其它情况，成功插入无返回值。

失败：抛出异常。

##错误##

`insert()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -6 | SDB_INVALIDARG | 参数错误。 | 查看参数是否填写正确。|
| -23 | SDB_DMS_NOTEXIST| 集合不存在。 | 检查集合是否存在。|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -38 | SDB_IXM_DUP_KEY | 索引键已存在。| 检查插入的记录的索引键是否已经存在。|

当异常抛出时，可以通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取[错误码](reference/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)了解更多内容。

##版本##

v1.0及以上版本。

## 示例##

1. 不指定 _id 字段，插入一条记录。

	```lang-javascript
 	> db.foo.bar.insert( { name: "Tom", age: 20 } )
 	```

2. 插入一条带有 _id 字段的记录。

 	```lang-javascript
 	> db.foo.bar.insert( {_id: 10, age: 20 } )
 	```

3. 插入多条记录，如下操作会在集合bar中插入两条记录。

 	```lang-javascript
 	> db.foo.bar.insert( [ { _id: 20, name: "Mike", age: 15 }, { name: "John", age: 25, phone: 123 } ] )
 	```

4. 插入拥有重复“_id”键的多条记录，如下操作将会在集合bar中插入两条记录。

	```lang-javascript
 	> db.foo.bar.insert( [ { _id: 1, a: 1 }, { _id: 1, b:2 }, { _id: 3, c: 3 } ],  SDB_INSERT_CONTONDUP )
	```

 	```lang-javascript
 	> db.foo.bar.find()
 	{
      "_id": 1,
      "a": 1,
 	}
 	{
      "_id": 3,
      "c": 3
 	}
 	```

