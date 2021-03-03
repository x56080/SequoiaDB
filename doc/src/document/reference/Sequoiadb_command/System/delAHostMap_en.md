##NAME##

delAHostMap - Delete a hostname to ip address mapping in the host file

##SYNOPSIS##

***System.delAHostMap( \<hostname\> )***

##CATEGORY##

System

##DESCRIPTION##

Delete a hostname to ip address mapping in the host file

##PARAMETERS##

| Name      | Type     | Default | Description   | Required or not |
| ------- | -------- | ------------ | ---------- | -------- |
| hostname     | string   | ---     | hostname       | yes       |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Delete a hostname to ip address mapping in the host file

```lang-javascript
> System.delAHostMap( "hostname" )
```