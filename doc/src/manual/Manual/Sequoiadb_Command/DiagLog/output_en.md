##NAME##

output - Set the output paths for search() and analyze()

##SYNOPSIS##

**diaglog.search().output(\<path\>)**

**diaglog.analyze().output(\<path\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the output paths for search() and analyze()。

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| ---------- | -------- | ------------------ | --------------- | -------- |
| path   | string   | ---          | Set the output paths for search() and analyze(), which must be absolute paths.          | yes       |

##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Place the result file of collect() into the specified directory.

    ```lang-javascript
    > diaglog.collect().all().path( '/home/sdbadmin/collect' )
    /home/sdbadmin/collect/diaglog_20250101_120101
    ```

* Specify the directory to search() and output the results to the specified directory.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).path( '/home/sdbadmin/collect/diaglog_20250101_120101' ).output( '/home/sdbadmin/search' )
    /home/sdbadmin/search/result
    ```

* Specify the directory to analyze in analyze(), and output the results to the specified directory.

    ```lang-javascript
    > diaglog.analyze().path( '/home/sdbadmin/collect/diaglog_20250101_120101' ).output( '/home/sdbadmin/search' )
    /home/sdbadmin/analyze/result
    ```

