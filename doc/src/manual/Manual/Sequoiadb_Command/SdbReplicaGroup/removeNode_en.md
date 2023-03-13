##NAME##

removeNode - remove the specified node in the replication group

##SYNOPSIS##

**rg.removeNode(\<hostname\>, \<svcname\>, [options])**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to remove the specified node in the current replication group.

##PARAMETERS##

- hostname ( *string, required* )

    Host name.

- svcname ( *number/string, required* )

    Node port name.

- options ( *object, optional* )

    Set the optional parameter through the parameter "options":

    Enforced ( *boolean* )：Whether to forcibly remove the node, and the default value is "false", which means that the node is not forcibly removed.

    - When there are multiple nodes in the replication group, if the user needs to remove the primary node, the user needs to specify "Enforced"  as "true".
    - If the user needs to remove the only empty node in the group, the user needs to specify "Enforced" as "true".

    Format: `Enforced: true`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `removeNode()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -204 | SDB_CATA_RM_NODE_FORBIDDEN | Attempts to remove the only non-empty node within the group. | It is necessary to remove the data of the current node first, and then execute the remove operation. |
| -206 | SDB_CATA_RM_CATA_FORBIDDEN | Attempts to remove the primary catalog node. | Only standby catalog nodes can be removed. |
| -79  | SDB_NET_CANNOT_CONNECT     | The CM process on the remove node host does not exist, or the host is down. | If the user needs to force remove, the user can add  {Enforced: true} option. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

- Remove the node in the replication "group1".

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.removeNode("sdbserver", 11820)
    ```

- Force removetion of nodes in the replication "group1".

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.removeNode("sdbserver", 11820, {Enforced: true})
    ```

[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md