##NAME##

remove - Delete file.

##SYNOPSIS##

***File.remove( \<filepath\> )***

##CATEGORY##

File

##DESCRIPTION##

Delete file or directory.

##PARAMETERS##

| Name     | Type     | Default | Description | Required or not |
| -------- | -------- | ------- | ----------- | --------------- |
| filepath | string   | ---     | file path   | yes             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Delete the file named 'file.txt' in the '/opt/sequoiadb' directory;

```lang-javascript
> File.remove( "/opt/sequoiadb/file.txt" )
```
