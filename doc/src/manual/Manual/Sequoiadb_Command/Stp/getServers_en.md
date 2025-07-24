##NAME##

getServers - get the server information synchronized by the STP node

##SYNOPSIS##
**stp.getServers()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get the server information synchronized by the STP node. STP node and server group description can refer to [logical time][logicaltime].

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the list of server group information synchronized by the STP node through the cursor. Users can refer to [stpq query server information][stpq] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Get the server information synchronized by the STP node.

```lang-javascript
> var stp = new Stp()
> stp.getServers()
{
  "Version": 1,
  "Group": [
    {
      "Role": "server",
      "HostName": "server-1",
      "Service": "9622"
    }
  ],
  "PrimaryNode": {
    "HostName": "server-1",
    "Service": "9622"
  }
}
```

[^_^]:
    Links
[logicaltime]:manual/Distributed_Engine/Architecture/Stp/logicaltime.md
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
