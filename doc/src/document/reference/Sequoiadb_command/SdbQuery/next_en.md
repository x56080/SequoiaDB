##NAME##

next - Get the next record pointed to by the current cursor.

##SYNOPSIS##

***query.next()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Get the next record pointed to by the current cursor.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the next record pointed to by the current cursor.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 20 under the collection, bar, and get the next record pointed to by the current cursor.

```lang-javascript
> db.foo.bar.find( { age: { $gt: 20 } } ).next()
{
  "_id": {
    "$oid": "5cf8aefe5e72aea111e82b39"
  },
  "name": "ben",
  "age": 21
}
```