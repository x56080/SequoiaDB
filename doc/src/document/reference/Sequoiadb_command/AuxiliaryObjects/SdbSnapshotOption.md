指定快照查询参数。

包括指定选择条件、返回字段名、排序情况、快照参数以及对返回结果集的处理等参数。

##语法##

**SdbSnapshotOption[.cond(\<cond\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.sel(\<sel\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.sort(\<sort\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.options(\<options\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.skip(\<skipNum\>)]  
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;[.limit(\<retNum\>)]**

**SdbSnapshotOption[.cond(\<cond\>)][.skip(\<skipNum\>)][.limit(\<retNum\>)]**

**SdbSnapshotOption[.cond(\<cond\>)].options(\<options\>)**

##方法##

###cond(\<cond\>)###

选择条件。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|cond |	Json 对象 | 选择条件，只返回 cond 字段指定的节点或分区组的快照信息，为 null 时，返回整个集群的快照信息。 | 是 |

###sel(\<sel\>)###

查询返回记录的字段名。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|sel     |Json 对象 | 选择返回字段名。为 null 时，返回所有的字段名。 | 是 |

> **Note：**

>* sel 参数是一个json结构，如：{字段名:字段值}，字段值一般指定为空串。sel中指定的字段名在记录中存在，设置字段值不生效；不存在则返回sel中指定的字段名和字段值。
>* 记录中字段值类型为数组的，我们可以在sel中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

###sort(\<sort\>)###

查询返回记录的字段名。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
|sort |	Json 对象 | 指定结果集按指定字段名排序的情况。字段名的值为1或者-1，如：{"name":1,"age":-1}。1代表升序；-1代表降序。 如果不设定 sort 则表示不对结果集做排序。 | 是 |

###options(\<options\>)###

查询返回记录的字段名。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| options   |	Json 对象 | 指定快照参数，因不同快照类型而异，在对应[快照类型](database_management/monitoring/snapshot/snapshot.md)查看选项及示例。  | 是 |

####options选项####
| 参数名  | 参数类型 | 对应快照 | 描述 | 是否必填 |
| ------  | -------- | -------- | ---- | -------- |
| Mode    |  String  | [配置快照](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md) | 指定返回配置的模式。在 run 模式下，显示当前运行时配置信息，在 local 模式下，显示配置文件中配置信息。如 { "Mode": "local" }。默认为 run。 | 否 |
| Expand  |  Bool/String  | [配置快照](database_management/monitoring/snapshot/SDB_SNAP_CONFIGS.md) | 是否扩展显示用户未配置的配置项。如 { "Expand": false }。默认为 true。| 否 |
| ShowError | String | ALL | 指定是否返回错误信息。在 show 模式下，显示错误信息，在 only 模式下，只显示错误信息，不显示其他快照信息，在 ignore 模式下，不显示错误信息。如 { "ShowError: "only" }。默认为 show。 | 否 |
| ShowErrorMode | String | ALL | 指定返回错误信息的格式。在 aggr 模式下，错误信息聚合为一条记录显示，在 flat 模式下，一个错误节点对应一条记录显示。如 { "ShowErrorMode": "flat" }。默认为 aggr。 | 否 |

> **Note：**

> * ShowError 参数和 ShowErrorMode 参数只在协调节点执行快照生效。
> * ShowError 参数和 ShowErrorMode 参数仅支持会返回错误信息的快照。
> * 当 ShowError 参数为 ignore 的情况下，ShowErrorMode 参数不起作用。
> * 特别地，当 ShowErrorMode 参数为 flat 的情况下，对[系统快照](database_management/monitoring/snapshot/SDB_SNAP_SYSTEM.md)和[数据库快照](database_management/monitoring/snapshot/SDB_SNAP_DATABASE.md)不起作用  。

###skip(\<skipNum\>)###

查询返回记录的字段名。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| skipNum | int | 自定义从结果集哪条记录开始返回。默认值为0，表示从第一条记录开始返回。 | 是 |

> **Note：**

>如果不设定 skipNum 的内容或者设定 skipNum 的值为0，相当于返回所有的结果集；如果想从结果集的第3条记录开始返回，可设置 skipNum 的值等于2。

###limit(\<retNum\>)###

查询返回记录的字段名。

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| retNum | int | 自定义返回结果集的记录条数。默认值为-1，表示返回从`skipNum`位置开始到结果集结束位置的所有记录。 | 是 |

> **Note：**

>如果不设定 retNum 的内容，相当于返回所有的结果集记录。如果想返回结果集的前5条记录，可设置 retNum 的值为5。

##返回值##

返回自身，类型为 SdbSnapshotOption。

##错误##

[错误码](reference/Sequoiadb_error_code.md)


##示例##

* 通过组名或组 ID 查询某个分区组的快照信息，如：

	```lang-javascript
    > var option = new SdbSnapshotOption().cond( { GroupName:'data1' } )
	> db.snapshot( SDB_SNAP_CONTEXTS, option )
	{
	  "SessionID": "vmsvr1-cent-x64-1:11820:22",
	  "Contexts": [
		{
		  "ContextID": 11,
		  "Type": "DUMP",
		  "Description": "BufferSize:0",
		  "DataRead": 0,
		  "IndexRead": 0,
		  "QueryTimeSpent": 0,
		  "StartTimestamp": "2013-12-28-16.13.57.864245"
		}
	  ]
	}
	{
	  "SessionID": "vmsvr1-cent-x64-1:11840:23",
	  "Contexts": [
		{
		  "ContextID": 10,
		  "Type": "DUMP",
		  "Description": "BufferSize:0",
		  "DataRead": 0,
		  "IndexRead": 0,
		  "QueryTimeSpent": 0,
		  "StartTimestamp": "2013-12-28-16.13.57.865103"
		}
	  ]
	}
	```

* 通过“组名+主机名+服务名”或“组 ID+节点 ID”查询某个节点的快照信息，如：

	```lang-javascript
    > var option = new SdbSnapshotOption().cond( { GroupName: 'data1', HostName: "vmsvr1-cent-x64-1", ServiceName: "11820" } )
	> db.snapshot( SDB_SNAP_CONTEXTS, option )
	{
	  "SessionID": "vmsvr1-cent-x64-1:11820:22",
	  "Contexts": [
		{
		  "ContextID": 11,
		  "Type": "DUMP",
		  "Description": "BufferSize:0",
		  "DataRead": 0,
		  "IndexRead": 0,
		  "QueryTimeSpent": 0,
		  "StartTimestamp": "2013-12-28-16.13.57.864245"
		}
	  ]
	}
	```

* 通过“主机名+服务名”查询某个节点的快照信息，如：

	```lang-javascript
    > var option = new SdbSnapshotOption().cond( { HostName: "ubuntu-200-043", ServiceName: "11820" } )
	> db.snapshot( SDB_SNAP_CONTEXTS, option )
	{
	  "NodeName": "ubuntu-200-043:11820",
	  "SessionID": 18,
	  "Contexts": [
		{
		  "ContextID": 31,
		  "Type": "DUMP",
		  "Description": "IsOpened:1,HitEnd:0,BufferSize:0",
		  "DataRead": 0,
		  "IndexRead": 0,
		  "QueryTimeSpent": 0,
		  "StartTimestamp": "2016-10-27-17.53.45.042061"
		}
	  ]
	}
	```

* 查看数据组 db1 中数据节点 20000 上配置文件中的配置信息并指定快照参数。

	```lang-javascript
	> var option = new SdbSnapshotOption().cond( { GroupName:'db1', ServiceName:'20000' } ).options( { "mode": "local", "expand": false } )
	> db.snapshot( SDB_SNAP_CONFIGS, option )
	{
  	"NodeName": "ubuntu-zwb:20000",
  	"dbpath": "/home/equoiadb/20000/",
  	"ServiceName": "20000",
  	"diaglevel": 3,
  	"role": "data",
  	"catalogaddr": "ubuntu-zwb:30003,ubuntu-zwb:30013,ubuntu-zwb:30023",
  	"perfstat": "FALSE",
  	"businessname": "yyy",
  	"clustername": "xxx"
	}
	Return 1 row(s).
	```
