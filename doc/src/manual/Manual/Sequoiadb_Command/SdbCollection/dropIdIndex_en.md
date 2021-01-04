##NAME##

dropIdIndex - Drop the $id index.

##SYNOPSIS##

**db.collectionspace.collection.dropIdIndex()**

##CATEGORY##

SdbCollection

##DESCRIPTION##

Drop the $id index in the collection. And prohibit the operation of update or delete.

##RETURN VALUE##

On success, return void.

On error, exception will be thrown. Users can get the error message by [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) or get the error code by [getLastError()](reference/Sequoiadb_command/Global/getLastError.md).

##ERRORS##

| Error code | Description | Solution |
| ---------- | ----------- | -------- |
| -47       | $id index does not exist                 |          -               |

##EXAMPLES##

* Drop the $id index.

```lang-javascript
> db.sample.employee.dropIdIndex()
```



