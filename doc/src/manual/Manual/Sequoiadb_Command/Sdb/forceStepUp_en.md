##NAME##

forceStepUp - force the standby node to be upgraded to the primary node 

##SYNOPSIS##

**db.forceStepUp([options])**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to force a standby node to be upgraded to the primary node in a replication group that does not meet the election conditions. Please use this command with caution.

##PARAMETERS##

|Name      |type        |Description  |Required or not |
|--------- |----------- |------------ |----------|
|options   |object      |Parameter set.   | not |

options:

|Name    |type      |Description                           |Default|
|--------- |---------- |------------------------------ |--------|
|Seconds   |number     |Duration of mandatory upgrade to primary node.   |120|

> **Note:**
>
> * This function is currently only available in the catalog group.
> * The primary node cannot exist in the target replication group, and the LSN of other nodes cannot be greater than the LSN of the target node.
> * When the duration expires, all nodes will reelect according to the election rules.
> * If a user is created, the catalog node cannot be directly connected. Users can first modify the auth parameter in the configuration file of the catalog node, and configure auth=false to turn off the authentication function of catalog. Then restart the cluster before proceeding.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v2.0 and above

##EXAMPLES##

Connect catalog node `hostname1:30000` and force it to be promoted to primary node for 300s.

```lang-javascript
> var db = new Sdb("hostname1", 30000)
> db.forceStepUp({Seconds: 300})
```


[^_^]:
   links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md