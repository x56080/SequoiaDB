##NAME##

disableReadonly - disable readonly mode

##SYNOPSIS##

**SdbDC.disableReadonly()**

##CATEGORY##

SdbDC

##DESCRIPTION##

This function is used to disable readonly mode for the data center. After disabling, the data center will resume normal read and write operations.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `disableReadonly()` function are as follows:

| Error Code | Error Type | Description | Solution |
| ---------- | ---------- | ----------- | -------- |
| -6 | SDB_INVALIDARG | Parameter type error | Check whether the parameter type is correct |

When the exception happens, use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v3.0 and above

##EXAMPLES##

**Example 1:** Disable readonly mode for data center

```lang-javascript
> var dc = db.getDC()
> dc.disableReadonly()
```

[^_^]:
    Links
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
