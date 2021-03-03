##NAME##

count - Get the number of record matching the criteria.

##SYNOPSIS##

***query.count()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the number of record matching the criteria.

>**Note**:

>The result of count() ignores the effects of skip() and limit().

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the number of record matching the criteria.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Select the record whose age is greater than (with using [$gt](reference/operator/match_operator/gt.md)) 10 in the collection bar and return the number of records.

```lang-javascript
> db.foo.bar.find( { age: { $gt: 10 } } ).count()
3
```