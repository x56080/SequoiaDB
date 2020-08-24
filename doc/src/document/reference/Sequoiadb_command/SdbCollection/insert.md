##名称##

insert - 将单条或者批量记录插入当前集合。

##语法##
**db.collectionspace.collection.insert(\<doc|docs\>,[flag])**

**db.collectionspace.collection.insert(\<doc|docs\>,[options])**

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

* `options` ( *Object*， *选填* )

	插入选项，用于控制插入操作的行为及结果。参数 `options` 等同于参数 `flag`，其属性选项如下：
	* `ReturnOID` ( *Bool*， *选填* )：等同于参数 `flag` 中的 SDB_INSERT_RETURN_ID 选项。
	* `ContOnDup` ( *Bool*， *选填* )：等同于参数 `flag` 中的 SDB_INSERT_CONTONDUP 选项。
	* `ReplaceOnDup` ( *Bool*， *选填* )：等同于参数 `flag` 中的 SDB_INSERT_REPLACEONDUP 选项。

**Note:**

* 当插入的记录不包含 “_id” 字段时，SequoiaDB 会自动为记录添加一个 “_id” 字段来唯一标识该记录。

* 参数 `flag` 中的 SDB_INSERT_CONTONDUP 和 SDB_INSERT_REPLACEONDUP 选项不能同时组合使用。使用参数 `options` 时的情况与使用参数 `flag` 的一致。

##返回值##

* 成功返回详细结果信息（BSONObj 对象），结构如下：

 ```lang-json
 {
		InsertedNum    : <INT32>  成功插入的记录数，不包含替代和忽略的记录,
		DuplicatedNum  : <INT32>  因重复键冲突被忽略或替代的记录数
 }
 ```

  当用户开启 `flag` 参数的 SDB_INSERT_RETURN_ID 选项或者 `options` 参数的 ReturnOID 选项时，详细结果信息中还包含 "_id" 字段，情况如下：

	* 单条插入：直接返回插入记录的“_id”字段的内容。
	* 批量插入：以数组的方式返回插入记录的“_id”字段的内容。

  当集合包含[自增字段](data_model/auto_increment.md)时，详细结果中还包含 "LastGenerateID" 字段。它返回了插入记录中自动生成的自增字段值。当插入操作生成多个自增字段值时，总是取第一个值，情况如下：

    * 批量插入：只返回第一条记录的自增字段值。
    * 多个自增字段：只返回第一个自增字段的值。

* 出错抛异常。

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

5. 插入记录，并以 Json 对象的方式返回结果。

 	```lang-javascript
 	> db.foo.bar.insert({a:1}, {ReturnOID:true, ContOnDup:true})
 	{
   		"_id": {
     		"$oid": "5becec3d6404b9295a63caca"
   		}
		"InsertedNum": 1,
  		"DuplicatedNum": 0
 	}
	>
 	> db.foo.bar.insert([{a:1}, {b:1}], {ReturnOID:true, ContOnDup:true})
 	{
   		"_id": [
     		{
       			"$oid": "5bececdf6404b9295a63cacb"
     		},
     		{
       			"$oid": "5bececdf6404b9295a63cacc"
     		}
   		]
		"InsertedNum": 2,
		"DuplicatedNum": 0
 	}
    > db.foo.bar.createAutoIncrement({ Field: "ID" })
    > db.foo.bar.insert({ a: 1 })
    {
        "InsertedNum": 1,
        "DuplicatedNum": 0,
        "LastGenerateID": 1
    }
 	```
