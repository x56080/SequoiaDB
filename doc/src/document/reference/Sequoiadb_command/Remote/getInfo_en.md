##NAME##

getInfo - Get the object information of the Remote object.

##SYNOPSIS##

***remoteObj.getInfo()***

##CATEGORY##

Remote

##DESCRIPTION##

Get the object information of the Remote object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the object information of the Remote object.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Create a Remote object.

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
```

* Get the object information of the Remote object.

```lang-javascript
> remoteObj.getInfo()
{
  "hostname": "192.168.20.71",
  "svcname": "11790"
}
```