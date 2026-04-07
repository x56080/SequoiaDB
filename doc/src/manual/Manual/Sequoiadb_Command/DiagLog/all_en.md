##NAME##

all - Set collect() to collect trap files, core files, and all snapshots.

##SYNOPSIS##

**diaglog.collect().all()**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set collect() to collect trap files, core files, and all snapshots. This is equivalent to using trap(), core(), and snapshot('SNAP_ALL') simultaneously.

##PARAMETERS##

NULL

##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a Sdb object

    ```lang-javascript
    > var db = new Sdb()
    ```

* Create a DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog()
    ```

* Collect trap files, core files, and all snapshots.

    ```lang-javascript
    > diaglog.collect().all().conn(db)
    /tmp/sequoiadb/collect/diaglog_20250101_120101.auto
    ```
