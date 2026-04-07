##NAME##

conn - Set Sdb connection for the DiagLog object

##SYNOPSIS##

**diaglog.search().conn(\<Sdb\>)**

**diaglog.collect().conn(\<Sdb\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set Sdb connection for the DiagLog object, which is used by the DiagLog object to get cluster information. If only search local files. this function does not need to be used.

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| Sdb      | Object      | ---    | Sdb connection object  | yes       |


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

* Search for the most recent log with error -79, including the log following the error, and limit the returned results to 10

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).after( 1 ).conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
