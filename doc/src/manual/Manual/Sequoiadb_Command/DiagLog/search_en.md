##NAME##

search - Specify the running mode as search

##SYNOPSIS##

**diaglog.search([location])**

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

* Create a new DiagLog object

    ```lang-javascript
 	> var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Search for the most recent logs reporting the error -79, limiting the results to 10.

    ```lang-javascript
    > diaglog.search().error( -79 ).limit( 10 )
    /tmp/sequoiadb/search/cluster_2025-01-01-12:01:01.000.auto 
    ```
