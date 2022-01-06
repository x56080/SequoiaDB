##NAME##

dropCL - drop the specified collection in the current collection space

##SYNOPSIS##

**db.collectionspace.dropCL(\<name\>)**

##CATEGORY##

SdbCS

##DESCRIPTION##

This function is used to delete the specified collection in the current collection space.

##PARAMETERS##

name ( *string, required* )

Collection name.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `dropCL()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ------ | --- | ------ |
| -23 | SDB_DMS_NOTEXIST | Collection does not exist. | Check whether the collection exists.|

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v1.0 and above

##EXAMPLES##

Delete the collection "employee" under the collection space "sample".

```lang-javascript
> db.sample.dropCL("employee")
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md