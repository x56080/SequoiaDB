##NAME##

sniffPort - Determine whether a port is usable

##SYNOPSIS##

***System.sniffPort( \<port\> )***

##CATEGORY##

System

##DESCRIPTION##

Determine whether a port is usable

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| ------- | -------- | ------------ | ------------ | -------- |
| port  | int   | ---    | port number     | yes       |

##RETURN VALUE##

On success, return true if the port is usable, otherwise return false.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Determine whether a port is usable

```lang-javascript
> System.sniffPort(50000)
{
  "Usable": false
}
```