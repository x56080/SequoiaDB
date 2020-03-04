##NAME##

Oma - Class for cluster management.

##SYNOPSIS##
**var oma = new Oma([hostname],[svcname])**

##CATEGORY##

Oma

##DESCRIPTION##

Class for cluster management.

##DESCRIPTION##

* `hostname` ( *String*， *Required* )

	the hostname of the target sdbcm.

* `svcname` ( *Int | String*， *Required* )

	the port of the target sdbcm, default to be 11790.

##RETURN VALUE##

On success, return an object of Oma.

On error, exception will be thrown.

##ERRORS##

the exceptions of `Oma()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -15 | SDB_NETWORK | Network error. | Check the hostname and the port fo sdbcm is reachable. |

When error happen, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](troubleshooting/general/general_guide.md) for
more detail.

##HISTORY##

since v1.12

##EXAMPLES##

1. Create an Oma object from localhost

	```lang-javascript
 	> var oma = new Oma()
 	```

2. Create an Oma object from remote host

	```lang-javascript
 	> var oma = new Oma( "ubuntu-dev1", 11790 )
	```
