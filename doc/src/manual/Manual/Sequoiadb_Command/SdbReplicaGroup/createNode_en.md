##NAME##

createNode - create a node in the current replication group

##SYNOPSIS##

**rg.createNode(\<host\>, \<service\>, \<dbpath\>, \[config])**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to create a node in the current replication group.

##PARAMETERS##

- host ( *string, required * )

    Specify the host name of the node.

- service ( *number/string, required * )

    Node port number.

- dbpath ( *string, required* )

    - It is the data file path, which is used to store node data files. Ensure that the data administrator (created during installation, default is sdbadmin) user has write permissions.
    - If the configuration path does not start with "/", the data file storage path will be the master directory (default is `/home/sequoiadb`) of the database administrator user (default is sdbadmin)  + configured path.

- config ( *object, optional* )

    Node configuration information, such as configuration log size, whether to open transactions. For more details, refer to [Database Configuration][cluster_config].

> **Note:**  
> The definition format of rg.createNode() has four parameters: host，service，dbpath，config. As is shown in the table above, host，dbpath is a string type, Service type supports number or string, which is required; The last one is a object, which is optional.  
> Format: ("\<hostname\>", "\<service\>", "\<dbpath\>, "[{\<configParam\>: value, ...}])

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createNode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| -------- | --------|-------------- | ------------------------------- |
| -15      |SDB_NETWORK| Network Error     | 1. Check whether the sdbcm status is normal, if the status is abnormal, try to restart. <br> 2. Check whether the host is correct and whether the network can communicate normally. |
| -145     |SDBCM_NODE_EXISTED| Node already exists   | Check whether the node exists. |
| -157     |SDB_CM_CONFIG_CONFLICTS| Node configuration conflict | Execute the "netstat" command to check whether the node port is occupied. |
| -3       | SDB_PERM|Permission error     | Check whether the node path is correct and the path permissions are correct. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Create a "hostname1:11830" node in the replication group "group1" and specify the log file size as 64MB.

```lang-javascript
> var rg = db.getRG("group1")
> rg.createNode("hostname1", 11830, "/opt/sequoiadb/database/data/11830", {logfilesz: 64})
```

> **Note:**  
>
> A replication group can create multiple nodes, and each node needs to reserve at least five sequential ports. Because the system controls five communication interfaces for each node in the backstage.


[^_^]:
    Links
[cluster_config]:manual/Manual/Database_Configuration/configuration_parameters.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
