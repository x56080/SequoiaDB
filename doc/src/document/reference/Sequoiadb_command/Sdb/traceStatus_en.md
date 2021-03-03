##NAME##

traceStatus - Show the tracking status of the current program.

##SYNOPSIS##

***db.traceStatus()***

##CATEGORY##

Sdb

##DESCRIPTION##

Show the tracking status of the current program.

##PARAMETERS##

None

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Turn on the database engine program tracking.

```lang-javascript
> db.traceOn( 100, new SdbTraceOption().components( "dms" ).functionNames( "_dmsStorageUnit::insertRecord" ).threadTypes( "RestListener" ) )
```

* View the tracking status of the current program. 

```lang-javascript
> db.traceStatus()
{
  "TraceStarted": true,
  "Wrapped": false,
  "Size": 104857600,
  "FreeSize": 104857600,
  "PadSize": 0,
  "Mask": [
    "dms"
  ],
  "BreakPoint": [],
  "Threads": [],
  "ThreadTypes": [
    "RestListener"
  ],
  "FunctionNames": [
    "_dmsStorageUnit::insertRecord"
  ]
}
```
