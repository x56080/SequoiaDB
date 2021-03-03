##NAME##

listCollections - Enumerate collection information

##SYNOPSIS##

**db.listCollections()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enumerate all collection information in the database.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a detailed list of collections through the cursor.Users can refer to [$LIST_CL](reference/SQL_grammar/monitoring/LIST_CL.md) to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error code. For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v2.0 and above

##EXAMPLES##
*  All collection information in the database

	```lang-javascript
	> db.listCollections()
	{
		"Name": "sample.employee"
	}
	```