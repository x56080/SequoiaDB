##语法##
***db.listProcedures( [cond] )***

枚举所有的存储过程函数。

##参数描述##

| 参数名 | 参数类型  | 描述 													  | 是否必填 |
|--------|-----------| -----------------------------------------------------------|----------|
| cond 	 | Json 对象 | 条件为空时，枚举所有的函数，不为空时，枚举符合条件的函数。 | 否	   	 |


##返回值##

返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 列出所有的函数信息

	```lang-javascript
	> db.listProcedures()
	{ "_id" : { "$oid" : "52480389f5ce8d5817c4c353" }, 
	  "name" : "sum", 
	  "func" : "function sum(x, y) {return x + y;}", 
	  "funcType" : 0 
	}
	{ "_id" : { "$oid" : "52480d3ef5ce8d5817c4c354" }, 
	  "name" : "getAll", 
	  "func" : "function getAll() {return db.foo.bar.find();}", 
	  "funcType" : 0 
	}
	```

* 指定返回函数名为 sum 的记录

	```lang-javascript
	> db.listProcedures({name:"sum"})
	{ "_id" : { "$oid" : "52480389f5ce8d5817c4c353" }, 
	  "name" : "sum", 
	  "func" : "function sum(x, y) {return x + y;}", 
	  "funcType" : 0 
	}
	```
