
##NAME##

deleteConf - delete node configuration, restoring to its default value.

##SYNOPSIS##
**db.deleteConf(\<config\>,[options])**

##CATEGORY##
Sdb

##DESCRIPTION##
Restore configurations to their default values, reload to take effect and delete configurations from configuration file. Some configurations require restart to take effect, some are not allowed to be changed. 

##PARAMETERS##

* `configs` ( json object , *Required* )

	The specific configurations to update. Including configuration name and placeholder, format: { preferedinstance:1, diaglevel:1 }, '1' is just a placeholder with no actual meaning.

* `options` ( json object )

	Specify location params, such as NodeID, HostName, ServiceName, etc.

	It only takes effect on coordinate nodes. The global nodes in default.

1. Global

	Specify the command's location which is local or global.

	* Global:false  : run on local
	* Global:true   : run on global nodes

2. GroupID

	Specify the command's location by group ID.

	* GroupID:1000
	* GroupID:[1000, 1001, ...]

3. GroupName

	Specify the command's location by group name.

	* GroupName:"db1"
	* GroupName:["db1", "db2", ...]

4. NodeID

	Specify the command's location by node ID.

	* NodeID:1000
	* NodeID:[1000, 1001, ...]

5. HostName

	Specify the command's location by node name.

	* The param should be used with 'ServiceName'.
	* HostName:"host-01"
	* HostName:[ "centos-01", "ubuntu-02", ... ]

6. ServiceName

	Specify the command's location by ServiceName.

	* The param should be used with 'HostName'.
	* ServiceName:"11820"
	* ServiceName:[ "11780", "11810", ... ]

7. NodeSelect

	Specify the command's node select type for a group

	* without the node information. Value is "all",
	* "master", "any" or "secondary".
	* NodeSelect:"all"


**Note:**

* Configurations that require restart or that is not allowed to change will provide more detailed information through error message return value.
* If the default value equals the current value of a certain configuration, then error message will not be reported.
* If no location parameter is provided, the default value will be {Global:true}, i.e., effective on all nodes.
* You can use **Snapshot(SDB_SNAP_CONFIG)** to aquire current configurations of a specific node.

##RETURN VALUE##

No return value, when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/faq.md).

##HISTORY##
Since v2.9.

##EXAMPLES##
1. Delete and restore configuration 'diaglevel' on data node 20000.

	```lang-javascript
	// connect to coord
	> db = new Sdb( "localhost", 11810 )
	> db.deleteConf( { diaglevel:1 }, { GroupName:"db1", ServiceName:"20000" } )
 	```

2. Delete and restore configuration 'preferedinstance' and 'diaglevel' on all nodes of group db2.

	```lang-javascript
	// connect to coord
	> db = new Sdb( "localhost", 11810 )
	> db.deleteConf( { preferedinstance:1, diaglevel:1 }, { GroupName:"db2" } )
	```

3. Get more specific informantion on error.

	```lang-javascript
	// connect to coord
	> db = new Sdb( "localhost", 11810 )
	// set configurations, get error message.
	> db.deleteConf( { transactionon:1 }, { ServiceName:"20000" } )
	(nofile):0 uncaught exception: -264
	One or more nodes did not complete successfully
	Takes 0.009322s.
	// get detailed information that config 'transactionon' requires a restart.
	> getLastErrObj()
	{
		"errno": -264,
		"description": "One or more nodes did not complete successfully",
		"detail": "",
		"ErrNodes": [
		{
			"NodeName": "ubuntu-zwb:20000",
			"GroupName": "db1",
			"Flag": -322,
			"ErrInfo": {
			"errno": -322,
			"description": "Some configuration changes didn't take effect",
			"detail": "Config 'transactionon' require(s) restart to take effect."
			}
		}
		]
	}
	Takes 0.004652s.
	```