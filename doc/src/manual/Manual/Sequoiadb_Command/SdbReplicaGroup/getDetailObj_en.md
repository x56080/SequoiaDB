##NAME##

getDetailObj - get detailed information of the current replica group

##SYNOPSIS##

**rg.getDetailObj()**

##CATEGORY##

Replica Group

##DESCRIPTION##

Get detailed information of the current replica group, such as group id, group status, node information in the group, etc.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the current group details of type BSONObj. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2.7 and above

v3.4.2 and above

##EXAMPLES##

1. Get detailed information of the replica group named group1.

	```lang-javascript
	> var rg = db.getRG("group1") 
	> rg.getDetailObj()
	```

