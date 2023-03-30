##NAME##

convLogicalTimeToRealTime - convert the specified logical time to system time

##SYNOPSIS##

**stp.convLogicalTimeToRealTime(\<logicalTime\>)**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to convert the specified logical time to system time.

##PARAMETERS##

logicalTime ( *number, required* )

Logical time in microseconds.

##RETURN VALUE##

When the function executes successfully, it will return an object of type BSONObj. Users can get the converted system time through this object. For field descriptions is as follows:

| Name | Type | Description |
| ---- | ---- | ----------- |
| RealTime | timestamp | System time in microseconds. |

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0.4 and above

##EXAMPLES##

1. Get the current logical time.

    ```lang-javascript
    > var stp = new Stp()
    > stp.getTimeUS()
    {
      "TimeStamp": 1679339776883577,
      "TimeError": 1000000
    }
    ```

2. Convert logical time to system time.

    ```lang-javascript
    > stp.convLogicalTimeToRealTime(1679339776883577)
    {
      "RealTime": {
        "$timestamp": "2023-03-20-10.36.16.000582"
      }
    }
    ```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md