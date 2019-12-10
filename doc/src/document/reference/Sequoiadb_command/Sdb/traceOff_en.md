##NAME##

traceOff -  Turn off the database engine program tracking and export tracking results to binary files.

##SYNOPSIS##

***db.traceOff( \<dumpFile\> )***

##CATEGORY##

Sdb

##DESCRIPTION##

Turn off the database engine program tracking and export tracking results to binary files.

##PARAMETERS##

| Name        | Type   | Default | Description                  | Required or not |
| ----------- | ------ | ------- | ---------------------------- | --------------- |
| dumpFile    | string | ---     | file'name of the binary file | yes             |

>Note：

>Parameter dumpFile can be filled in as an empty string. It means to turn off the database engine program tracking and but not to export binary file. If the path of the specified file is a relative path, it will store the file in the `diagpath` directory of the corresponding node's database directory.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Turn off the database engine program tracking and export tracking results to binary   files, /opt/sequoiadb/trace.dump.

```lang-javascript
> db.traceOff("/opt/sequoiadb/trace.dump")
```

* Using [traceFmt()](reference/Sequoiadb_command/Global/traceFmt.md) to analysis the binary file.

```lang-javascript
> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace_output" )
```
