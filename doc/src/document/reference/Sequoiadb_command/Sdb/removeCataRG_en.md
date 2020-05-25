##NAME##

removeCataRG - remove catalog replica group.

##SYNOPSIS##

***db.removeCataRG()***

##CATEGORY##

Sdb

##DESCRIPTION##

Remove catalog replica group, require that there is no data node and coord node information on the catalog replica group, remove catalog replica group will remove all catalog node in the replica group.

##PARAMETERS##

No parameters.

##RETURN VALUE##

On success, no return value.



On error, exception will be thrown.


##ERRORS##

When exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##HISTORY##



Since v1.10


##EXAMPLES##

* Remove catalog replica group.

```lang-javascript
> var rg = db.removeCataRG()
```
