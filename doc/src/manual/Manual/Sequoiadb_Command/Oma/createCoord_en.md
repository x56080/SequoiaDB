
##NAME##

createCoord - Create a coord node in target host of sdbcm.

##SYNOPSIS##

**oma.createCoord(\<svcname\>,\<dbpath\>,[config])**

##CATEGORY##

Oma

##DESCRIPTION##

Create a coord node in target host of sdbcm, in general, the coord node is only used temporarily.

**Note:**

* Coord Nodes created through this interface cannot be managed by the cluster, and coord nodes that can be managed by the cluster can be created through the createNode() interface.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

* `dbpath` ( *String*， *Required* )

	The node data directory.

* `config` ( *Object*， *Optional* )

	Node configuration information.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `createCoord()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -3 | SDB_PERM | Permission error. | Check if the node path is correct and the path permissions are correct. |
| -15 | SDB_NETWORK | Network error. | 1.Check if the sdbcm status is normal, if the status is abnormal, you can try restart.  2.Check network conditions. |
| -145 | SDBCM_NODE_EXISTED | Node already exist. | Check if the node already exists. |
| -157 | SDB_CM_CONFIG_CONFLICTS | Node configuration conflict. | Check if the port and data directory are used. |

When error happen, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/faq.md) for
more details.

##HISTORY##

since v2.0

##EXAMPLES##

1. Create a coord node with port number 11810 and associate the node with the specified catalog node.

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.createCoord( 11810, "/opt/sequoiadb/database/coord/11810", { catalogaddr: "ubuntu1:11823, ubuntu2:11823" } )
	```