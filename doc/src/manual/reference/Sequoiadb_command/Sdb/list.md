##语法##
***db.list( \<listType\>, [cond], [sel], [sort] )***

枚举列表，列表是一种轻量级得到当前系统状态的命令。查看更多有关[列表信息](manual/Maintainance/Monitoring/list/list.md)。

##参数描述##

| 参数名   | 参数类型    | 描述   													| 是否必填 |
|----------|-------------|----------------------------------------------------------|----------|
| listType | 枚举        | [列表类型](manual/Maintainance/Monitoring/list/list.md)。| 是 	   |
| cond     | Json 对象   | 设置匹配条件以及[命令位置参数](reference/Sequoiadb_command/location.md)。 | 否 	   |
| sel      | Json 对象   | 选择返回的字段名。为 null 时，返回所有的字段名。         | 否 	   |
| sort     | Json 对象   | 对返回的记录按选定的字段排序。1为升序；-1为降序。        | 否 	   |

>**Note:**

>* listType 字段的值请参考[列表类型](manual/Maintainance/Monitoring/list/list.md)。
>* sel 参数是一个json结构，如：{字段名:字段值}，字段值一般指定为空串。sel中指定的字段名在记录中存在，设置字段值不生效；不存在则返回sel中指定的字段名和字段值。
>* 记录中字段值类型为数组，我们可以在sel中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

##返回值##
返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 指定 listType 的值为 SDB_LIST_CONTEXTS：

	```lang-javascript
	> db.list( SDB_LIST_CONTEXTS )
	{
	  "NodeName": "ubuntu-200-043:11850",
	  "SessionID": 29,
	  "TotalCount": 1,
	  "Contexts": [
		254
	  ]
	}
	```

* 指定 listType 的值为 SDB_LIST_STORAGEUNITS：

	```lang-javascript
	> db.list( SDB_LIST_STORAGEUNITS )
    {
      "NodeName": "ubuntu-200-043:11830",
      "Name": "foo",
      "UniqueID": 61,
      "ID": 4094,
      "LogicalID": 186,
      "PageSize": 65536,
      "LobPageSize": 262144,
      "Sequence": 1,
      "NumCollections": 1,
      "CollectionHWM": 1,
      "Size": 306315264
    }
	```

* 返回符合条件 LogicalID 大于1的记录，并且每条记录只返回 Name 和 ID 这两个字段，记录按 Name 字段的值升序排序

	```lang-javascript
	> db.list( SDB_LIST_STORAGEUNITS, { "LogicalID": { $gt: 1 } }, { Name: "", ID: "" }, { Name: 1 } )
	{
	  "Name": "foo",
	  "ID": 4094
	}
	```

* 指定命令位置参数，只返回数据组 db1 的 context：

	```lang-javascript
	> db.list( SDB_LIST_CONTEXTS, { GroupName: "db1" } )
	{
	  "NodeName": "ubuntu-200-043:20000",
	  "SessionID": 29,
	  "TotalCount": 1,
	  "Contexts": [
		254
	  ]
	}
	```
