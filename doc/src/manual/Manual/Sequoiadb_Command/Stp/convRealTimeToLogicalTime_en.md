##NAME##

convRealTimeToLogicalTime - convert the specified system time to logical time

##SYNOPSIS##

**stp.convRealTimeToLogicalTime(\<realTime\>)**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to convert the specified system time to logical time.

##PARAMETERS##

realTime ( *timestamp, required* )

System time in microseconds, format is `{$timestamp: "2023-03-20-15.46.16.000582"}`.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get the converted logical time through this object. For field descriptions is as follows:

| Name | Type | Description |
| ---- | ---- | ----------- |
| LogicalTime | number | Logical time in microseconds. |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0.4 and above

##EXAMPLES##

Convert the specified system time to logical time.

```lang-javascript
> var stp = new Stp()
> stp.convRealTimeToLogicalTime({$timestamp: "2023-03-20-15.46.16.000582"})
{
  "LogicalTime": 1679358376000577
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md