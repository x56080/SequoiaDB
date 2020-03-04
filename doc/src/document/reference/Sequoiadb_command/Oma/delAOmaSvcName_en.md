##NAME##

delAOmaSvcName - Delete the service name of sdbcm from its configuration file in target host.

##SYNOPSIS##

**oma.delAOmaSvcName(hostname,[confFile])**

##CATEGORY##

Oma

##DESCRIPTION##

Delete the service name of sdbcm from its configuration file in target host.

##DESCRIPTION##

* `hostname` ( *string*， *Required* )

	The hostname of the target host.

* `configFile` ( *string*， *Optional* )

	The configuration file path, use the default configuration file if not filled.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `delAOmaSvcName()` are as below:

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

1. Delete the service name of sdbcm from its configuration file in sdbserver1 host.

	```lang-javascript
	> var oma = new Oma( "sdbserver1", 11790 )
	> oma.delAOmaSvcName( "sdbserver1")
    ```