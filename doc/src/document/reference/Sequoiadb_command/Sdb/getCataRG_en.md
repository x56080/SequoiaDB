##NAME##

getCataRG - Get the reference of the catalog group.

##SYNOPSIS##

***db.getCataRG()***

##CATEGORY##

Sdb

##DESCRIPTION##

Get the reference of the catalog group.

##PARAMETERS##

No parameters.

##RETURN VALUE##

On success, return the reference of the catalog group.

On error, exception will be thrown.

##ERRORS##

When exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get the catalog group.

```lang-javascript
> var rg = db.getCataRG()
```
