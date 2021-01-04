##NAME##

removeCoordRG - Delete the coordination replication group.

##SYNOPSIS##

**db.removeCoordRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

Delete the coordination replication group in the database. In principle, all coordination nodes of the replication group will be deleted. However, if in the process of deleting these nodes, first delete the coordination nodes which connected to the db object, there may be some coordination nodes left unremoved. Need to use the 'removeCoord' method of class 'Oma' to remove the remaining coordination nodes.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown. Users can get the error message by [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) or get the error code by [getLastError()](reference/Sequoiadb_command/Global/getLastError.md).  For error handling, refer to the common [troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Delete the coordination replication group.

	```lang-javascript
	> db.removeCoordRG()
	```