
##NAME##

updateNodeConfigs - Use the new configuration information to update the contents in the configuration file of the specified SequoiaDB node.

##SYNOPSIS##

**oma.updateNodeConfigs(\<svcname\>,\<config\>)**

##CATEGORY##

Oma

##DESCRIPTION##

Use the new configuration information to update the contents in the configuration file of the specified SequoiaDB node.


**Note:**

* The updated configuration information needs to restart the node to take effect.

##DESCRIPTION##

* `svcname` ( *Int | String*， *Required* )

	The port of the node.

* `config` ( *Object*， *Required* )

	Node configuration information, specific reference database configuration.

##RETURN VALUE##

On success, no return value.

On error, exception will be thrown.

##ERRORS##

the exceptions of `updateNodeConfigs()` are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -6 | SDB_INVALIDARG | Parameter error. | Check if the parameters are correct. |
| -259 | SDB_OUT_OF_BOUND | No node port number or configuration information entered | Enter the node port number or configuration information |

When error happens, use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md)
to get the error code. See [troubleshooting](manual/FAQ/faq_sdb.md) for
more details.

##HISTORY##

since v2.0

##EXAMPLES##

1. Update the configuration information of the node with port number 11810.

	```lang-javascript
	> var oma = new Oma( "localhost", 11790 )
	> oma.setNodeConfigs( 11810, { diaglevel: 3 } )
	```