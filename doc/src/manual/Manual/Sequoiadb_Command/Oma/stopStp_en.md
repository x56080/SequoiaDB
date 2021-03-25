##NAME##

stopStp - stop STP service

##SYNOPSIS##

**oma.stopStp()**

##CATEGORY##

Oma

##DESCRIPTION##

This function is used to stop STP service(Sequence Time Protocol service) in target host of sdbcm.

##PARAMETERS##

None

##RETURN VALUE##

When the function executes successfully, there is no return value.

When the function fails, an exception will be thrown and an error message will be printed.

##ERRORS##

The common exceptions of `stopStp()` function are as below:

| Error Code | Error Type | Description | Solution |
| ------ | --- | ------------ | ----------- |
| -146 | SDBCM_NODE_NOTEXISTED | STP does not exist. | Check config path of STP `<INSTALL_DIR>/conf/stp/stp.conf` if the STP exists. |

When the exception happens，use [getLastErrMsg()][getLastErrMsg] to get the error message or use [getLastError()][getLastError] to get the [error code][error_code]. For more details, refer to [Troubleshooting][faq].

##VERSION##

v5.0 and above

##EXAMPLES##

Stop the STP process.

```lang-javascript
> var oma = new Oma("localhost",11790)
> oma.stopStp()
```

[^_^]:
    Links:
[getLastErrMsg]:manual/Manual/Sequoiadb_Command/Global/getLastErrMsg.md
[getLastError]:manual/Manual/Sequoiadb_Command/Global/getLastError.md
[faq]:manual/FAQ/faq_sdb.md
[error_code]:manual/Manual/Sequoiadb_error_code.md
