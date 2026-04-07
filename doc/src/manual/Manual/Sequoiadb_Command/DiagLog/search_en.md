##NAME##

search - Specify the running mode as search

##SYNOPSIS##

**diaglog.search([location]).error(\<errorcode\>).limit(\<num\>).conn(\<Sdb\>)**

**diaglog.search([location]).keypattern(\<keyword\>).lastFile(\<num\>).diaglevel(\<0-4\>).conn(\<Sdb\>)**

**diaglog.search([location]).pid(\<pid\>).tid(\<tid\>).lastest(\<minutes\>).path(\<path\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

Set the running mode to search and search for relevant content in the cluster diagnostic logs.

##PARAMETERS##

Only the following command positional parameters are supported: GroupID, GroupName, NodeID, HostName, ServiceName, NodeName and Role.

##RETURN VALUE##

The file name contains the search results.

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

* Search for the most recent logs reporting the error -79, limiting the results to 10.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).conn(db)
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```

* View the contents of the search results file。

    ```lang-javascript
    > diaglog.next()
    ...
    ```

* Close the search results file.

    ```lang-javascript
    > diaglog.close()
    ```

* Search for local files collected by the collect() or the sdb nodes directory "diaglog". The DiagLog object indicates no Sdb connection.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 ).path( '/home/sdbadmin/collect/diaglog_20250101_120101' )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:02:01.000.auto 
    > diaglog.search().error( -79 ).limit( 10 ).path( '/opt/sequoiadb/database/coord/diaglog' )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:02:01.000.auto 
    ```
