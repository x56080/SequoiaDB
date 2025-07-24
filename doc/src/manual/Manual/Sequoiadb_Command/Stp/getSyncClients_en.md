##NAME##

getSyncClients - get time synchronization information of the cluster where the STP node is located

##SYNOPSIS##

**stp.getSyncClients()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get time synchronization information, such as the synchronization interval and time tolerance of the cluster where the STP node is located. Users can refer to [stpq query synchronize client information][stpq] to get the returned field information.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the time synchronization information of the cluster where the STP node is located through the cursor. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Get time synchronization information of the cluster where the STP node is located.

```lang-javascript
> var stp = new Stp()
> stp.getSyncClients()
{
  "SyncSource": {
    "HostName": "server-1",
    "Service": "9622"
  },
  "SyncClients": [
    {
      "Role": "client",
      "HostName": "server-2",
      "Service": "9622",
      "OID": {
        "$oid": "6018fc765f862e2e3241e692"
      },
      "SyncInterval": 60,
      "TimeError": 5559908,
      "MaxTimeError": 50000000,
      "SyncPassed": 470,
      "SyncCount": 47,
      "SyncStatus": "CheckOffset",
      "SyncPort": 9622
    }
  ]
}
```

[^_^]:
    Links
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
