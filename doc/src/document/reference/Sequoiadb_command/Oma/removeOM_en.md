##NAME##

removeOM - Remove sdbom service(SAC service) in target host of sdbcm.

##SYNOPSIS##

**oma.removeOM(\<svcname\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Remove sdbom service(SAC service) in target host of sdbcm.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `removeOM()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -146 | SDBCM_NODE_NOTEXISTED | Node does not exist. | Check if the node exists. |

When error happen, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](troubleshooting/general/general_guide.md) for
more detail.

##HISTORY##

since v2.0

##EXAMPLES##

1. Delete the sdbcm service process installed locally. 

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.removeOM( 11780 )
 	```