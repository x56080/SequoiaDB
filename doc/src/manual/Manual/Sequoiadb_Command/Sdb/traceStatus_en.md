
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

On success, the current program trace status will be returned via a cursor. The returned field information is as follows:

| Name          | Type     |   Description    |
| --------------| ---------| -----------------|
| TraceStarted  | boolean  | Whether trace is enabled   |
| Wrapped       | boolean  | Whether trace memory write is wrapped   |
| Size          | int64    | Trace memory total size     |
| FreeSize      | int64    | Trace memory free size      |
| Mask          | string array   | Component mask. Ref to the `conponent` in [SdbTraceOption][option] |
| BreakPoint    | string array   | Trace break points |
| Threads       | int32 array    | Threads filter   |
| ThreadTypes   | string array   | Thread type filter. Ref to the `threadTypes` in [SdbTraceOption][option] |
| FunctionNames | string array   | Trace function names filter   |
| BreakPointRuns| object array   | Trace break point runtime information   |

- The field in object of `BreakPointRuns` is as follows:

| Name          | Type     |   Description    |
| --------------| ---------| -----------------|
| TID           | int32    | Thread ID        |
| BreakPoint    | string   | Break point function |
| Time          | string   | Break time   |

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()][getLastError] to get the [error code][Sequoiadb_error_code]  and use [getLastErrMsg()][getLastErrMsg] to get detail error message. For more detial, please  reference to [Troubleshooting][faq].

##EXAMPLES##

* Enable trace

```lang-javascript
> db.traceOn( 100, new SdbTraceOption().components( "dms" ).functionNames( "_dmsStorageUnit::insertRecord" ).threadTypes( "RestListener" ) )
```
    
* View trace status
   
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
   ],
   "BreakPointRuns": []
}
```

[^_^]:
    All references and links used in this document
[option]:manual/Manual/Sequoiadb_Command/AuxiliaryObjects/SdbTraceOption.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[Sequoiadb_error_code]:manual/Manual/Sequoiadb_error_code.md