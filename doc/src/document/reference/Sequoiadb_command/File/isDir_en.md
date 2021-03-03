##NAME##

isFile - Determine if it is a directory.

##SYNOPSIS##

***File.isDir( \<filepath\> )***

##CATEGORY##

File

##DESCRIPTION##

Determine if it is a directory.

##PARAMETERS##

| Name     | Type     | Default | Description | Required or not |
| -------- | -------- | ------- | ----------- | --------------- |
| filepath | string   | ---     | fiel path   | yes             |

##RETURN VALUE##

Return true if the specified file is a directory, or return false.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Determine if it is a directory.

```lang-javascript
> File.isFile( "/opt/sequoiadb" )
true
```