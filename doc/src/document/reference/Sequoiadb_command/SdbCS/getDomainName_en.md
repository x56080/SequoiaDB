##NAME##

getDomainName - get the domain name of the collectionspace

##SYNOPSIS##

**db.collectionspace.getDomainName()**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to get the domain name of the collectionspace.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type String. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When error happen, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md)
to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md)
to get the error code. See [troubleshooting](troubleshooting/general/general_guide.md) for
more detail.

##VERSION##

v3.2 and above

##EXAMPLES##

Get the domain name of the collectionspace "sample".

```lang-javascript
> db.sample.getDomainName()
mydomain
```