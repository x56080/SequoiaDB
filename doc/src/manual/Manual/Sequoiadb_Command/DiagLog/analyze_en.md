##NAME##

analyze - The specified running mode is analyze.

##SYNOPSIS##

**diaglog.analyze([location]).path(\<collect filename\>)**

**diaglog.analyze([location]).path(\<collect filename\>).output(\<dir\>)**

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

* Create a Sdb object

    ```lang-javascript
    > var db = new Sdb()
    ```

* Create a DiagLog object

    ```lang-javascript
    > var diaglog = new DiagLog()
    ```

* Search for the last 10 error logs with error code -79, and collect the corresponding log files locally.

    ```lang-javascript
    > var fileName = diaglog.collect().error( -79 ).limit( 10 ).conn(db).run()
    ```

* Analyze the local files.

    ```lang-javascript
    > diaglog.analyze().path( fileName )
    /tmp/sequoiadb/analyze/diaglog_2025-01-01-12:01:01.000.auto
    ```
