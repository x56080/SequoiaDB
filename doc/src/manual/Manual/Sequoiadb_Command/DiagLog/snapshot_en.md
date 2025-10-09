##NAME##

snapshot - Set collect() to collect snapshots

##SYNOPSIS##

**diaglog.collect().snapshot(\<type\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

 Set collect() to collect snapshots.

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| type     | string      | ---   | SNAP_CSCL: $SNAPSHOT_CS、$SNAPSHOT_CL、$SNAPSHOT_CATA<br>SNAP_SYS: $SNAPSHOT_SYSTEM、$SNAPSHOT_CONFIGS、$SNAPSHOT_DB、$SNAPSHOT_HEALTH、$SNAPSHOT_SEQUENCES、$SNAPSHOT_SVCTASKS、$SNAPSHOT_TASKS<br>SNAP_SESSION: $SNAPSHOT_SESSION、$SNAPSHOT_CONTEXT<br>SNAP_QUERY: $SNAPSHOT_QUERIES、$SNAPSHOT_LOCKWAITS、$SNAPSHOT_LATCHWAITS、$SNAPSHOT_TRANS、$SNAPSHOT_ACCESSPLANS、$SNAPSHOT_INDEXSTATS、$SNAPSHOT_TRANSDEADLOCK、$SNAPSHOT_TRANSWAIT<br>SNAP_ALL: All of the above  | yes       |

##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Collect all types of snapshots.

    ```lang-javascript
    > diaglog.collect().snapshot( 'SNAP_ALL' )
    /tmp/sequoiadb/collect/diaglog_20250101_120101.auto
    ```
