[^_^]:
     alterColumn

##NAME##

alterColumn -  modify the field definition in Schema

##SYNOPSIS##

**schema.addColumn(\<columnName\>, \<columnDef\>)**

##CATEGORY##

SdbInfoSchema

##DESCRIPTION##

This function is used to modify the definition of the specified field in Schema.

##PARAMETERS##

- columnName ( *string, required* )

    Field name.

- columnDef ( *object, required* )

    The field definition that needs to be modified can be selected through the parameter "columnDef":

    - Type ( *string* ): Data type of the field.

        Format: `Type: "int32"`

    - ReadDefault ( *The type corresponds to the value of the field "Type"* ): Read default value.

        Read data through "Information Schema", and the default value to be returned when the specified field does not exist in the record.

        Format: `ReadDefault: 0`

    - WriteDefault ( *The type corresponds to the value of the field "Type"* ): Write default value.

        Write data through "Information Schema", and the default value that needs to be supplemented when the specified field does not exist in the written record.

        Format: `WriteDefault: 0`

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

1. Get a reference to the specified Schema.

    ```lang-javascript
    > var schema = db.getSchema("s1")
    ```

2. Modify the read default value of the field "id" in the Schema to 0.

    ```lang-javascript
    > schema.alterColumn("id", {ReadDefault: 0})
    ```




[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md