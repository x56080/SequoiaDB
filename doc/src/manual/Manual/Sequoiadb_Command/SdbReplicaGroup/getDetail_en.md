##NAME##

getDetail - get detailed information of the current replica group

##SYNOPSIS##

**rg.getDetail()**

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

When the exception happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](manual/faq.md).

##VERSION##

v1.10 and above

##EXAMPLES##

1. Get detailed information of the replica group named group1.

	```lang-javascript
	> var rg = db.getRG("group1") 
	> rg.getDetail()
	```

