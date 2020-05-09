##NAME##

getCoordRG - Get a reference to a coordinated replication group.

##SYNOPSIS##

**db.getCoordRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

Get a reference to a coordinated replication group.

##RETURN VALUE##

On success, return the reference of the replication group.

On error, exception will be thrown. Users can get the error message by [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) or get the error code by [getLastError()](reference/Sequoiadb_command/Global/getLastError.md). For error handling, refer to the common [troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get a reference to a coordinated replication group.

	```lang-javascript
	> var rg = db.getCoordRG()
	```