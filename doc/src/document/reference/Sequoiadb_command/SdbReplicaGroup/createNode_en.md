##NAME##

createNode - create a node in the current replication group

##SYNOPSIS##

**rg.createNode(\<host\>, \<service\>, \<dbpath\>, \[config])**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to create a node in the current replication group.

##PARAMETERS##

- host ( *string, required* )

    Hostname

- service ( *number, required* )

    Node port number

- dbpath ( *string, required* )

    The storage path of the node data file.

    >**Note:**
    >
    > The database management user (created when SequoiaDB is installed, the default is sdbadmin) needs to have write permission on the directory specified by this parameter.

- config ( *object, optional* )

    Node configuration information, such as configuration log size, whether to open transactions and so on. Specific configuration can refer to [parameter description](database_management/database_configuration/parameters_instructions.md).

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createNode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -15      |SDB_NETWORK| Network Error     | 1) Check whether the sdbcm status is normal. If the status is abnormal, try to restart sdbcm.<br> 2) Check whether the "hostname/IP" is correct and whether the network can communicate normally.|
| -145     |SDBCM_NODE_EXISTED| Node already exists   | Check whether the node exists. |
| -157     |SDB_CM_CONFIG_CONFLICTS| Node configuration conflict | Check whether the node port is occupied. |
| -3       | SDB_PERM|Permission error     | Check whether the node path and path permissions are correct. |

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

Create the node "sdbserver1:11830" in the replication group "group1", and specify the size of the synchronization log file to be 64MB.

```lang-javascript
> var rg = db.getRG("group1")
> rg.createNode("sdbserver1", 11830, "/opt/sequoiadb/database/data/11830", {logfilesz: 64})
```

>**Note:**  
>
> Multiple nodes can be created in a replication group, and each node needs to reserve at least five extended ports. Because the system controls five communication interfaces for each node in the background.