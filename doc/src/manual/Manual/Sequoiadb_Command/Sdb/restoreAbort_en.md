[^_^]:
   restoreAbort()

##NAME##

restoreAbort - abort Restore mode

##SYNOPSIS##

**db.restoreAbort()**

##CATEGORY##

Sdb

##DESCRIPTION##

Cluster is set to Restore mode after [restorePrepare()][restorePrepare] or an offline restore. The user can abort Restore mode through this function.

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

Abort Restore mode for the cluster.

```lang-javascript
> db.restoreAbort()
```



[^_^]:
    links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
[faq]:manual/faq.md
[restorePrepare]:manual/Manual/Sequoiadb_Command/Sdb/restorePrepare.md

