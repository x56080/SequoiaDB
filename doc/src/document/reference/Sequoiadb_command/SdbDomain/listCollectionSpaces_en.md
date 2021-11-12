##NAME##

listCollectionSpaces - list the collection space contained in the domain

##SYNOPSIS##

**domain.listCollectionSpaces()**

##CATEGORY##

SdbDomain

##DESCRIPTION##

This function is used to list the collection space contained in the specified domain.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbCursor. Users can get a list of collection space information contained in the domain through this object.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v3.2 and above

##EXAMPLES##

List the collection space information contained in the specified domain.

```lang-javascript
> var domain = db.getDomain('mydomain')
> domain.listCollectionSpaces()
{
  "Name": "sample1"
}
{
  "Name": "sample2"
}
```
