##NAME##

getIpTablesInfo - Acquire filrewall information

##SYNOPSIS##

***System.getIpTablesInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire firewall information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return firewall information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Acquire firewall information

```lang-javascript
> System.getIpTablesInfo()
```