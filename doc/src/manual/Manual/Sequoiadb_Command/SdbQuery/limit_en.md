
##NAME##

limit - Control the number of records returned by the query.

##SYNOPSIS##

***query.limit( \<num\> )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Control the number of records returned by the query.

##PARAMETERS##

| Name | Type | Default | Description                | Required or not |
| ---- | ---- | ------- | -------------------------- | --------------- |
| num  | int  | defualt to display all records | the number of records returned | yes |

>**Note:**

>If the number of records in the result set is less than num, it is returned according to the actual number of records. Otherwise return only the first "num" records.

##RETURN VALUE##

On success, returns the cursor of the query result set.

On error, exception will be thrown.

##ERRORS##

When exception happens, use [getLastError()](manual/Manual/Sequoiadb_Command/Global/getLastError.md) to get the [error code](manual/Manual/Sequoiadb_error_code.md) and use [getLastErrMsg()](manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md) to get error message. For more details, refer to [Troubleshooting](manual/faq.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 10 under the collection, employee, and return only the first two records.

```lang-javascript
> db.sample.employee.find( { age: { $gt: 10 } } ).limit( 2 )
{
  "_id": {
    "$oid": "5cf8aef75e72aea111e82b38"
  },
  "name": "tom",
  "age": 20
}
{
  "_id": {
    "$oid": "5cf8aefe5e72aea111e82b39"
  },
  "name": "ben",
  "age": 21
}
```