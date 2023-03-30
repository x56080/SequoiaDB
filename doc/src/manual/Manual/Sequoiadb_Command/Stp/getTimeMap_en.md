##NAME##

getTimeMap - get the logical time and the corresponding system time

##SYNOPSIS##

**stp.getTimeMap()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get the logical time and system time of the STP node.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get the logical time and the corresponding system time through this object. For field descriptions is as follows:

| Name | Type | Description |
| ---- | ---- | ----------- |
| TimeMap.LogicalTime | number | Logical time in microseconds. |
| TimeMap.RealTime | timestamp | System time in microseconds. |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0.4 and above

##EXAMPLES##

Get the logical time and system time of the STP node.

```lang-javascript
> var stp = new Stp()
> stp.getTimeMap()
{
  "TimeMap": [
    {
      "LogicalTime": 1679767981830566,
      "RealTime": {
        "$timestamp": "2023-03-24-04.53.47.000610"
      }
    },
    {
      "LogicalTime": 1679707861320036,
      "RealTime": {
        "$timestamp": "2023-03-24-04.51.01.000039"
      }
    }
  ]
}
```

[^_^]:
    Links
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md