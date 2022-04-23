##NAME##

limit - controls the number of records returned by the query

##SYNOPSIS##

**query.limit(\<num\>)**

##CATEGORY##

SdbQuery

##DESCRIPTION##

This function is used to control the number of records returned by the query.

##PARAMETERS##

| Name | Type| Description | Required or not |
| ---- | --- | ----------- | --------------- |
| num    | number | Customize the number of records to return to the result set.| required |

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbQuery.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get the error message or use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md). For more details, refer to [Troubleshooting](troubleshooting/general/general_guide.md).

##VERSION##

v2.0 and above

##EXAMPLES##

Get the record with the largest field "age" in the collection "sample.employee".

```lang-javascript
> db.sample.employee.find().sort({age: -1}).limit(1)
{
  "_id": {
    "$oid": "5813035cc842af52b6000009"
  },
  "name": "Tom",
  "age": 22
}
```