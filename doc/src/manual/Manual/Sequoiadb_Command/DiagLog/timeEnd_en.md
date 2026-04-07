##NAME##

timeEnd - Sets the end time of the search log in search()

##SYNOPSIS##

**diaglog.search().timeEnd(\<time\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Sets the end time of the search log in search().

##PARAMETERS##

| Name      | Type     | Default | Description     | Required or not |
| -------- | -------- | ------ | --------------- | -------- |
| time  | string   | ---    | Sets the end time for the search log in search(). The time format can be parsed by Date.parse(). | yes       |

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

* Search error -79, and the error occurred before "2025-01-01T12:01:01", limiting the number of results returned to 10.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).timeEnd( '2026-01-01T12:01:01' ).conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto
    ```
