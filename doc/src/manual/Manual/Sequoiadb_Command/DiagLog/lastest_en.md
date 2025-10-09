##NAME##

lastest - Set search() to search only logs from a recent time period.

##SYNOPSIS##

**diaglog.search().lastest(\<minutes\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set search() to search only logs from a recent time period.

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| minutes  | int   | ---    | Set search() to only search logs from the most recent minutes. | yes       |


##RETURN VALUE##

DiagLog

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Search the logs that reported error -79 within the last 10 minutes, limiting the number of results returned to 10.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).lestest( 10 )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
