##NAME##

limit - Limit the number of results returned by search()

##SYNOPSIS##

**diaglog.search().limit(\<num\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Limit the number of results returned by search().

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| num  | int   | ---    | Limit search() to return num results | yes       |


##RETURN VALUE##

DiagLog

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