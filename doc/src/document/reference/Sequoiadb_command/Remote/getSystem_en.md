##NAME##

getSystem - Create a remote System object

##SYNOPSIS##

**remoteObj.getSystem()**

##CATEGORY##

Remote

##DESCRIPTION##

This function is used to open a file or create a new file.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return a System object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the error message or use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error code.For more detials, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

* Create a remote object.

```lang-javascript
> var remoteObj = new Remote( "192.168.20.71", 11790 )
```

* Create a remote System object.

```lang-javascript
> var system = remoteObj.getSystem()
```