##NAME##

getLastComment - Get the last comment.

##SYNOPSIS##

***IniFile.getLastComment()***

##CATEGORY##

IniFile

##DESCRIPTION##

Get the last comment.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return comment string.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Open an INI file.

```lang-javascript
> var ini = new IniFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
```

* Get the last comment.

```lang-javascript
> ini.getLastComment( "info", "name" )
End of INI file
```