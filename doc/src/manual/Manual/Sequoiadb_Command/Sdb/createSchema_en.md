[^_^]:
     createSchema

##NAME##

createSchema - create a Schema

##SYNOPSIS##

**db.createSchema(\<name\>, \<schemaDef\>)**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to create a Schema and specify the field definitions of the Schema.

##PARAMETERS##

- name ( *string, required* )

    The name of the Schema. Users needs to ensure that it is globally unique.

- schemaDef ( *object, required* )

    Modify the field definition of Schema through the parameter "schemaDef", the format is `{<Name1>: {<field definition>}, <Name2>: {field definition}, ...}`:

    - Name ( *string* ): Field name.
        
        Format: `{"id": {}}`

    - Type ( *string* ): Data type of the field.

        Format: `{"id": {Type: "int32"}}`

    - ReadDefault ( *The type corresponds to the value of the field "Type"* ): Read default value. This parameter is optional.

        Read data through "Information Schema", and the default value to be returned when the specified field does not exist in the record.

        Format: `{"id": {Type: "int32", ReadDefault: 0}}`

    - WriteDefault ( *The type corresponds to the value of the field "Type"* ): Write default value. This parameter is optional.

        Write data through "Information Schema", and the default value that needs to be supplemented when the specified field does not exist in the written record.

        Format: `{"id": {Type: "int32", WriteDefault: 0}}`

##RETURN VALUE##

When the function executes successfully, it will return an object of type SdbInfoSchema. 

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createSchema()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -397 | SDB_SCHEMA_EXIST | Schema already exists. | Check whether a Schema with the same name exists. |
| -6 | SDB_INVALIDARG | Parameter error. | Check whether the parameter type is filled in correctly. |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

Create a Schema named "s1".

```lang-javascript
> db.createSchema("s1", {"id": {Type: "int32", ReadDefault: 1}})
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[SYSSCHEMAS]:manual/Manual/Catalog_Table/SYSSCHEMAS.md