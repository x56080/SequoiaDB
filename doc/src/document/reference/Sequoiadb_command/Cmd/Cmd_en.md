##NAME##

Cmd - Create a Command object.

##SYNOPSIS##

***new Cmd() / [remoteObj](reference/Sequoiadb_command/Remote/Remote.md).getCmd()***

##CATEGORY##

Cmd

##DESCRIPTION##

Create a Command object.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Create a Command object.

```lang-javascript
> var cmd = new Cmd()
```

* Create a remote Command object.

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
> var cmd = remoteObj.getCmd()
```