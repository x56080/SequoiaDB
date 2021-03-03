##NAME##

type - Acquire the type of operating system

##SYNOPSIS##

***System.type()***

##CATEGORY##

System

##DESCRIPTION##

Acquire the type of operating system

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the type of operating system.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Acquire the type of operating system

```lang-javascript
> System.type()
LINUX
```