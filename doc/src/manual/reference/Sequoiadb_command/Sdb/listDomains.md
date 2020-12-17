##语法##
***db.listDomains( [cond], [sel], [sort] )***

枚举系统中所有由用户创建的域。

##参数描述##

| 参数名   | 参数类型    | 描述   													| 是否必填 |
|----------|-------------|----------------------------------------------------------|----------|
| cond     | Json 对象   | 匹配条件，只返回符合 cond 的记录，为 null 时，返回所有。 | 否 	   |
| sel      | Json 对象   | 选择返回的字段名。为 null 时，返回所有的字段名。         | 否 	   |
| sort     | Json 对象   | 对返回的记录按选定的字段排序。1为升序；-1为降序。        | 否 	   |


##返回值##
返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](manual/faq.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##
* 列出系统中所有由用户创建的域

	```lang-javascript
	> db.listDomains()
	{
	  "_id": {
		"$oid": "5811641e3426f0835eef45bf"
	  },
	  "Name": "mydomain",
	  "Groups": [
		{
		  "GroupName": "group1",
		  "GroupID": 1001
		},
		{
		  "GroupName": "group2",
		  "GroupID": 1002
		},
		{
		  "GroupName": "group3",
		  "GroupID": 1000
		}
	  ]
	}
	```
