##NAME##

sort - Sort the result set by the specified field.

##SYNOPSIS##

***query.sort( \<sort\> )***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Sort the result set by the specified field.

##PARAMETERS##

| Name | Type     | Default | Description | Required or not |
| ---- | -------- | ------- | ----------- | --------------- |
| sort | JSON     | the result set isn't sorted by default | specified field and sorting rules | yes |

The optional values of the 'sort' parameter are as follows：

| Optional values | Description      |
| --------------- | ---------------- |
| 1               | ascending order  |
| -1              | descending order |

>**Note:**

>When find() uses the [sel](reference/Sequoiadb_command/SdbCollection/find.md), if the option does not cantain the sort field specified by sort(), the sort set by sort() is meaningless and is automatically ignored.

##RETURN VALUE##

On success, return rearch result set.

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 20 under the collection, bar, and return only name field and age field with sorting by ascending order of age field values.

```lang-javascript
> db.foo.bar.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { age: 1 } )
{
  "name": "Jack",
  "age": 22
}
{
  "name": "Tom",
  "age": 23
}
{
  "name": "John",
  "age": 25
}
```

* Specifiy an invalid field.

```lang-javascript
> db.foo.bar.find( { age: { $gt: 20 } }, { age: "", name: "" } ).sort( { "sex": 1 } )
{
  "name": "Jack",
  "age": 22
}
{
  "name": "Tom",
  "age": 23
}
{
  "name": "John",
  "age": 25
}
```
