##NAME##

getMeta - get metadata information of STP node

##SYNOPSIS##

**stp.getMeta()**

##CATEGORY##

Stp

##DESCRIPTION##

This function is used to get metadata information, such as version number and synchronization interval of the STP.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return the metadata information list of STP node through the cursor. Users can refer to [stpq query metadata][stpq] to get the returned field information.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Get metadata information of STP node.

```lang-javascript
> var stp = new Stp()
> stp.getMeta()
{
  "MetaSHMKey": "9622",
  "MetaData": {
    "Version": 1,
    "SyncInterval": 60,
    "SyncHWTime": {
      "Second": 23667387,
      "NanoSecond": 823851456
    },
    "BaseHWTime": {
      "Second": 23664627,
      "NanoSecond": 618702083
    },
    "BaseRealTime": {
      "Second": 1612364947,
      "NanoSecond": 487145000
    },
    "Offset": 0,
    "SlewRate": 10000,
    "TimeError": 1000000
  },
  "MetaLSN": {
    "Offset": 1612367707692292,
    "Version": 2
  }
}
```

[^_^]:
    Links
[stpq]:manual/Distributed_Engine/Architecture/Stp/Tools/stpq.md
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
