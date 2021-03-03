##NAME##

toArray - Return the result set as an array.

##SYNOPSIS##

***query.toArray()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Return the result set as an array.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return result set as an array.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Return the first record of the array.

```lang-javascript
> var arr = db.foo.bar.find().toArray()
> arr[0]
{
  "name": "Alice",
  "age": 19
}
```