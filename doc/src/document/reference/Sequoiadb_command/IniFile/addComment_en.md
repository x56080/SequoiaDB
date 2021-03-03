##NAME##

addComment - Append a comment to the specified item.

##SYNOPSIS##

***IniFile.addComment( \<section\>, \<key\>, \<comment\>, \[pos\] )***

***IniFile.addComment( \<key\>, \<comment\>, \[pos\] )***

##CATEGORY##

IniFile

##DESCRIPTION##

Append a comment to the specified item.

##PARAMETERS##

| Name     | Type     | Default | Description                            | Required or not |
| -------- | -------- | --------| -------------------------------------- | --------------- |
| section  | string   | ---     | section name                           | yes             |
| key      | string   | ---     | item key name                          | yes             |
| comment  | string   | ---     | comment                                | yes             |
| pos      | boolean  | true    | true: pre-comment; false: post comment | not             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Open an INI file.

```lang-javascript
> var ini = new IniFile( "/opt/sequoiadb/file.ini", SDB_INIFILE_FLAGS_DEFAULT )
```

* Append a comment to the specified item.

```lang-javascript
> ini.addComment( "info", "name", "what's your name" )
```