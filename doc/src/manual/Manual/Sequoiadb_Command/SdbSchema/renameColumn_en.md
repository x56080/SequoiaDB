[^_^]:
     renameColumn

##NAME##

renameColumn - rename the field in Schema

##SYNOPSIS##

**schema.renameColumn(\<oldName\>, \<newName\>)**

##CATEGORY##

SdbInfoSchema

##DESCRIPTION##

This function is used to rename the specified field in Schema.

##PARAMETERS##

- oldName ( *string, required* )

     Old field name.

- newName ( *string, required* )

     New field name.  

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

2. Rename the name of the field "id" in that Schema to "oid".

    ```lang-javascript
    > schema.renameColumn("id", "oid")
    ```




[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md