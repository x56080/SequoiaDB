##NAME##

diaglevel - Set the log level to filter in search()

##SYNOPSIS##

**diaglog.search().diaglevel(\<level\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the log level to filter in search().

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| level    | set      | ---    | Optional values ​​[0-4], including lower levels <br>0: SEVERE<br>1: ERROR<br>2: EVENT<br>3: WARNING<br>4: INFO  | yes       |


##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Search for logs containing the most recent error -79, limit the returned results to 10, set the log level to 1, and only include SEVERE and ERROR level logs in the results.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).diaglevel( 1 )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
