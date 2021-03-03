##NAME##

remove - Delete the result set after the query.

##SYNOPSIS##

***query.remove()***

##CATEGORY##

SdbQuery

##DESCRIPTION##

Delete the result set after the query.

>**Note:**  

>1. Remove() cannot be used with count() and update().  

>2. If remove() is used with sort(), it must use an index when sorts on a single node.  

>3. When remove is used with limit() and skip() in a cluster, it must ensure that the query conditions are executed on a single node or on a single child table.

##PARAMETERS##

NULL

##RETURN VALUE##

On success, return the cursor of the deleted result set. 

On error, exception will be thrown.

##ERRORS##

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please  reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Select the record that the age field value greater than (with using [$gt](reference/operator/match_operator/gt.md)) 10 under the collection, bar, and remove the records.

```lang-javascript
> db.foo.bar.find( { age: { $gt: 10 } } ).remove()
{
  "_id": {
    "$oid": "5d2c4455f6d7aeedc15ddf87"
  },
  "name": "tom",
  "age": 18
}

```