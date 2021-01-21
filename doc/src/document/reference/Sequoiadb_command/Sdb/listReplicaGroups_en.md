##NAME##

listReplicaGroups - Enumerate replication group information

##SYNOPSIS##

**db.listReplicaGroups()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate replication group information.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [$LIST_GROUP](reference/SQL_grammar/monitoring/LIST_GROUP.md) to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v2.0 and above

##EXAMPLES##

* Return all replication group information

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

	This replication group has two nodes: 11800 and 11850, of which 11850 is the master node. For replication Group details,refer to [Copy List](database_management/monitoring/list/SDB_LIST_GROUPS.md).