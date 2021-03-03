##NAME##

query - Access query result set with using subscript.

##SYNOPSIS##

***query[ \<index\> ]***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Access query result set with using subscript.

##PARAMETERS##

| Name  | Type | Defulat | Description | Required or not |
| ----- | ---- | ------- | ----------- | --------------- |
| index | int  | ---     | subscript   | yes             |

##RETURN VALUE##

On success, return the record of the specified subscript.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Return the record with the subscript 0.

```lang-javascript
> var query = db.foo.bar.find()
> println( query[0] )
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
```