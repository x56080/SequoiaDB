##NAME##

size - Get the number of records from the current cursor to the final cursor.

##SYNOPSIS##

***query.size()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the number of records from the current cursor to the final cursor.

>**Note:**  

>The result that size() return is influenced by skip() and limit().

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the number of records.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 20 under the collection, bar, and get the number of records from the current cursor to the final cursor.

```lang-javascript
> db.foo.bar.find( { age: { $gt: 20 } } ).size()
1
```