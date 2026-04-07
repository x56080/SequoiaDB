##NAME##

collect - The specified running mode is collect.

##SYNOPSIS##

**diaglog.collect([location]).snapshot(\<snapType\>).trap().core().conn(\<Sdb\>)**

**diaglog.collect([location]).all().compress(\<mode\>).conn(\<Sdb\>)**

##CATEGORY##

DiagLog

##DESCRIPTION##

The specified running mode is collect.Search for relevant content in the cluster diagnostic logs and collect the relevant log files locally.

##PARAMETERS##

Only the following command positional parameters are supported: GroupID, GroupName, NodeID, HostName, ServiceName, NodeName and Role.

##RETURN VALUE##

The directory name contains the collected files.

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

* Search for the last 10 logs reporting -79 errors and retrieve the relevant log files locally.

    ```lang-javascript
    > diaglog.collect().error( -79 ).limit( 10 ).conn(db)
    /tmp/sequoiadb/collect/diaglog_20250101_120101.auto
    ```

* Retrieve the CSCL snapshots of the cluster, as well as the trap and core files from all nodes to the local.

    ```lang-javascript
    > diaglog.collect().core().tarp().snapshot('SNAP_CSCL').conn(db)
    /tmp/sequoiadb/collect/diaglog_20250101_120101.auto
    ```
