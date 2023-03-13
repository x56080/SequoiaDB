##NAME##

addSchema - bind Schema

##SYNOPSIS##

**db.collectionspace.collection.addSchema(\<schemaName\>)**

##CATEGORY##

SdbInfoSchema

##DESCRIPTION##

This function is used to bind the collection with Schema.

##PARAMETERS##

schemaName ( *string, required* )

The name of the Schema. 

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common expretions of `addSchema` function are as follows:

| Error Code | Error Type | Description | Solution |
| ------ | ----------------------- | -------------- | -------------------------- |
| -396   | SDB_SCHEMA_NOT_EXIST    | Schema does not exist.  | Check whether the specified Schema exists. |
| -6     | SDB_INVALIDARG          | Parameter error.        | Check whether the parameter type is filled in correctly.   |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

Bind collection "sample.employee" to Schema named "s1".

```lang-javascript
> db.sample.employee.addSchema("s1")
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md