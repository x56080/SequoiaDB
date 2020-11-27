##名称##

find - 查询记录。

##语法##

**db.collectionspace.collection.find([cond],[sel])**

**db.collectionspace.collection.find([cond],[sel]).hint([hint])**

**db.collectionspace.collection.find([cond],[sel]).skip([skipNum]).limit([retNum]).sort([sort])**

**db.collectionspace.collection.find([cond],[sel])[.hint([hint])][.skip([skipNum])][.limit([retNum])][.sort([sort])]**

**db.collectionspace.collection.find([SdbQueryOption])**

##类别##

Collection

##描述##

选择集合的记录，并通过游标（cursor）将记录返回。在 SequoiaDB 中，游标是一个指针，指向一个查询结果集，客户端可以通过游标遍历检索结果。

##参数##

* `cond` ( *Object*， *选填* )

	记录匹配条件。为空时，查询所有记录；不为空时，查询符合条件记录。如：{"age":{"$gt":30}}。匹配条件可使用[匹配符](reference/operator/match_operator/overview.md)或[全文检索语法](manual/Distributed_Engine/Architecture/Data_Model/text_index.md)。

* `sel` ( *Object*， *选填* )

	查询返回记录的字段名。为空时，返回记录的所有字段；如果指定的字段名记录中不存在，则按用户设定的内容原样返回。如：{"name":"","age":"","addr":""}。

* `hint` ( *Object*， *选填* )

	指定查询使用索引的情况。
	* 不指定`hint`：查询是否使用索引及使用哪个索引将由数据库决定；
	* `hint`为{"":null}：查询走表扫描；
	* `hint`为单个索引：如：{"":"myIdx"}，表示查询将使用当前集合中名字为"myIdx"的索引进行；
	* `hint`为多个索引：如：{"1":"idx1","2":"idx2","3":"idx3"}，
                        表示查询将使用上述三个索引之一进行。
                        具体使用哪一个，由数据库评估决定。

* `skipNum` ( *Int32*， *选填* )

	自定义从结果集哪条记录开始返回。默认值为0，表示从第一条记录开始返回。

* `retNum` ( *Int32*， *选填* )

	自定义返回结果集的记录条数。默认值为-1，表示返回从`skipNum`位置开始到结果集结束位置的所有记录。

* `sort` ( *Object*， *选填* )

	指定结果集按指定字段名排序的情况。字段名的值为1或者-1，如：{"name":1,"age":-1}。
	* 不指定`sort`：表示不对结果集做排序；
	* 字段名的值为1：表示按该字段名升序排序；
	* 字段名的值为-1：表示按该字段名降序排序。

* `SdbQueryOption` ( *Object*， *选填* )

	使用一个对象来指定记录查询参数。使用方法可参考[SdbQueryOption](reference/Sequoiadb_command/AuxiliaryObjects/SdbQueryOption.md)。

**注意：**

* `sel`参数为Object类型，其字段内容为空字符串即可，数据库只关心其字段名。

* `hint`参数为Object类型，其字段名可以为任意不重复的字符串，数据库只关心起字段内容。

##返回值##

成功：返回DBCursor对象。

失败：抛出异常。

##错误##

`find()`函数常见异常如下：

| 错误码 | 错误类型 | 描述 | 解决方法 |
| ------ | ------ | --- | ------ |
| -2 | SDB_OOM | 无可用内存。| 检查物理内存及虚拟内存的设置及使用情况。|
| -6 | SDB_INVALIDARG | 参数错误。 | 查看参数是否填写正确。|
| -34 | SDB_DMS_CS_NOTEXIST | 集合空间不存在。| 检查集合空间是否存在。|
| -23 | SDB_DMS_NOTEXIST| 集合不存在。 | 检查集合是否存在。|

当异常抛出时，可以通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取[错误码](reference/Sequoiadb_error_code.md)，
或通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息。
可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)了解更多内容。

##版本##

v1.0及以上版本。

##示例##

1. 查询所有记录，不指定 cond 和 sel 字段。

	```lang-javascript
	> db.foo.bar.find()
 	```

2. 查询匹配条件的记录，即设置 cond 参数的内容。如下操作返回集合 bar 中符合
   条件 age 字段值大于25且 name 字段值为"Tom"的记录。

	```lang-javascript
 	> db.foo.bar.find( { age: { $gt: 25 }, name: "Tom" } )
 	```

3. 指定返回的字段名，即设置 sel 参数的内容。如有记录{ age: 25, type: "system" }
   和{ age: 20, name: "Tom", type: "normal" }，如下操作返回记录的age字段和name字段。

	```lang-javascript
 	> db.foo.bar.find( null, { age: "", name: "" } )
 	{
    	"age": 25,
      	"name": ""
 	}
 	{
      	"age": 20,
      	"name": "Tom"
 	}
 	```
4. 使用索引 ageIndex 遍历集合 bar 下存在 age 字段的记录，并返回。

	```lang-javascript
	> db.foo.test.find( {age: {$exists:1} } ).hint( { "": "ageIndex" } )
	{
  		"_id": {
    		"$oid": "5812feb6c842af52b6000007"
  		},
  		"age": 10
	}
	{
  		"_id": {
    		"$oid": "5812feb6c842af52b6000008"
  		},
  		"age": 20
	}
	```

5. 选择集合 bar 下 age 字段值大于10的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），从第5条记录开始返回，即跳过前面的四条记录。

	```lang-javascript
	> db.foo.bar.find( { age: { $gt: 10 } } ).skip(3).limit(5)
	```
	如果结果集的记录数不大于3，那么无记录返回；
	如果结果集的记录数大于3，则从第4条开始, 最多返回5条记录。

6. 返回集合 bar 中 age 字段值大于20的记录（如使用 [$gt](reference/operator/match_operator/gt.md) 查询），设置只返回记录的 name 和 age 字段，并按 age 字段值的升序排序。

	```lang-javascript
  	> db.foo.bar.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { age: 1 } )
	```
	通过 [find()](reference/Sequoiadb_command/SdbCollection/find.md) 方法，我们能任意选择我们
	想要返回的字段名，在上例中我们选择了返回记录的 age 和 name 字段，此时用 sort() 方法时，
	只能对记录的 age 或 name 字段排序。而如果我们选择返回记录的所有字段，即不设置 find 方法
	的 sel 参数内容时，那么 sort() 能对任意字段排序。

7. 指定一个无效的排序字段。

	```lang-javascript
  	> db.foo.bar.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { "sex": 1 } )
	```
	因为“sex”字段并不存在 find() 方法的 sel 选项 {age:"",name:""} 中，
	所以 sort() 指定的排序字段 {"sex":1} 将被忽略。

8. 使用[全文检索语法](manual/Distributed_Engine/Architecture/Data_Model/text_index.md)查询集合 "bar" 中的 "about" 字段包含 "rock climbing" 的记录。

	```lang-javascript
	> db.foo.bar.find({"":{"$Text":{"query":{"match":{"about" : "rock climbing"}}}}})
	```

