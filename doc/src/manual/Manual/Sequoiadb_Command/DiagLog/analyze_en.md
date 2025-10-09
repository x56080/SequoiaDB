##NAME##

analyze - The specified running mode is analyze.

##SYNOPSIS##

**diaglog.analyze([location])**

##CATEGORY##

DiagLog

##DESCRIPTION##

The specified running mode is analyze.

##PARAMETERS##

Only the following command positional parameters are supported: GroupID, GroupName, NodeID, HostName, ServiceName, NodeName and Role.

##RETURN VALUE##

The directory name contains the analysis results.

##ERRORS##

when exception happen, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md)  and use [getLastErrMsg()](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](manual/Manual/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](manual/FAQ/faq_sdb.md).

##EXAMPLES##

* Create a new DiagLog object

    ```lang-javascript
 	> var diaglog = new DiagLog( "sdbserver1", 11810, "sdbadmin", "sdbadmin" )
    ```

* Search the 10 most recent log entries for error -79 and retrieve the relevant log files to local machine.

    ```lang-javascript
    > var fileName = diaglog.collect().error( -79 ).limit( 10 ).run()
    ```

* Analyze the retrieved files.

    ```lang-javascript
    > diaglog.analyze().path( fileName )
    /tmp/sequoiadb/analyze/diaglog_2025-01-01-12:01:01.000.auto
    ```
