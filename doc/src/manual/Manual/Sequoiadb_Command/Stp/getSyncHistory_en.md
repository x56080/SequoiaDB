##NAME##

getSyncHistory - get the historical time synchronization information between the STP node and each synchronization source

##SYNOPSIS##

**stp.getSyncHistory()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get the historical time synchronization information, such as the number of synchronizations and time offsets between the STP node and each synchronization source.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the historical time synchronization information list of STP node and each synchronization source through the cursor. Users can refer to [stpq query synchronization history][stpq] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Get the historical time synchronization information between the STP node and each synchronization source.

```lang-javascript
> var stp = new Stp()
> stp.getSyncHistory()
{
  "SyncSources": [
    {
      "Role": "server",
      "HostName": "server-1",
      "Service": "9622",
      "SyncCount": 116,
      "ValidCount": 89,
      "MinDelay": 250910,
      "MaxDelay": 19122284,
      "InitOffset": 98988348233399,
      "NegOffset": {
        "Count": 46,
        "Min": -9898,
        "Max": -5513457
      },
      "PosOffset": {
        "Count": 42,
        "Min": 3020,
        "Max": 4935051
      },
      "LastDelay": 584499,
      "LastOffset": -20017,
      "LastPassed": 110
    },
    ...
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
