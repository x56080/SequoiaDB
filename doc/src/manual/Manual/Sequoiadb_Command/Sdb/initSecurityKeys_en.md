##NAME##

initSecurityKeys - initialize the key

##SYNOPSIS##

**db.initSecurityKeys()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to initialize the key. After the initialization is successful, the "MK" key and "DEK" key will be generated and stored in the directory `<catalog_dbpath>/security/MK` and `<catalog_dbpath>/security/DEK`, the file description can refer to [key file][data_encryption].

>**Note:**
>
> Currently only supports executing this function through local connection.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `createCL()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | --- | ------------ | ----------- |
| -394 | SDB_OPERATION_DENIED | Non-local connection. | Perform initialization over a local connection. |
| -398 | SDB_SEC_KEYS_INITIALIZED | Keys already initialized. | - |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.6.1 and above

##EXAMPLES##

Initialize the key.

```lang-javascript
> db = new Sdb("localhost", 11810)
> db.initSecurityKeys()
```

[^_^]:
     Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[data_encryption]:manual/Distributed_Engine/Architecture/data_encryption.md#密钥文件