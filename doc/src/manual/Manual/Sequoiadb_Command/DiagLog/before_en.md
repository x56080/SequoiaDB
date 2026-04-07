##NAME##

before - Set the number of items preceding the search() result context

##SYNOPSIS##

**diaglog.search().before(\<num\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the number of items preceding the search() result context。

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| num      | int      | ---    | Return to the previous num log entries.  | yes       |


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

* Search the logs for the most recent error -79, including the previous log entry for the error, and limit the number of results returned to 10.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).before( 1 ).conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
