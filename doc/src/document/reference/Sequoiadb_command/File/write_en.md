##NAME##

write - Write to a file descriptor.

##SYNOPSIS##

***File.write( \<content\> )***

##CATEGORY##

File

##DESCRIPTION##

Write to a file descriptor.

##PARAMETERS##

| Name    | Type     | Default | Description                 | Required or not |
| ------- | -------- | ------- | --------------------------- | --------------- |
| content | string   | ---     | what is written to the file | yes             |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Open a file and get a file descriptor;

```lang-javascript
> var file = new File( "/opt/sequoiadb/file.txt" )
```

* Write file;

```lang-javascript
> file.write( "sequoiaDB" )
```