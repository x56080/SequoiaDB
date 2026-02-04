##NAME##

getDC - get data center object

##SYNOPSIS##

**db.getDC()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to get the data center object. Through this method, you can obtain an SdbDC object for managing and controlling the status, location information, and operating modes of the data center.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it returns an SdbDC object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `getDC()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error | Check whether the parameter type is correct |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

**Example 1:** Get data center object

```lang-javascript
> var dc = db.getDC()
> dc
datacenter1
```

**Example 2:** Get data center object and activate

```lang-javascript
> var dc = db.getDC()
> dc.activate()
```

**Example 3:** Get data center detailed information

```lang-javascript
> var dc = db.getDC()
> dc.getDetail()
{
  "Activated": true,
  "CATVersion": 3,
  "CSUniqueHWM": 258690,
  "DataCenter": {
    "Address": "sdbserver1:11800,sdbserver2:11800,sdbserver3:11800",
    "BusinessName": "yyy",
    "ClusterName": "xxx"
  },
  "GlobalID": 98807,
  "Readonly": false,
  "RecycleBin": {
    "AutoDrop": true,
    "Enable": true,
    "ExpireTime": 4320,
    "MaxItemNum": 30,
    "MaxVersionNum": 2,
    "RecycleIDHWM": 410998
  },
  "TaskHWM": 704332,
  "Type": "GLOBAL",
  "_id": {
    "$oid": "654c6a81030f266b4709758e"
  }
}
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
