##NAME##

removeCoordRG - Delete the coordinated replication group.

##SYNOPSIS##

**db.removeCoordRG()**

##CATEGORY##

Sdb

##DESCRIPTION##

Delete the coordinated replication group in the database. In principle, all coordination nodes of the replication group will be deleted. but if the coordination nodes connected to the db object are deleted first during the deletion of these nodes, there may be some coordination nodes left unremoved, and Oma class needs to be used. The removeCoord method removes the left coordination node.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown. Users can get the error message by [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) or get the error code by [getLastError()](reference/Sequoiadb_command/Global/getLastError.md).  For error handling, refer to the common [troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Delete the coordinated replication group.

	```lang-javascript
	> db.removeCoordRG()
	```