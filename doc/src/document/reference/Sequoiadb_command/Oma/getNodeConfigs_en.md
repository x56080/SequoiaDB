##NAME##

getNodeConfigs - Get the configuration information from the  configuration file of specified SequoiaDB node.

##SYNOPSIS##

**oma.getNodeConfigs(\<svcname\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Get the configuration information from the  configuration file of specified SequoiaDB node.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

##RETURN VALUE##

On success, return the configuration information of specified SequoiaDB node.

On error, exception will be thrown.

##ERRORS##

the exceptions of `getNodeConfigs()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -4 | SDB_FNE | File does not exist. | Check the configuration file path is it right or not. |

When error happen, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](troubleshooting/general/general_guide.md) for
more detail.

##HISTORY##

since v2.0

##EXAMPLES##

1. Get the configuration information of the 11820 node on the target host sdbserver1.

	```lang-javascript
	> var oma = new Oma( "sdbserver1", 11790 )
	> oma.getNodeConfigs( 11820 )
    {
    "catalogaddr": "sdbserver1:11803",
    "dbpath": "/opt/sequoiadb/database/data/11820/",
    "diaglevel": "5",
    "role": "data",
    "svcname": "11820"
    }
    Takes 0.000567s.
	```