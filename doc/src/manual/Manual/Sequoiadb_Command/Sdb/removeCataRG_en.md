##NAME##

removeCataRG - remove catalog replication group.

##SYNOPSIS##

***db.removeCataRG()***

##CATEGORY##

Sdb

##DESCRIPTION##

Remove catalog replication group, require that there is no data node and coordination node information on the catalog replication group, remove catalog replication group will remove all catalog node in the replication group.

##PARAMETERS##

No parameters.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

When exception occurs, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detail, please refer to the [Troubleshooting](troubleshooting/general/general_guide.md) manual.

##HISTORY##

Since v1.10

##EXAMPLES##

* Remove catalog replication group.

	```lang-javascript
	> db.removeCataRG()
	```
