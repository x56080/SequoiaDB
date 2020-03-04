##NAME##

removeData - Remove the specified standalone node in target host of sdbcm.

##SYNOPSIS##

**oma.removeData(\<svcname\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Remove the specified standalone node in target host of sdbcm.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `removeData()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -3 | SDB_PERM | Permission error. | Check if the node path is correct and the path permissions are correct. |
| -15 | SDB_NETWORK | Network error. | 1.Check if the sdbcm status is normal, if the status is abnormal, you can try restart.  2.Check network conditions. |
| -146 | SDBCM_NODE_NOTEXISTED | Node does not exist. | Check if the node exists. |

When error happen, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](troubleshooting/general/general_guide.md) for
more detail.

##HISTORY##

since v2.0

##EXAMPLES##

1. Delete the standalone node with port number 11820. 

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.removeData( 11820 )
 	```