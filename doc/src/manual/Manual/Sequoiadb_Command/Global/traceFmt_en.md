
##NAME##

traceFmt - Format trace input to output by type.

##SYNOPSIS##

**traceFmt(\<formatType\>,\<input\>,\<output\>)**

##CATEGORY##

Global

##DESCRIPTION##

The trace file exported by db.traceOff() is in binary format and is not convenient for users to read. This command can be used to format the trace file into user-readable content and output it to the specified file. 

##PARAMETERS##

* `formatType` ( *Int32*, *Required* )

  `formatType` can take the following two values:

  * 0: Output analysis file, including thread execution sequence (output file suffix is .flw), execution time analysis(output file suffix is .CSV), execution time peak(output file suffix is .except) and trace record error information(output file suffix is .error).

  * 1: Output dump record information(output file suffix is .fmt).

  > **Note:**   
	
  > The CSV file can be viewed by using Excel.
 
* `input` ( *String*， *Required* )

	The binary file that db.traceOff() exports

* `output` ( *String*， *Required* )

	Output target file


##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

the exceptions of `traceFmt()` are as below:

| Error code | Error type                | Description             | Solution |
| ---------- | ------------------------- | ----------------------- | -------- |
| -3         | SDB_PERM                  | Permission Error        | Check the path of the input and output file is ok or not. |
| -4         | SDB_FNE                   | File Not Exist          | Check the input file exist or not. |
| -6         | SDB_INVALIDARG            | Invalid Argument        | Check the input format type is valid or not. |
| -189       | SDB_PD_TRACE_FILE_INVALID | Trace file is not valid | Check the input file is ok or not.	|

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](manual/faq.md).

##HISTORY##

Since v1.0.

##EXAMPLES##

* Format trace input to output.

```lang-javascript
> traceFmt( 0, "/opt/sequoiadb/trace.dump", "/opt/sequoiadb/trace_output" )
```

* Using [traceStatus()](manual/Manual/Sequoiadb_command/Sdb/traceStatus.md) to view the tracking status of the current program. 

```lang-javascript
> db.traceStatus()
```