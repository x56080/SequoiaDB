##NAME##

mkdir - Create a directory.

##SYNOPSIS##

***File.mkdir( \<name\>, \[mode\] )***

##CATEGORY##

File

##DESCRIPTION##

Copy fileCreate a directory.

##PARAMETERS##

| Name    | Type     | Default | Description               | Required or not |
| ------- | -------- | ------- | ------------------------- | --------------- |
| name    | string   | ---     | the name of directory     | yes             |
| mode    | int      | 0755    | set directory permissions | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Create a directory.

```lang-javascript
> File.mkdir( "/opt/sequoiadb/newDir" )
```

