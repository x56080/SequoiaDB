##NAME##

original - Set the search() function to return the raw log format

##SYNOPSIS##

**diaglog.search().original()**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the search() function to return the raw log format.If this function is not used, the result is a simplified one-line format.

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

* Search for error -79 in the log file, in the raw log format, limited to 10 results.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).original().conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
