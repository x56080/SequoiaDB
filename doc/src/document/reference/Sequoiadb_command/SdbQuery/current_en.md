##NAME##

current - Get the record pointed to by the current cursor.

##SYNOPSIS##

***query.current()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the record pointed to by the current cursor.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return rearch result set.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 20 under the collection, bar, and get the record pointed to by the current cursor.

```lang-javascript
> db.foo.bar.find( { age: { $gt: 20 } } ).current()
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
```