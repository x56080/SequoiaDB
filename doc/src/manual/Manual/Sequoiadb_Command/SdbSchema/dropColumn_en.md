##NAME##

dropColumn - drop the field in Schema

##SYNOPSIS##

**schema.dropColumn(\<columnName\>)**

##CATEGORY##

SdbInfoSchema

##DESCRIPTION##

This function is used to drop the specified field in Schema.

##PARAMETERS##

columnName ( *string, required* )

Field name.

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

1. Get the reference of the specified Schema.

    ```lang-javascript
    > var schema = db.getSchema("s1")
    ```

2. Drop the field "age" in this Schema. 

    ```lang-javascript
    > schema.dropColumn("age")
    ```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md