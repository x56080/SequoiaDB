##语法##
***db.listReplicaGroups()***

枚举分区组信息。

##参数描述##
无

##返回值##
返回游标对象，出错抛异常，并输出错误信息，可以通过[getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)获取错误信息或通过[getLastError()](reference/Sequoiadb_command/Global/getLastError.md)获取错误码。
关于错误处理可以参考[常见错误处理指南](troubleshooting/general/general_guide.md)。

##错误##
常见错误可参考[错误码](reference/Sequoiadb_error_code.md)。

##示例##

* 返回所有分区组信息

	```lang-javascript
	> db.listReplicaGroups()
	{
	"Group": 
	[
	  {
		"dbpath": "/opt/sequoiadb/data/11800",
		"HostName": "vmsvr2-suse-x64",
		"Service": [
		  {
			"Type": 0,
			"Name": "11800"
		  },
		  {
			"Type": 1,
			"Name": "11801"
		  },
		  {
			"Type": 2,
			"Name": "11802"
		  },
		  {
			"Type": 3,
			"Name": "11803"
		  }
		],
		"NodeID": 1000
	  },
	  {
		"dbpath": "/opt/sequoiadb/data/11850",
		"HostName": "vmsvr2-suse-x64",
		"Service": [
		  {
			"Type": 0,
			"Name": "11850"
		  },
		  {
			"Type": 1,
			"Name": "11851"
		  },
		  {
			"Type": 2,
			"Name": "11852"
		  },
		  {
			"Type": 3,
			"Name": "11853"
		  }
		],
		"NodeID": 1001
	  }
	],
	"GroupID": 1001,
	"GroupName": "group",
	"PrimaryNode": 1001,
	"Role": 0,
	"Status": 1,
	"Version": 5,
	"_id": {
	  "$oid": "517b2fc33d7e6f820fc0eb57"
	  }
	}
	```

	这个分区组有两个节点：11800和11850，其中11850为主节点。分区组详细信息请见[分区组列表](manual/Maintainance/Monitoring/list/SDB_LIST_GROUPS.md)
