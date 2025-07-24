[^_^]:
     restoreCheck()

##NAME##

restoreCheck - check a time for restore

##SYNOPSIS##

**db.restoreCheck([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

The SequoiaDB cluster will calculate the consistency time window for global recovery based on the current logs of each data node and the available log space. This function is used to check whether the specified time point is within the current time window, or check whether the cluster can be restored to the specified time point. At the same time, it can be used to obtain the latest consistent point in time.

##PARAMETERS##

options ( *object, optional* )

Set the time point to check, the available options are as follows.

- Time ( *number/string/Timestamp* ): Specify the target time point to be checked, in seconds.


    When the value of this parameter is 0, the latest consistent time point will be checked. When the value is a string, the value filled in should conform to the ISO 8601 format.

    Format: `Time:0` or `Time:1609430400` or `Time:"2021-01-01T00:00:00+08:00"` or `Time:Timestamp("2021-01-01T00:00:00+08:00")`

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

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of consistency point through this object, the field descriptions are as follows:

| Name | Type | Descriptions |
| ------ | ---- | ---- |
| Time   | string | Consistent point in time available for recovery. |
| MinRecoverableTime | string | The earliest recoverable consistency point, based on the available logs. |
| MaxRecoverableTime | string | If the cluster entered recovery mode by executing [sdbrestore][restore], this value is the latest recoverable consistency point based on the backup; if the cluster entered recovery mode by executing [restorePrepare()][prepare], this value is the time that `restorePrepare()` was executed. |

When the function fails, an exception will be thrown and error message will be printed.

##ERRORS##

The common exceptions of `restoreCheck()` function are as follows:

| Error Code | Error Type | Description | Solution |
|---|---|---|---|
| -6   | SDB_INVALIDARG | The specified time point exceeds the time range recorded in the current log. | The specified consistency time point value range is [MinRecoverableTime,MaxRecoverableTime]. |
| -359 | SDB_RESTORE_NOT_IN_PROGRESS | Cluster is not in Restore mode. | Run db.restorePrepare() to enter Restore mode. |
| -360 | SDB_RESTORE_NO_CONSISTENT_PIT | No valid consistency point or the specified time cannot be reached by restore. | Restore a backup that covers the given time. |

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0.2 and above

##EXAMPLES##

- View the latest recoverable consistency point in time.

    ```lang-javascript
    > db.restoreCheck({Time: 0})
    ```

    The output is as follows:

    ```lang-json
    {
     Time: "2020-01-01T12:00:00+00:00",
     MaxRecoverableTime: "2020-01-01T12:00:00+00:00",
     MinRecoverableTime: "2020-01-01T00:00:00+00:00",
    }
    ```

- Check whether the specified point in time is a recoverable consistency point in time.

    ```lang-javascript
    > db.restoreCheck({Time: "2020-01-01T03:00:00+00:00"})
    ```

    The output is as follows:

    ```lang-json
    {
     Time: "2020-01-01T03:00:00+00:00",
     MaxRecoverableTime: "2020-01-01T12:00:00+00:00",
     MinRecoverableTime: "2020-01-01T00:00:00+00:00",
    }
    ```

- If the synchronization log of a replication group in the cluster does not have enough space to write, even if the specified point in time is a recoverable consistent point in time, no recovery will be performed.

    ```lang-javascript
    > db.restoreCheck({Time:"2021-03-19-12.49.40.012277"})
    ```

    The output is as follows:

    ```lang-json
    (shell):1 uncaught exception: -203
    No writable log space:
    Restore cannot reach 1620338773126102, limited to 1620338780935745
    ```

    Get detailed error information.

    ```lang-javascript
    > getLastErrObj()
    {
     "errno": -203,
     "description": "No writable log space",
     "detail": "Restore cannot reach 1620338773126102, limited to 1620338780935745",
     "ErrNodes": [
       {
         "NodeName": "sdbserver:20000",
         "GroupName": "db1",
         "Flag": -203,
         "ErrInfo": {
           "errno": -203,
           "description": "No writable log space",
           "detail": "Restore cannot reach 1620338773126102, limited to 1620338780935745"
         }
       }
     ]
    }
    ```

- If the specified point in time is before the valid time window, earlier backup is needed to reach the target time.

    ```lang-javascript
    > db.restoreCheck({Time: "2021-03-19-12.49.40.012277"})
    ```

    The output is as follows:


    ```lang-json
    (shell):1 uncaught exception: -6
    Failed restore check
    ```

    Get detailed error information.

    ```lang-json
    > getLastErrObj()
    {
     "Detail": "Some nodes require earlier backups to reach the target time",
     "Time": "2021-03-19-12.49.40.012277",
     "MaxRecoverableTime": "2021-03-19-12.49.46.630301",
     "ErrNodes": [
       {
         "NodeName": "sdbserver:20000",
         "GroupName": "db1",
         "MinRecoverableTime": "2021-03-19-12.49.41.012277",
         "MaxRecoverableTime": "2021-03-19-12.49.46.630301"
       }
     ]
    }
    ```

- If the specified point in time is after the valid time window, later backup is needed to reach the target time.

    ```lang-javascript
    > db.restoreCheck({Time: "2021-03-19-12.50.46.630301"})
    ```

    The output is as follows:

    ```lang-json
    (shell):1 uncaught exception: -6
    Failed restore check
    ```

    Get detailed error information.

    ```lang-json
    > getLastErrObj()
    {
      "Detail": "One or more nodes require later backups to reach the target time",
      "Time": "2021-03-19-12.50.46.630301",
      "ErrNodes": [
        {
          "NodeName": "sdbserver:20000",
          "GroupName": "db1",
          "MinRecoverableTime": "2021-03-19-12.49.41.012277",
          "MaxRecoverableTime": "2021-03-19-12.49.46.630301"
        }
      ]
    }
    ```

- When nodes use different backups for recovery, the log gap between nodes is too large, the log space of the node is not enough, or there are unrecoverable operations (such as DDL operations) on the node, it may cause the cluster to fail to find a valid time window.

    ```lang-javascript
    > db.restoreCheck({Time: 0})
    ```

    The output is as follows:

    ```lang-json
    (shell):1 uncaught exception: -360
    Failed restore check
    ```

    Get detailed error information.

    ```lang-json
    > getLastErrObj()
    {
      "Detail": "No available global consistency point",
      "Time": "1970-01-01-08.00.00.000000",
      "MaxRecoverableTime": "2021-03-19-12.52.01.630926",
      "ErrNodes": [
        {
          "NodeName": "sdbserver:20200",
          "GroupName": "db3",
          "MinRecoverableTime": "2021-03-19-12.52.01.630927",
          "MaxRecoverableTime": "2021-03-19-12.52.01.630926"
        }
      ]
    }
    ```


[^_^]:
    links
[restore]:manual/Distributed_Engine/Maintainance/Backup_Recovery/point_in_time_restore.md
[prepare]:manual/Manual/Sequoiadb_Command/Sdb/restorePrepare.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md