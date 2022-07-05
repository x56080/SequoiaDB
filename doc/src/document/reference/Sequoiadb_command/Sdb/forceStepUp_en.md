##NAME##

forceStepUp - force the standby node to be upgraded to the primary node 

##SYNOPSIS##

**db.forceStepUp([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to forcibly promote the standby node to the primary node in a replication group that is not eligible for election. Before upgrading, make sure that the LSN of the target node is the maximum value in the group. If a node with a smaller LSN is forcibly promoted to master, data will be rolled back. Users can obtain node LSN information through [Node Health Detection Snapshot](database_management/monitoring/snapshot/SDB_SNAP_HEALTH.md).

>**Note:**
>
> This function is only supported in catalog replication groups.

##PARAMETERS##

options ( *object, optional* )

Modify the duration of the primary node through the parameter "options":

- Seconds ( *number* ): The duration of the forced upgrade to the master node, in seconds, the default value is 120.

    When the specified time is exceeded, the replication group will be re-elected according to the election rule.

    Format: `Seconds: 300`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

1. Connect to catalog node 11800.

    ```lang-javascript
    > var cata = new Sdb("localhost", 30000)
    ```

    >**Note:**
    >
    > If the catalog node cannot be connected, the node parameter "auth" needs to be configured to false. For the configurationmethod, can refer to [Parameter Configuration](database_management/database_configuration/configuration_parameters.md).

2. Forcibly promote catalog node 11800 to master with a specified duration of 300 seconds.

    ```lang-javascript
    > cata.forceStepUp({Seconds: 300})
    ```