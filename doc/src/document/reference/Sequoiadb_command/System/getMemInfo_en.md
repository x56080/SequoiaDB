##NAME##

getMemInfo - Acquire memory information

##SYNOPSIS##

***System.getMemInfo()***

##CATEGORY##

System

##DESCRIPTION##

Acquire memory information

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return memory information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Acquire memory information

```lang-javascript
> System.getMemInfo()
{
  "Size": 5967,
  "Used": 2919,
  "Free": 384,
  "Unit": "M"
}
```