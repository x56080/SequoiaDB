[^_^]:
   restoreToTime()

##NAME##

restoreToTime - restore the cluster to a consistent time

##SYNOPSIS##

**db.restoreToTime([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to restore the cluster with the Restore mode turned on to the specified time. All data changes committed after the time will be rolled back and currently uncommitted transactions will be terminated.

##PARAMETERS##

options ( *object, required* )

Set the time point to be restored, the available options are as follows.

- Time ( *number/string/Timestamp* ): Specify the target time point of restoration, in seconds.

    When the value of this parameter is 0, it will be restored to the latest consistent point in time; when the value is a string, the value filled in should conform to the ISO 8601 format.

    Format: `Time: 0` or `Time: 1609430400` or `Time: "2021-01-01T00:00:00+08:00"` or `Time: Timestamp("2021-01-01T00:00:00+08:00")`

    
> **Note:**
>
> On Unix-based machines use `date` to get or format timestamps.
>
> - Get the current time in ISO 8601 time.
>
>     ```lang-bash
>     $ date -Iseconds 
>     2021-01-01T00:00:00+08:00
>     ```
>
> - Format date to ISO 8601 format.
>
>     ```lang-bash
>     $ date -Iseconds --date='2021/01/01 15:11:09'
>     2021-01-01T15:11:09+08:00
>     ```
>
> - Get the current timestamp.
>
>     ```lang-bash
>     $ date +%s
>     1609430400
>     ```

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `restoreToTime()` function are as follows:

| Error Code | Error Type | Description | Solution |
|---|---|---|---|
| -359 | SDB_RESTORE_NOT_IN_PROGRESS | Cluster is not in Restore mode | Run db.restorePrepare() to enter Restore mode |
| -360 | SDB_RESTORE_NO_CONSISTENT_PIT | No valid consistency point or the specified time cannot be reached by restore | Restore a backup that covers the given time |

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0.2 and above

##EXAMPLES##

- Restore to the latest consistency point.

    ```lang-javascript
    > db.restoreToTime({Time: 0})
    ```

- Restore using epoch seconds.

    ```lang-javascript
    > db.restoreToTime({Time: 1577836800})
    ```

- Restore using ISO 8601 string.
 
    ```lang-javascript
    > db.restoreToTime({Time: "2020-01-01T00:00:00+00:00"})
    ```

- Restoring using Timestamp.

    ```lang-javascript
    > db.restoreToTime({Time: Timestamp("2020-01-01T00:00:00+00:00")})
    ```


[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/faq.md

