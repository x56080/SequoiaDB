##NAME##

core - Set collect() to collect core files

##SYNOPSIS##

**diaglog.collect().core()**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set collect() to collect core files.

##PARAMETERS##

NULL

##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Collect core files.

    ```lang-javascript
    > diaglog.collect().core()
    /tmp/sequoiadb/collect/diaglog_20250101_120101.auto
    ```
