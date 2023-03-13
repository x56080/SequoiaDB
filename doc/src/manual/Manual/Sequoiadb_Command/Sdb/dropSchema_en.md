[^_^]:
     dropSchema

##NAME##

dropSchema - drop the specified Schema

##SYNOPSIS##

**db.dropSchema(\<name\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to drop the specified Schema.

##PARAMETERS##

name ( *string, required* )

The name of the Schema.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `dropSchema()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -396 | SDB_SCHEMA_NOT_EXIST | Schema does not exist. | Check whether the specified Schema exists. |
| -6 | SDB_INVALIDARG | Parameter error. | Check whether the parameter type is filled in correctly. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

Drop the Schema named "s1".

```lang-javascript
> db.dropSchema("s1")
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md