##NAME##

close - Close the file opened by the DiagLog object

##SYNOPSIS##

**diaglog.close()**

##CATEGORY##

DiagLog

##DESCRIPTION##

Close the file opened by the DiagLog object.

##PARAMETERS##

NULL

##RETURN VALUE##

NULL

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Search logs for error -79, limiting the results to 10.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```

* Read the result file
    ```lang-javascript
    > diaglog.next()
    ... 
    ```

* Close the result file
    ```lang-javascript
    > diaglog.close()
    ```
