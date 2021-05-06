##NAME##

getConf - update the configuration of the STP node

##SYNOPSIS##

**stp.updateConf(\<config\>)**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to update the STP node configuration and make the configuration take effect dynamically.

##PARAMETERS##

configs ( *object, required* )

Configuration parameters that to be modified.

Format: `{syncinterval: 100, diaglevel: 3}`

> **Note:**
> 
> * After the configuration is changed, it will be updated to the configuration file and become a fixed configuration. The configuration that is forbidden to be modified will return an error message after the change.
> * Users can use **stp.getConf()** to aquire current configurations of a specific STP node.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Update the configuration of the STP node.

```lang-javascript
> var stp = new Stp()
> stp.updateConf({diaglevel: 5, syncinterval: 100})
```

[^_^]:
    Links
[stp]:manual/Distributed_Engine/Architecture/Stp/Tools/stp.md
[getConf]:manual/Manual/Sequoiadb_Command/Stp/getConf.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/FAQ/faq_sdb.md
