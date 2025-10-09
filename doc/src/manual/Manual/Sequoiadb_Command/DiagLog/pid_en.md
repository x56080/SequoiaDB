##NAME##

pid - Set the pid for the search() function

##SYNOPSIS##

**diaglog.search().pid(\<pid\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the pid for the search() function.

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| pid  | int   | ---    | Search the logs for the PID. | yes       |

##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Search for logs with PID 12345, limiting the return to 10 results.

    ```lang-javascript
    > diaglog.search().pid( 12345 ).limit( 10 )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
