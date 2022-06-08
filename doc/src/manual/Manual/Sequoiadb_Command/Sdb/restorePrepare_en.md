[^_^]:
   restorePrepare()

##NAME##

restorePrepare - enable Restore mode

##SYNOPSIS##

**db.restorePrepare()**

##CATEGORY##

Sdb

##DESCRIPTION##

This function is used to enable Restore mode in preparation for an online restore.

> **Note:**
>
> Restore mode blocks both transactional data access and any changes to the cluster. User can still commit or rollback a transaction though.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0.2 and above

##EXAMPLES##

Enable Restore mode for the cluster.

```lang-javascript
> db.restorePrepare()
```

[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/faq.md
