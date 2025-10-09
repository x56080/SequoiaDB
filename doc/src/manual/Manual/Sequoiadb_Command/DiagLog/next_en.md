##NAME##

next - Display the log results of the search() search.

##SYNOPSIS##

**diaglog.next(\<num\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Display the log results of the search() search..

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| num     | int      | 1    | The log results from the search() function are displayed as <num> log entries.  | not       |

##RETURN VALUE##

search() Search log results.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Displays two results after searching the logs.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 );
    > diaglog.next(2);
    ...
    ```
