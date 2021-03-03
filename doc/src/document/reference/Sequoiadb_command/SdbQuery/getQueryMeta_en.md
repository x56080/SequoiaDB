##NAME##

getQueryMeta - Get query metadata information.

##SYNOPSIS##

***query.getQueryMeta()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get query metadata information.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return query metadata information.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Get query metadata information.

```lang-javascript
> db.foo.bar.find().getQueryMeta()
{
  "HostName": "ubuntu",
  "ServiceName": "42000",
  "NodeID": [
    1001,
    1003
  ],
  "ScanType": "tbscan",
  "Datablocks": [
    9
  ]
} 
```