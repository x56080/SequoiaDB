##NAME##

compress - Set the compression method for collect()

##SYNOPSIS##

**diaglog.collect().compress(\<type\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the compression method for collect().

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| type     | string      | 'tar.gz'    | Compression method: 'tar.gz' or 'zip'  | yes       |

##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Compressed to zip format.

    ```lang-javascript
    > diaglog.collect().all().compress( 'zip' )
    /tmp/sequoiadb/collect/diaglog_20250101_120101.auto
    ```