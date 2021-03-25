##NAME##

getTime - get the current logical time of the STP node

##SYNOPSIS##

**stp.getTime()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get the current logical time of the STP node in nanoseconds.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the current logical time list of STP node through the cursor. Users can refer to [stpq query time][stpq] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##HISTORY##

v5.0 and above

##EXAMPLES##

Get the current logical time of the STP node.

```lang-javascript
> var stp = new Stp()
> stp.getTime()
{
  "TimeStamp": {
    "Second": 1612364955,
    "NanoSecond": 134292423
  },
  "TimeError": 1000000
}
```

[^_^]:
    Links
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
