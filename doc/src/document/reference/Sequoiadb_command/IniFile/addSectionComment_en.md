##NAME##

addSectionComment - Append a comment to the specified section.

##SYNOPSIS##

***IniFile.addSectionComment( \<section\> \<comment\> )***

##CATEGORY##

IniFile

##DESCRIPTION##

Append a comment to the specified section.

##PARAMETERS##

| Name     | Type     | Default | Description   | Required or not |
| -------- | -------- | --------| ------------- | --------------- |
| section  | string   | ---     | section name  | yes             |
| comment  | string   | ---     | comment       | yes             |

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

* Append a comment to the specified section.

  ```lang-javascript
  > ini.addSectionComment( "info", "personal information" )
  ```