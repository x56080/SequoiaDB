##NAME##

arrayAccess - Save the result set in an array and get the record with specified subscript.

##SYNOPSIS##

***query.arrayAccess( \<index\> )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Save the result set in an array and get the record with specified subscript.

##PARAMETERS##

| Name  | Type | Default | Description     | Required or not |
| ----- | ---- | ------- | --------------- | --------------- |
| index | int  | ---     | array subscript | yes             |

##RETURN VALUE##

On success, return the specified record.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Return the record with array subscript 0.

```lang-javascript
> db.foo.bar.find().arrayAccess(0)
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
```
