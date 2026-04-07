##NAME##

lastFile - Set search() to only search recent log files.

##SYNOPSIS##

**diaglog.search().lastFile(\<num\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set search() to only search recent log files..

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| num  | int   | ---    | Set search() to search only the most recent `num` logs. | yes       |


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

* Search for the most recent log file containing the -79 error, limiting the returned results to 10.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).lastFile( 1 ).conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
