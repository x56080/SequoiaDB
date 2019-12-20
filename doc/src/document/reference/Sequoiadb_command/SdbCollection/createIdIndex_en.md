##NAME##

createIdIndex - You can set AutoIndexId to false when creating a collection in SequoiaDB. In this way, the collection will not create a default $id index, and data update operations and delete operations will be prohibited. This method can create the $id index, and open update function and delete function at the same time.

##SYNOPSIS##

***db.collectionspace.collection.createIdIndex\(\[options\])***

##CATEGORY##

SdbCollection

##DESCRIPTION##

Create the $id index.

##PARAMETERS##

| Name    | Type | Defaults | Description | Required or not |
| ------- | ---- | -------- | ----------- | --------------- |
| options | JSON | ---      | optional    | not             |

The detailed description of 'options' parameter is as follows:

| Name      | Type | Default | Description |
| --------- | ---- | ------- | ----------- |
| SortBufferSize | int | 64 | The size of sort buffer used when creating index, the unit is MB. And it means don't use sort buffer when the size if zero |

##RETURN VALUE##

On success, return void.

On error, exception will be thrown.

##ERRORS##

| Error code | Description | Solution |
| ---------- | ----------- | -------- |
| -247       | $id index already exists                 |          -               |
| -291       | an index with the same definition exists | delete conflicting index |

when exception happen, use [getLastError()](reference/Sequoiadb_command/Global/getLastError.md) to get the [error code](reference/Sequoiadb_error_code.md)  and use [getLastErrMsg()](reference/Sequoiadb_command/Global/getLastErrMsg.md) to get [error message](reference/Sequoiadb_command/Global/getLastErrMsg.md). For more detial, please reference to [Troubleshooting](troubleshooting/general/general_guide.md).

##EXAMPLES##

* Create the $id index with default parameters.

```lang-javascript
> db.foo.bar.createIdIndex()
```

* Specify sort cache size when creating the $id index.

```lang-javascript
> db.foo.bar.createIdIndex( { SortBufferSize: 128 } )
```



