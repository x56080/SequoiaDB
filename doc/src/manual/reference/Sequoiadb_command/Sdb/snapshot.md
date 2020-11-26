##语法##
***db.snapshot( \<snapType\>, [cond], [sel], [sort] )***

***db.snapshot( \<snapType\>, [SdbSnapshotOption] )***



枚举快照，快照是一种得到当前系统状态的命令。查看更多有关[快照信息](manual/Maintainance/Monitoring/snapshot/snapshot.md)

##参数描述##

| 参数名 			| 参数类型 	| 描述 		| 是否必填 |
| ------ 			| ------ 	| ------ 	| ------   |
| snapType 			| 枚举 		| [快照类型](manual/Maintainance/Monitoring/snapshot/snapshot.md)。 | 是 |
| cond 				| Json 对象 | 设置匹配条件以及[命令位置参数](reference/Sequoiadb_command/location.md)。 	| 否 |
| sel 				| Json 对象 | 选择返回字段名。为 null 时，返回所有的字段名。 	| 否 |
| sort 				| Json 对象 | 对返回的记录按选定的字段排序。1为升序；-1为降序。 | 否 |
| SdbSnapshotOption	| Json 对象 | 使用一个对象来指定快照查询参数，使用方法请参考 [SdbSnapshotOption](reference/Sequoiadb_command/AuxiliaryObjects/SdbSnapshotOption.md) | 否 |

> **Note:**

>* snapType 字段的值请参考 [快照类型](manual/Maintainance/Monitoring/snapshot/snapshot.md)。
>* sel 参数是一个json结构，如：{字段名:字段值}，字段值一般指定为空串。sel中指定的字段名在记录中存在，设置字段值不生效；不存在则返回sel中指定的字段名和字段值。
>* 记录中字段值类型为数组，我们可以在sel中指定该字段名，用"."操作符加上双引号("")来引用数组元素。

##返回值##
返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

## 示例##

* 指定 snapType 的值为 SDB_SNAP_CONTEXTS：

	```lang-javascript
	> db.snapshot( SDB_SNAP_CONTEXTS )
	{
	  "SessionID": "vmsvr1-cent-x64-1:11820:22",
	  "Contexts": [
		{
		  "ContextID": 8,
		  "Type": "DUMP",
		  "Description": "BufferSize:0",
		  "DataRead": 0,
		  "IndexRead": 0,
		  "QueryTimeSpent": 0,
		  "StartTimestamp": "2013-12-28-16.07.59.146399"
		}
	  ]
	}
	{
	  "SessionID": "vmsvr1-cent-x64-1:11830:22",
	  "Contexts": [
		{
		  "ContextID": 6,
		  "Type": "DUMP",
		  "Description": "BufferSize:0",
		  "DataRead": 0,
		  "IndexRead": 0,
		  "QueryTimeSpent": 0,
		  "StartTimestamp": "2013-12-28-16.07.59.147576"
		}
	  ]
	}
	{
	  "SessionID": "vmsvr1-cent-x64-1:11840:23",
	  "Contexts": [
		{
		  "ContextID": 7,
		  "Type": "DUMP",
		  "Description": "BufferSize:0",
		  "DataRead": 0,
		  "IndexRead": 0,
		  "QueryTimeSpent": 0,
		  "StartTimestamp": "2013-12-28-16.07.59.148603"
		}
	  ]
	}
	```

* 通过组名或组 ID 查询某个分区组的快照信息，如：

	```lang-javascript
	> db.snapshot( SDB_SNAP_CONTEXTS, { GroupName:'data1' } )
	> db.snapshot(SDB_SNAP_CONTEXTS,{GroupID:1000})
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
	> db.snapshot( SDB_SNAP_CONTEXTS, { GroupName: 'data1', HostName: "vmsvr1-cent-x64-1", svcname: "11820" } )
	> db.snapshot(SDB_SNAP_CONTEXTS,{GroupID:1000,NodeID:1001})
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
	> db.snapshot( SDB_SNAP_CONTEXTS, { HostName: "ubuntu-200-043", svcname: "11820" } )
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

* 返回未在coord聚集前的原始数据：

	```lang-javascript
	> db.snapshot( SDB_SNAP_DATABASE, { RawData: true } ,{ NodeName: null, GroupName: null, TotalDataRead: null } )
	{
	  "NodeName": "ubuntu1604-yt:30000",
	  "GroupName": "SYSCatalogGroup",
	  "TotalDataRead": 276511
	}
	{
	  "NodeName": "ubuntu1604-yt:20000",
	  "GroupName": "db1",
	  "TotalDataRead": 16542209
	}
	{
	  "NodeName": "ubuntu1604-yt:40000",
	  "GroupName": "db2",
	  "TotalDataRead": 959
	}
	Return 3 row(s).
    ```