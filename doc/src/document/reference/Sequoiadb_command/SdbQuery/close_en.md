##NAME##

close - Close the cursor.

##SYNOPSIS##

***query.close()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Close the cursor.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Close the cursor.

```lang-javascript
> var query = db.foo.bar.find()
> query.close()
```

* Return the record with the subscript 0.

```lang-javascript
> query[0]
uncaught exception: -31
Failed to get next
```