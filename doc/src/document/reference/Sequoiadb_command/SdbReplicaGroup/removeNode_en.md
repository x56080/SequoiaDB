##NAME##

removeNode - remove the specified node in the replication group

##SYNOPSIS##

**rg.removeNode(\<host\>, \<service\>, [options])**

##CATEGORY##

SdbReplicaGroup

##DESCRIPTION##

This function is used to remove the specified node in the replication group.

##PARAMETERS##

| Name    | Type       | Description    | Required or not |
|---------|------------|----------------|----------|
| host    | string     | Node hostname.  | yes       |
| service | number | Node port number.   | yes       |
| options | object | Options, Users can refer to the following options description. | not |

options：

| Name    |  Type      |  Description                 |  Default|
| ------- | ---------- | ---------------------------- | ------- |
| Enforced | boolean   | Forced node deletion or not. |  false  |

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](manual/FAQ/faq_sdb.md).

##VERSION##

v2.0 and above

##EXAMPLES##

- Remove the node in the replication group1.

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.removeNode("vmsvr2-suse-x64", 11800)
    ```

- Force removetion of nodes in the replication group1.

    ```lang-javascript
    > var rg = db.getRG("group1")
    > rg.removeNode("vmsvr2-suse-x64", 11800, {Enforced: true})
    ```