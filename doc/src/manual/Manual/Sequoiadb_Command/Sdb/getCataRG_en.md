##NAME##

getCataRG - Get the reference of the catalog replication group.

##SYNOPSIS##

***db.getCataRG()***

##CATEGORY##

Sdb

##DESCRIPTION##

Get the reference of the catalog replication group.

##PARAMETERS##

No parameters.

##RETURN VALUE##

On success, return the reference of the catalog replication group.

On error, exception will be thrown.

##ERRORS##

When exception occurs, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detail, please refer to the [Troubleshooting](troubleshooting/general/general_guide.md) manual.

##EXAMPLES##

* Get the catalog replication group.

```lang-javascript
> var rg = db.getCataRG()
```
