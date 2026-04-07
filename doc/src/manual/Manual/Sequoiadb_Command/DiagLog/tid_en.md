##NAME##

tid - Set the tid for the search in search()

##SYNOPSIS##

**diaglog.search().tid(\<tid\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the tid for the search in search().

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| tid  | int   | ---    | The log file corresponding to the tid searched in the logs | yes       |

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

* Search for logs with TID 12345, and limit the return to 10 results.

    ```lang-javascript
    > diaglog.search().tid( 12345 ).limit( 10 ).conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
