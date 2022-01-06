##NAME##

getConf - get the configuration of the STP node

##SYNOPSIS##

**stp.getConf()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get the configuration information, such as the listening port and the maximum tolerable time error of the STP node. 

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the configuration information list of STP node through the cursor. Users can refer to [stpq query configuration][stpq] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Get the configuration of the STP node.

```lang-javascript
> var stp = new Stp()
> stp.getConf()
{
  "port": "9622",
  "serverlist": "server-1:9622",
  "role": "server",
  "syncinterval": 60,
  "maxtimeerror": 50000,
  "diaglevel": 3
}
```

[^_^]:
    Links
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
